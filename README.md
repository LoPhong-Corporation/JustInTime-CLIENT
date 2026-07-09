# JustInTime Agent

> A lightweight Windows activity monitoring agent with offline synchronization and cloud-ready architecture.

![Platform](https://img.shields.io/badge/Platform-Windows-blue)
![Language](https://img.shields.io/badge/Language-C-success)
![Compiler](https://img.shields.io/badge/Compiler-MSVC-purple)
![Database](https://img.shields.io/badge/Database-SQLite-orange)
![License](https://img.shields.io/badge/License-MIT-green)

---

## 📖 Overview

**JustInTime** is an open-source Windows activity monitoring system designed to track application usage while remaining lightweight, modular, and privacy-friendly.

Unlike traditional monitoring software, JustInTime follows an **offline-first architecture**:

- Activity is collected locally.
- Data is stored in SQLite.
- Records are synchronized only when a network connection is available.
- The backend and dashboard are completely separated from the agent.

The long-term goal is to build a complete productivity platform consisting of:

- Windows Agent
- REST API
- Web Dashboard
- Mobile Application
- AI-powered Productivity Analysis

---

# ✨ Features

## Current

- Active window monitoring
- Process detection
- Window title detection
- Usage duration tracking
- SQLite local storage
- Unicode support (UTF-16 / UTF-8)
- Offline synchronization queue
- Device identification
- Modular architecture

---

## Planned

- WinHTTP client
- HTTPS communication
- Batch synchronization
- Retry mechanism
- Configuration system
- Windows Service
- Auto start
- Crash recovery
- Logging
- Dashboard
- Productivity reports
- AI insights

---

# 🏗 Architecture

```
                 JustInTime

                    Cloud
                      │
      ┌───────────────┼───────────────┐
      │                               │
      ▼                               ▼
 Windows Agent                  Web Dashboard
      │                               │
      └───────────────┬───────────────┘
                      │
                  REST API
                      │
                 PostgreSQL
                      │
                AI Analytics
```

---

# 📂 Project Structure

```
JustInTime
│
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
├── CMakeLists.txt
│
└── README.md
```

---

# ⚙ How It Works

```
Windows

      │
      ▼

Monitor Active Window

      │
      ▼

Collect Activity

      │
      ▼

SQLite Database

      │
      ▼

Unsynced Queue

      │
      ▼

Network Layer

      │
      ▼

REST API

      │
      ▼

PostgreSQL

      │
      ▼

Dashboard
```

---

# 🚀 Roadmap

## Phase 1 — Windows Agent

- [x] Active Window Monitor
- [x] Process Detection
- [x] Usage Tracking
- [x] SQLite Storage
- [x] Device ID
- [x] Unicode Support
- [x] Sync Queue
- [x] Modular Structure
- [ ] WinHTTP
- [ ] HTTPS
- [ ] Batch Upload
- [ ] Retry Logic
- [ ] Config System

---

## Phase 2 — Windows Service

- [ ] Windows Service
- [ ] Auto Start
- [ ] Watchdog
- [ ] Crash Recovery
- [ ] Logging

---

## Phase 3 — Backend

- [ ] FastAPI
- [ ] Authentication
- [ ] Device Registration
- [ ] PostgreSQL
- [ ] Activity API
- [ ] Statistics Engine

---

## Phase 4 — Dashboard

- [ ] Login
- [ ] Timeline
- [ ] Daily Statistics
- [ ] Weekly Statistics
- [ ] Charts
- [ ] Export

---

## Phase 5 — AI

- [ ] Productivity Score
- [ ] Habit Detection
- [ ] Weekly Reports
- [ ] Smart Suggestions

---

# 🛠 Technologies

| Component | Technology |
|----------|------------|
| Language | C (MSVC) |
| Platform | Windows |
| Database | SQLite3 |
| Network | WinHTTP *(planned)* |
| Backend | FastAPI *(planned)* |
| Database Server | PostgreSQL *(planned)* |
| Dashboard | React *(planned)* |

---

# 🎯 Design Goals

JustInTime is designed with several core principles:

- Lightweight
- Offline-first
- Modular
- Unicode-safe
- Low CPU usage
- Low memory footprint
- Easy to maintain
- Easy to extend
- Production-ready architecture

---

# 📌 Development Status

Current version focuses on building a stable Windows Agent.

The backend and dashboard will be developed after the networking layer is completed.

---

# 📜 License

This project is licensed under the MIT License.

---

## ⭐ Contributing

Contributions, ideas, and bug reports are welcome.

If you'd like to contribute:

1. Fork the repository.
2. Create a feature branch.
3. Commit your changes.
4. Open a Pull Request.

---

Made with ❤️ by **LoPhong Corporation**
