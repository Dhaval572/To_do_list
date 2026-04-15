# ✅ Todo List with Redis – C++23 Terminal App

A todo list application with Redis backend and clean terminal interface. Built with modern C++23.

---

## ✨ Features

- Persistent storage in Redis
- Sequential task IDs (1, 2, 3...)
- Priority levels: critical, high, medium, low
- Due date support (DD-MM-YYYY format)
- Search tasks by keyword
- Colored terminal output
- Redis database viewer script

---

## 📦 Dependencies

| Dependency | Purpose |
|------------|---------|
| Redis | Database server |
| hiredis | Redis client library for C++ |
| C++23 | Core language |
| Ninja | Build system |
| CMake | Build configuration |

No extra dependencies – just these.

---

## 🚀 Quick Start

### Install Dependencies

```bash
# Fedora
sudo dnf install redis hiredis hiredis-devel cmake ninja-build

# Ubuntu
sudo apt install redis-server libhiredis-dev cmake ninja-build
```

### Build & Run

```bash
# Start Redis
redis-server --daemonize yes

# Build
chmod +x build.sh
./build.sh

# Run app
./build/todo_app

# View database (another terminal)
./redis_view.sh
```

---

## 📖 Usage

### Main Menu

```
1. ➕ Add Task
2. 📋 List Tasks
3. ✅ Complete Task
4. 🗑️ Delete Task
5. 🔍 Search Tasks
C. 🧹 Clear All Tasks
0. 🚪 Exit
```

### Example

```
Choice: 1
Title: Buy groceries
Priority [medium]: low
Add due date? (y/n): y
Due date (DD-MM-YYYY): 20-04-2025
✅ Task #1 added!

Choice: 2

ID  Status  Priority    Title
#1   ⏳   [     low] Buy groceries (due: 20-04-2025)
```

---

## 📊 Redis Viewer

View your database in readable format:

```bash
./redis_view.sh
```

Output:
```
╔══════════════════════════════════════════════════════════╗
║              TODO LIST REDIS DATABASE VIEWER             ║
╚══════════════════════════════════════════════════════════╝

📊 DATABASE INFO
──────────────────────────────────────────────────────────
● Redis Keys:     2
● Memory Used:    1.2M

📋 ALL TASKS
──────────────────────────────────────────────────────────

┌────────────────────────────────────────────────────────┐
│ Task #1                                                │
├────────────────────────────────────────────────────────┤
│ Title:     Buy groceries                               │
│ Priority:  low                                         │
│ Status:    ⏳ pending                                  │
│ Due Date:  20-04-2025                                  │
│ Created:   15-04-2026 10:30:15                         │
└────────────────────────────────────────────────────────┘
```

---

## 🏗️ Architecture

```
main() → TodoApp → TodoRepository → RedisConnection → Redis
                      ↓
                  hiredis library
```

### Namespaces

- Color – ANSI codes
- Helper – Time, screen clear
- DateValidation – Date checking
- Display – UI rendering
- Input – User input

---

## 📁 Project Structure

```
to_do_list/
├── main.cpp          # Application code
├── CMakeLists.txt    # Build config
├── build.sh          # Build script
├── redis_view.sh     # Database viewer
├── json.hpp          # JSON library
├── dump.rdb          # Redis data (auto)
└── README.md         # This file
```

---

## 🔧 Build Configuration

- Compiler: clang++
- Standard: C++23
- Optimization: -O2
- Build system: Ninja

---

## 📝 Date Format

Use DD-MM-YYYY

- 15-04-2025 ✓ Valid
- 31-04-2025 ✗ April has 30 days
- 29-02-2024 ✓ Leap year
- 15/04/2025 ✗ Wrong separator

---

## 🧪 Testing

```bash
# Run app
./build/todo_app

# View data
redis-cli HGETALL todos
./redis_view.sh

# Check Redis persistence
ls -lh dump.rdb
```

---