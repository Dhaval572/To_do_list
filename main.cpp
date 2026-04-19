#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <algorithm>
#include <optional>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <charconv>

#include <hiredis/hiredis.h>
#include "json.hpp"

using json = nlohmann::json;
using namespace std::literals;

// ANSI Color Codes
namespace Color
{
    constexpr auto RESET   = "\033[0m"sv;
    constexpr auto RED     = "\033[91m"sv;
    constexpr auto GREEN   = "\033[92m"sv;
    constexpr auto YELLOW  = "\033[93m"sv;
    constexpr auto BLUE    = "\033[94m"sv;
    constexpr auto CYAN    = "\033[96m"sv;
    constexpr auto BOLD    = "\033[1m"sv;
}

// Helper Functions
namespace Helper
{
    [[nodiscard]] std::string get_current_time()
    {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%d-%m-%Y %H:%M:%S");
        return ss.str();
    }

    void clear_screen()
    {
        std::cout << "\033[2J\033[1;1H";
    }
}

// Task Structure
struct Task
{
    int id;
    std::string title;
    std::string priority;
    std::string due_date;
    std::string status;
    std::string created_at;
    std::string completed_at;
};

// Date Validation (Zero overhead with string_view)
namespace DateValidation
{
    struct Result
    {
        bool valid;
        std::string error_message;
        int day;
        int month;
        int year;
    };

    [[nodiscard]] Result validate(std::string_view date_str)
    {
        if (date_str.size() != 10)
            return {false, "Date must be in DD-MM-YYYY format", 0, 0, 0};

        if (date_str[2] != '-' || date_str[5] != '-')
            return {false, "Use DD-MM-YYYY format (dashes in positions 2 and 5)", 0, 0, 0};

        int day, month, year;
        auto result1 = std::from_chars(date_str.data(), date_str.data() + 2, day);
        auto result2 = std::from_chars(date_str.data() + 3, date_str.data() + 5, month);
        auto result3 = std::from_chars(date_str.data() + 6, date_str.data() + 10, year);

        if (result1.ec != std::errc() || result2.ec != std::errc() || result3.ec != std::errc())
            return {false, "Invalid numbers in date", 0, 0, 0};

        if (year < 2000 || year > 2100)
            return {false, "Year must be between 2000 and 2100", 0, 0, 0};

        if (month < 1 || month > 12)
            return {false, "Month must be between 01 and 12", 0, 0, 0};

        constexpr std::array<int, 12> days_in_month{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
        int max_days = days_in_month[month - 1];

        if (month == 2)
        {
            bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
            if (is_leap) max_days = 29;
        }

        if (day < 1 || day > max_days)
            return {false, "Invalid day for this month", 0, 0, 0};

        return {true, "", day, month, year};
    }
}

// Redis Connection Manager
class RedisConnection
{
private:
    struct redis_deleter
    {
        void operator()(redisContext* ctx) const
        {
            if (ctx) redisFree(ctx);
        }
    };

    std::unique_ptr<redisContext, redis_deleter> context_;

public:
    RedisConnection()
    {
        auto* raw_ctx = redisConnect("127.0.0.1", 6379);
        if (!raw_ctx || raw_ctx->err)
        {
            std::cout << Color::RED << "❌ Redis connection failed: "
                      << (raw_ctx ? raw_ctx->errstr : "can't allocate context")
                      << Color::RESET << std::endl;
            exit(1);
        }
        context_.reset(raw_ctx);
        std::cout << Color::GREEN << "✅ Connected to Redis!" << Color::RESET << std::endl;
    }

    redisContext* get() const { return context_.get(); }
};

// Todo Repository (All Redis operations happen here)
class TodoRepository
{
private:
    std::unique_ptr<RedisConnection> redis_;
    std::string todo_key_ = "todos";
    std::string counter_key_ = "todo_counter";

