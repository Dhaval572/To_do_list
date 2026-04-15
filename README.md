```markdown
# ✅ Smart Todo List with Redis – C++23 Terminal App

A **feature‑rich, production‑ready** todo list application with Redis backend, beautiful terminal UI, and smart validation. Built with modern C++23 for high performance and readability.

---

## ✨ Features

- **Persistent Storage** – All tasks stored in Redis (survives app restarts)
- **Sequential Task IDs** – Simple 1, 2, 3 numbering (no UUIDs)
- **Priority Levels** – critical, high, medium, low
- **Due Date Support** – DD‑MM‑YYYY format with full validation (leap years, month days)
- **Search Tasks** – Find tasks by keyword
- **Beautiful Terminal UI** – Colored output, clean tables, intuitive menus
- **Exception‑Free** – Uses `std::from_chars` for fast, safe parsing
- **C++23 Standard** – Modern features without compiler warnings

---

## 🚀 Quick Start

### Prerequisites

| Dependency | Installation Command |
|------------|---------------------|
| **Redis** | `sudo dnf install redis` (Fedora) / `sudo apt install redis-server` (Ubuntu) |
| **hiredis** | `sudo dnf install hiredis hiredis-devel` |
| **CMake** | `sudo dnf install cmake` |
| **Ninja** | `sudo dnf install ninja-build` |
| **nlohmann-json** | `sudo dnf install nlohmann-json-devel` |

### Build & Run

```bash
# 1. Start Redis
redis-server --daemonize yes

# 2. Clone / navigate to project
cd /path/to/to_do_list

# 3. Make build script executable
chmod +x build.sh

# 4. Build the app (Release mode with -O2)
./build.sh

# 5. Run the app
./build/todo_app
```

---

## 📖 Usage Guide

### Main Menu

```
╔══════════════════════════════════════════════════════════╗
║            🚀 SMART TODO LIST WITH REDIS 🚀              ║
╚══════════════════════════════════════════════════════════╝

📌 MAIN MENU
1. ➕ Add Task
2. 📋 List Tasks
3. ✅ Complete Task
4. 🗑️  Delete Task
5. 🔍 Search Tasks
C. 🧹 Clear All Tasks
0. 🚪 Exit
```

### Adding a Task

```
👉 Enter your choice: 1
Title: Complete project report
Priority (low/medium/high/critical) [medium]: high
Add due date? (y/n): y
Due date (DD-MM-YYYY) or press Enter to skip: 25-12-2025
✅ Task #1 added!
```

### Listing Tasks

```
👉 Enter your choice: 2

──────────────────────────────────────────────────────────
ID  Status  Priority    Title
──────────────────────────────────────────────────────────
#1   ⏳   [     high] Complete project report (due: 25-12-2025)
──────────────────────────────────────────────────────────
```

### Completing a Task

```
👉 Enter your choice: 3
Enter Task number to complete: 1
✅ Task #1 completed!
```

### Searching Tasks

```
👉 Enter your choice: 5
Enter keyword to search: report
──────────────────────────────────────────────────────────
ID  Status  Priority    Title
──────────────────────────────────────────────────────────
#1   ✅   [     high] Complete project report (due: 25-12-2025)
──────────────────────────────────────────────────────────
```

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      main()                              │
│                 Creates TodoApp                          │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                    TodoApp                                │
│         • Handles user input & menu flow                  │
│         • Calls repository for data operations            │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                 TodoRepository                            │
│         • All Redis commands (HSET, HGET, HDEL)           │
│         • JSON serialization/deserialization              │
│         • Task ID counter management                      │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│               RedisConnection (RAII)                      │
│         • Manages connection lifecycle                    │
│         • Smart pointer with custom deleter               │
└─────────────────────────────────────────────────────────┘
```

### Namespace Organization

| Namespace | Purpose |
|-----------|---------|
| `Color` | ANSI color codes for terminal output |
| `Helper` | Utility functions (time, screen clear) |
| `DateValidation` | DD-MM-YYYY validation with leap year support |
| `Display` | UI rendering (header, menu, task table) |
| `Input` | User input with safe parsing (`std::from_chars`) |

---

## 🧠 Technical Highlights

### Redis Data Structure

```
Key: "todos" (Hash)
├── "1" → {"id":1,"title":"Task","priority":"high",...}
├── "2" → {"id":2,"title":"Another","priority":"medium",...}
└── ...

Key: "todo_counter" (String) → "2"
```

### Date Validation (Exception‑Free)

```cpp
auto result = DateValidation::validate("31-04-2025");
// result.valid = false
// result.error_message = "Invalid day for this month"
```

### Safe Integer Parsing

```cpp
int value;
auto result = std::from_chars(input.data(), input.data() + input.size(), value);
if (result.ec == std::errc()) { /* valid number */ }
```

### Priority Sorting

```cpp
const std::map<std::string, int> priority_order = {
    {"critical", 5}, {"high", 4}, {"medium", 3}, {"low", 2}
};
std::ranges::sort(tasks, [&](const Task& a, const Task& b) {
    return priority_order.at(a.priority) > priority_order.at(b.priority);
});
```

---

## 📁 Project Structure

```
to_do_list/
├── main.cpp              # Complete application (single file)
├── CMakeLists.txt        # CMake configuration
├── build.sh              # Release build script
├── json.hpp              # nlohmann/json (header‑only)
└── README.md             # This documentation
```

---

## 🔧 Build Configuration

| Flag | Purpose |
|------|---------|
| `-O2` | High performance optimization |
| `-march=native` | CPU‑specific optimizations |
| `-flto=auto` | Link Time Optimization |
| `-DNDEBUG` | Removes debug assertions |
| `-std=c++23` | Modern C++ features |

---

## 🧪 Testing

```bash
# Add sample tasks
./build/todo_app
# Follow menu to add, list, complete, search

# Verify Redis persistence
redis-cli
127.0.0.1:6379> HGETALL todos
127.0.0.1:6379> GET todo_counter
```

---

## 📝 Date Format Rules

| Input | Validation |
|-------|------------|
| `15-04-2025` | ✅ Valid |
| `31-01-2025` | ✅ Valid |
| `29-02-2024` | ✅ Valid (leap year) |
| `29-02-2025` | ❌ 2025 is not leap year |
| `31-04-2025` | ❌ April has 30 days |
| `15/04/2025` | ❌ Wrong separator |
| `15-13-2025` | ❌ Month > 12 |
| `32-01-2025` | ❌ Day > 31 |

---

## 🎓 Why This Project Stands Out

| Criteria | Implementation |
|----------|----------------|
| **Redis Integration** | Full use of hashes, counters, atomic operations |
| **Modern C++23** | `std::from_chars`, `std::ranges::sort`, `std::optional` |
| **Exception‑Free** | No try/catch overhead, uses return values |
| **RAII** | Smart pointers with custom deleters |
| **Clean Architecture** | Separation of concerns (Repository, UI, Input) |
| **User Experience** | Colored output, intuitive menus, validation feedback |
| **Production Ready** | Release build with `-O2`, Redis persistence |

---