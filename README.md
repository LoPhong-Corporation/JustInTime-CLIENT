# JustInTime Client

> A lightweight native Windows activity monitoring agent written in C and C++.

![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![Language](https://img.shields.io/badge/Language-C-success)
![Language](https://img.shields.io/badge/Language-C++-success)
![Language](https://img.shields.io/badge/Language-Python-success)
![Compiler](https://img.shields.io/badge/Compiler-MSVC-purple)
![Database](https://img.shields.io/badge/Database-SQLite-orange)
![License](https://img.shields.io/badge/License-MIT-green)

JustInTime Client is a high-performance Windows background agent that monitors user activity, records application usage locally using SQLite, and synchronizes data securely to the cloud through **Supabase Edge Functions**.

The project follows an **offline-first** architecture, ensuring that activity data is never lost even when the device is temporarily offline.

---

## Features

### ✅ Currently Implemented

- Foreground application monitoring
- Active window title detection
- Process name detection
- Activity duration tracking
- SQLite local database
- UTF-16 ⇄ UTF-8 conversion
- Full Unicode support
- Device identification
- JSON serialization
- HTTP communication
- Secure synchronization through Supabase Edge Functions
- Offline queue
- Automatic synchronization
- Unsynced record management

---

## Work In Progress

- Building UI system
- Building API connection between Dashboard and JustInTime Agent

---

## Planned

- Windows Service
- Auto Start
- Background Worker
- Dashboard
- Multi-device synchronization
- Automatic Update
- Installer

---

# Architecture

```text
                     Windows
                        │
                        ▼
          ┌────────────────────────┐
          │ Activity Monitor       │
          └────────────┬───────────┘
                       │
                       ▼
          ┌────────────────────────┐
          │ SQLite Local Database  │
          └────────────┬───────────┘
                       │
                       ▼
          ┌────────────────────────┐
          │ Sync Queue             │
          └────────────┬───────────┘
                       │
                       ▼
          ┌────────────────────────┐
          │ HTTP Client            │
          └────────────┬───────────┘
                       │
                       ▼
          ┌────────────────────────┐
          │ Supabase Edge Function │
          └────────────┬───────────┘
                       │
                       ▼
          ┌────────────────────────┐
          │ PostgreSQL Database    │
          └────────────────────────┘
```

---

# Why Edge Functions?

The client **never communicates directly with the database**.

Instead, every synchronization request is sent to a **Supabase Edge Function**, which is responsible for:

- Request validation
- Authentication
- Data verification
- Business logic
- Database access

This architecture improves security by preventing the client from having direct database access.

---

# Offline First Design

Every activity is stored locally before being synchronized.

```text
Foreground Window

↓

SQLite

↓

Unsynced Queue

↓

HTTP Request

↓

Edge Function

↓

Cloud Database

↓

Mark Record As Synced
```

If the network is unavailable, records remain safely stored in SQLite and will be synchronized automatically once the connection is restored.

---

# Project Structure

```
JustInTime-CLIENT/

├── include/
│   ├── activity.h
│   ├── database.h
│   ├── device.h
│   ├── network.h
│   ├── sync.h
│   └── ...
│
├── src/
│   ├── activity.c
│   ├── database.c
│   ├── device.c
│   ├── network.c
│   ├── sync.c
│   └── main.c
│
├── third_party/
│   └── sqlite/
│
├── docs/
│
├── assets/
│
├── CMakeLists.txt
│
└── README.md
```

---

# Database

Current local database table

| Column | Type |
|---------|------|
| id | INTEGER |
| device_id | TEXT |
| process_name | TEXT |
| window_title | TEXT |
| duration_seconds | INTEGER |
| start_time | INTEGER |
| end_time | INTEGER |
| synced | INTEGER |
| created_at | DATETIME |

---

# Technologies

- C17
- WinAPI
- SQLite3
- WinHTTP
- Supabase Edge Functions
- PostgreSQL
- CMake
- Python 3.13
- Flask

---

# Build

Requirements

- Windows 10 or newer
- Microsoft Visual C++ (MSVC)
- CMake

```bash
git clone https://github.com/LoPhong-Corporation/JustInTime-CLIENT.git

cd JustInTime-CLIENT

mkdir build

cd build

cmake ..

cmake --build .
```

---

# Current Development Status

| Module | Status |
|---------|:------:|
| Activity Monitor | ✅ |
| SQLite Database | ✅ |
| Unicode Support | ✅ |
| Device ID | ✅ |
| JSON Serialization | ✅ |
| HTTP Client | ✅ |
| Sync Engine | ✅ |
| Supabase Edge Functions | ✅ |
| Offline Queue | ✅ |
| Retry Queue | ✅ |
| Configuration | ✅ |
| Windows Service | ⏳ |
| Dashboard | ⏳ |
| Installer | ⏳ |

---

# Roadmap

## Version 0.1

- Activity Monitor
- SQLite
- Unicode
- Local Storage
- HTTP Sync
- Supabase Edge Functions

---

## Version 0.2

- Retry Queue
- Configuration
- Logging
- Error Recovery

---

## Version 0.3

- Windows Service
- Background Worker
- Auto Start

---

## Version 0.4

- Dashboard
- Statistics
- Multi-device Support

---

## Version 1.0

- Stable Release
- Automatic Update
- Installer
- Complete Documentation

---

# Future Ecosystem

```
JustInTime

├── JustInTime-CLIENT
│     Windows Agent
│
├── JustInTime-SERVER
│     REST API
│
├── JustInTime-Dashboard
│     Web Dashboard
│
├── JustInTime-Installer
│
└── Documentation
```

---

# Documentation

Project documentation is available in the GitHub Wiki.

Topics include:

- Architecture
- Activity Monitor
- Database
- Sync Engine
- Network Layer
- Configuration
- Development Guide

---

# Contributing

Contributions, issues, and feature requests are welcome.

Please open an Issue before submitting a Pull Request for large changes.

---

# License

This project is licensed under the MIT License.

See the [LICENSE](LICENSE) file for more information.


**Made with ❤️ by LoPhong Corporation**