    [[nodiscard]] int get_next_id()
    {
        auto* reply = (redisReply*)redisCommand(redis_->get(), "INCR %s", counter_key_.c_str());
        int id = reply->integer;
        freeReplyObject(reply);
        return id;
    }

    void init_counter_if_needed()
    {
        auto* reply = (redisReply*)redisCommand(redis_->get(), "EXISTS %s", counter_key_.c_str());
        if (reply->integer == 0)
        {
            freeReplyObject(reply);
            redisCommand(redis_->get(), "SET %s 0", counter_key_.c_str());
        }
        else
        {
            freeReplyObject(reply);
        }
    }

public:
    TodoRepository() : redis_(std::make_unique<RedisConnection>())
    {
        init_counter_if_needed();
    }

    int add_task(std::string_view title, std::string_view priority = "medium", std::string_view due_date = "")
    {
        int task_id = get_next_id();

        json task_json;
        task_json["id"] = task_id;
        task_json["title"] = title;
        task_json["priority"] = priority;
        task_json["due_date"] = due_date;
        task_json["status"] = "pending";
        task_json["created_at"] = Helper::get_current_time();

        std::string task_str = task_json.dump();
        redisCommand(redis_->get(), "HSET %s %d %s",
                     todo_key_.c_str(), task_id, task_str.c_str());

        return task_id;
    }

    [[nodiscard]] std::vector<Task> get_all_tasks()
    {
        std::vector<Task> tasks;
        auto* reply = (redisReply*)redisCommand(redis_->get(), "HGETALL %s", todo_key_.c_str());

        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (size_t i = 1; i < reply->elements; i += 2)
            {
                auto task_json = json::parse(reply->element[i]->str);
                Task task;
                task.id = task_json["id"];
                task.title = task_json["title"];
                task.priority = task_json["priority"];
                task.due_date = task_json.value("due_date", "");
                task.status = task_json["status"];
                task.created_at = task_json["created_at"];
                task.completed_at = task_json.value("completed_at", "");
                tasks.push_back(std::move(task));
            }
        }

        freeReplyObject(reply);

        const std::map<std::string_view, int> priority_order = {
            {"critical"sv, 5}, {"high"sv, 4}, {"medium"sv, 3}, {"low"sv, 2}
        };

        std::ranges::sort(tasks, [&](const Task& a, const Task& b)
        {
            return priority_order.at(a.priority) > priority_order.at(b.priority);
        });

        return tasks;
    }

    bool complete_task(int task_id)
    {
        auto* reply = (redisReply*)redisCommand(redis_->get(), "HGET %s %d",
                                                 todo_key_.c_str(), task_id);

        if (reply->type != REDIS_REPLY_STRING)
        {
            freeReplyObject(reply);
            return false;
        }

        auto task_json = json::parse(reply->str);
        freeReplyObject(reply);

        if (task_json["status"] == "completed")
        {
            return false;
        }

        task_json["status"] = "completed";
        task_json["completed_at"] = Helper::get_current_time();

        redisCommand(redis_->get(), "HSET %s %d %s",
                     todo_key_.c_str(), task_id, task_json.dump().c_str());
        return true;
    }

    bool delete_task(int task_id)
    {
        auto* reply = (redisReply*)redisCommand(redis_->get(), "HDEL %s %d",
                                                 todo_key_.c_str(), task_id);
        bool deleted = (reply->integer > 0);
        freeReplyObject(reply);
        return deleted;
    }

    void delete_all_tasks()
    {
        redisCommand(redis_->get(), "DEL %s", todo_key_.c_str());
        redisCommand(redis_->get(), "SET %s 0", counter_key_.c_str());
    }

    [[nodiscard]] std::vector<Task> search_tasks(std::string_view keyword)
    {
        auto all_tasks = get_all_tasks();
        std::vector<Task> results;

        for (const auto& task : all_tasks)
        {
            if (task.title.contains(keyword))
            {
                results.emplace_back(task);
            }
        }

        return results;
    }
};

// UI Display Functions
namespace Display
{
    void header()
    {
        std::cout << Color::CYAN << Color::BOLD;
        std::cout << "╔══════════════════════════════════════════════════════════╗\n";
        std::cout << "║            🚀 SMART TODO LIST WITH REDIS 🚀              ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════╝\n";
        std::cout << Color::RESET;
    }

    void menu()
    {
        std::cout << Color::YELLOW << Color::BOLD << "📌 MAIN MENU" << Color::RESET << '\n';
        std::cout << Color::GREEN << "1." << Color::RESET << " ➕ Add Task\n";
        std::cout << Color::GREEN << "2." << Color::RESET << " 📋 List Tasks\n";
        std::cout << Color::GREEN << "3." << Color::RESET << " ✅ Complete Task\n";
        std::cout << Color::GREEN << "4." << Color::RESET << " 🗑️  Delete Task\n";
        std::cout << Color::GREEN << "5." << Color::RESET << " 🔍 Search Tasks\n";
        std::cout << Color::RED   << "C." << Color::RESET << " 🧹 Clear All Tasks\n";
        std::cout << Color::GREEN << "0." << Color::RESET << " 🚪 Exit\n\n";
    }

    void tasks(const std::vector<Task>& task_list)
    {
        if (task_list.empty())
        {
            std::cout << Color::YELLOW << "📭 No tasks found.\n" << Color::RESET;
            return;
        }

        std::cout << Color::CYAN << "──────────────────────────────────────────────────────────\n" << Color::RESET;
        std::cout << Color::BOLD << "ID  Status  Priority    Title\n" << Color::RESET;
        std::cout << Color::CYAN << "──────────────────────────────────────────────────────────\n" << Color::RESET;

        for (const auto& task : task_list)
        {
            std::string status_icon;
            if (task.status == "completed")
                status_icon = std::string(Color::GREEN) + "✅" + std::string(Color::RESET);
            else
                status_icon = std::string(Color::RED) + "⏳" + std::string(Color::RESET);

            std::string_view priority_color = Color::GREEN;
            if (task.priority == "critical") priority_color = Color::RED;
            else if (task.priority == "high") priority_color = Color::YELLOW;
            else if (task.priority == "medium") priority_color = Color::BLUE;

            std::cout << Color::CYAN << "#" << task.id << Color::RESET << "  "
                      << status_icon << "   [" << priority_color << task.priority << Color::RESET << "] "
                      << task.title;

            if (!task.due_date.empty())
            {
                std::cout << " (due: " << task.due_date << ')';
            }
            std::cout << '\n';
        }

        std::cout << Color::CYAN << "──────────────────────────────────────────────────────────\n" << Color::RESET;
    }
}

// Input Handling Functions
namespace Input
{
    std::string get_line(std::string_view prompt)
    {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        return line;
    }

    std::optional<int> get_task_id(std::string_view prompt)
    {
        std::string input = get_line(prompt);
        int value;
        auto result = std::from_chars(input.data(), input.data() + input.size(), value);
        if (result.ec == std::errc() && result.ptr == input.data() + input.size())
        {
            return value;
        }
        std::cout << Color::RED << "❌ Please enter a valid number.\n" << Color::RESET;
        return std::nullopt;
    }

    std::string get_valid_date()
    {
        while (true)
        {
            std::string prompt = std::string(Color::GREEN) + "Due date (DD-MM-YYYY) or press Enter to skip: " + std::string(Color::RESET);
            std::string date = get_line(prompt);

            if (date.empty()) return "";

            auto result = DateValidation::validate(date);
            if (result.valid)
            {
                return date;
            }
            std::cout << Color::RED << "❌ " << result.error_message << '\n' << Color::RESET;
        }
    }
}

// Application Logic
class TodoApp
{
private:
    TodoRepository repository_;

    void handle_add_task()
    {
        std::string prompt = std::string(Color::GREEN) + "Title: " + std::string(Color::RESET);
        std::string title = Input::get_line(prompt);

        if (title.empty())
        {
            std::cout << Color::RED << "❌ Title cannot be empty.\n" << Color::RESET;
            return;
        }

        prompt = std::string(Color::GREEN) + "Priority (low/medium/high/critical) [medium]: " + std::string(Color::RESET);
        std::string priority = Input::get_line(prompt);

        if (priority != "low" && priority != "medium" && priority != "high" && priority != "critical")
        {
            priority = "medium";
        }

        prompt = std::string(Color::GREEN) + "Add due date? (y/n): " + std::string(Color::RESET);
        std::string add_due = Input::get_line(prompt);

        std::string due_date = (add_due == "y" || add_due == "Y") ? Input::get_valid_date() : "";

        int new_id = repository_.add_task(title, priority, due_date);
        std::cout << Color::GREEN << "✅ Task #" << new_id << " added!\n" << Color::RESET;
    }

    void handle_list_tasks()
    {
        auto tasks = repository_.get_all_tasks();
        Display::tasks(tasks);
    }

    void handle_complete_task()
    {
        std::string prompt = std::string(Color::GREEN) + "Enter Task number to complete: " + std::string(Color::RESET);
        auto task_id = Input::get_task_id(prompt);

        if (!task_id) return;

        if (repository_.complete_task(*task_id))
        {
            std::cout << Color::GREEN << "✅ Task #" << *task_id << " completed!\n" << Color::RESET;
        }
        else
        {
            std::cout << Color::RED << "❌ Task not found or already completed.\n" << Color::RESET;
        }
    }

    void handle_delete_task()
    {
        std::string prompt = std::string(Color::GREEN) + "Enter Task number to delete: " + std::string(Color::RESET);
        auto task_id = Input::get_task_id(prompt);

        if (!task_id) return;

        if (repository_.delete_task(*task_id))
        {
            std::cout << Color::GREEN << "✅ Task #" << *task_id << " deleted.\n" << Color::RESET;
        }
        else
        {
            std::cout << Color::RED << "❌ Task not found.\n" << Color::RESET;
        }
    }

    void handle_search_tasks()
    {
        std::string prompt = std::string(Color::GREEN) + "Enter keyword to search: " + std::string(Color::RESET);
        std::string keyword = Input::get_line(prompt);

        auto results = repository_.search_tasks(keyword);

        if (results.empty())
        {
            std::cout << Color::YELLOW << "🔍 No tasks matching '" << keyword << "'.\n" << Color::RESET;
        }
        else
        {
            Display::tasks(results);
        }
    }

    void handle_clear_all_tasks()
    {
        std::string prompt = std::string(Color::RED) + "⚠️  Delete ALL tasks? (y/n): " + std::string(Color::RESET);
        std::string confirm = Input::get_line(prompt);

        if (confirm == "y" || confirm == "Y")
        {
            repository_.delete_all_tasks();
            std::cout << Color::GREEN << "✅ All tasks cleared.\n" << Color::RESET;
        }
    }

public:
    void run()
    {
        Helper::clear_screen();

        while (true)
        {
            Display::header();
            Display::menu();

            std::string prompt = std::string(Color::BOLD) + "👉 Enter your choice: " + std::string(Color::RESET);
            std::string choice = Input::get_line(prompt);

            if (choice == "1") handle_add_task();
            else if (choice == "2") handle_list_tasks();
            else if (choice == "3") handle_complete_task();
            else if (choice == "4") handle_delete_task();
            else if (choice == "5") handle_search_tasks();
            else if (choice == "C" || choice == "c") handle_clear_all_tasks();
            else if (choice == "0") break;
            else std::cout << Color::RED << "❌ Invalid choice.\n" << Color::RESET;

            std::cout << Color::CYAN << "\nPress Enter to continue..." << Color::RESET;
            std::cin.get();
            Helper::clear_screen();
        }

        std::cout << Color::CYAN << "👋 Goodbye! Stay productive!\n" << Color::RESET;
    }
};

int main()
{
    TodoApp app;
    app.run();
    return 0;
}