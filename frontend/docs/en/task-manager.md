---
outline: deep
description: Task Manager in C Web Framework. Async and scheduled tasks, cron-like scheduling.
---

# Task Manager

Task Manager is a component for executing background tasks. It supports two modes of operation:

1. **Async tasks** — one-time tasks executed in a dedicated background thread
2. **Scheduled tasks** — recurring tasks executed according to a schedule

## Architecture

Task Manager creates two dedicated threads:

- **Async worker** — pops one-time async tasks from a queue and runs them. The thread waits for tasks on a condition variable with a 1-second timeout.
- **Scheduler worker** — every second, checks scheduled tasks and fires any whose `next_run` time has arrived.

Both threads are fully **thread-safe**: access to the async queue is guarded by `async_mutex` (plus condition variable `async_cond`), and access to the scheduled-task list is guarded by `scheduler_mutex`. Tasks run **sequentially** within their own thread — a long-running task blocks the others in the same thread.

## Schedule Types

| Type | Description | Fields | Example |
|------|-------------|---------|---------|
| `interval` | Runs every N seconds | `interval` | Every 5 seconds |
| `daily` | Runs every day at a specific time | `hour`, `minute` | Every day at 21:55 |
| `weekly` | Runs on a specific day of the week | `weekday`, `hour`, `minute` | Every Monday at 09:00 |
| `monthly` | Runs on a specific day of the month | `day`, `hour`, `minute` | 1st of every month at 03:30 |

## Configuration

Tasks are configured in the `task_manager` section of `config.json` — an array of objects. Four common fields are required for **every** type:

| Field | Type | Description |
|-------|------|-------------|
| `name` | string | Unique task name (duplicates are rejected) |
| `type` | string | Schedule type: `interval`, `daily`, `weekly`, `monthly` |
| `file` | string | Path to the shared library (`.so`) |
| `function` | string | Name of the function to call in the library |

The remaining fields depend on `type` and are described below. The same task can also be registered programmatically via the API — see [Programmatic API](#programmatic-api).

### Interval Task

```json
"task_manager": [
    {
        "name": "cleanup_expired_tokens",
        "type": "interval",
        "interval": 60,
        "file": "/app/build/exec/handlers/tasks/libtasks.so",
        "function": "cleanup_authorization_codes"
    }
]
```

`interval` — integer number of seconds, **≥ 1**.

### Daily Task

```json
{
    "name": "nightly_report",
    "type": "daily",
    "hour": 3,
    "minute": 30,
    "file": "/app/build/exec/handlers/tasks/libtasks.so",
    "function": "send_report"
}
```

`hour` — 0–23, `minute` — 0–59.

### Weekly Task

```json
{
    "name": "weekly_report",
    "type": "weekly",
    "weekday": "monday",
    "hour": 9,
    "minute": 0,
    "file": "/app/build/exec/handlers/tasks/libtasks.so",
    "function": "report_task"
}
```

Available `weekday` values: `sunday`, `monday`, `tuesday`, `wednesday`, `thursday`, `friday`, `saturday`.

### Monthly Task

```json
{
    "name": "monthly_cleanup",
    "type": "monthly",
    "day": 1,
    "hour": 3,
    "minute": 30,
    "file": "/app/build/exec/handlers/tasks/libtasks.so",
    "function": "monthly_task"
}
```

`day` — 1–31. If a month has no such day (e.g. `30` in February), the task is skipped that month and rescheduled for the next month that contains it.

## Creating Tasks

### Function Signature

Task functions must have the following signature:

```c
void my_task_function(void* data);
```

The `data` parameter is an arbitrary pointer passed in when the task is registered (via the `*_with_data` API variants, or `NULL` when no data is needed).

### Example Task Library

```c
// tasks.c
#include "log.h"

void cleanup_authorization_codes(void* data) {
    log_info("Cleaning up expired authorization codes\n");
    // Cleanup logic...
}

void send_report(void* data) {
    log_info("Sending nightly report\n");
    // Build and send the report...
}

void report_task(void* data) {
    log_info("Generating weekly report\n");
    // Report logic...
}
```

### Compilation

```bash
gcc -shared -fPIC -o libtasks.so tasks.c
```

The library links against the same `core` as the server, so the full framework API (`log.h`, `dbquery.h`, `mail.h`, etc.) is available.

## Programmatic API

### Include

```c
#include "taskmanager.h"
```

### Lifecycle

The server creates and frees the Task Manager automatically on startup/shutdown — you do not call these functions from within tasks.

```c
taskmanager_t* taskmanager_init(void);
void taskmanager_free(taskmanager_t* manager);
int taskmanager_create_threads(appconfig_t* config);
```

`taskmanager_create_threads` spawns both worker threads (async + scheduler) and returns 1 on success, 0 on failure to create a thread.

### Async Tasks

Execute a one-time task in a background thread:

```c
// Simple call
taskmanager_async(my_function, my_data);

// With a data free function
taskmanager_async_with_free(my_function, my_data, my_free_function);
```

**Parameters**\
`run` — task function.\
`data` — data to pass to the function.\
`free_fn` — function to free `data`, called after the task runs (or `NULL`).

**Return value**\
1 on success, 0 on error (manager not initialized, `run == NULL`, or out of memory).

<br>

### Scheduled Tasks

All registration functions return 1 on success and 0 on error (invalid argument, out of memory, or a task with that name **already exists**). The next run time is computed automatically at registration.

#### Interval Schedule

```c
// Every 60 seconds
taskmanager_schedule(manager, "my_task", 60, my_function);
taskmanager_schedule_with_data(manager, "my_task", 60, my_function, my_data);
```

`interval` — integer number of seconds, **≥ 1**.

#### Daily Schedule

```c
// Every day at 09:30
taskmanager_schedule_daily(manager, "daily_task", 9, 30, my_function);
taskmanager_schedule_daily_with_data(manager, "daily_task", 9, 30, my_function, my_data);
```

`hour` — 0–23, `minute` — 0–59.

#### Weekly Schedule

```c
// Every Monday at 10:00
taskmanager_schedule_weekly(manager, "weekly_task", MONDAY, 10, 0, my_function);
taskmanager_schedule_weekly_with_data(manager, "weekly_task", MONDAY, 10, 0, my_function, my_data);
```

#### Monthly Schedule

```c
// 15th of every month at 12:00
taskmanager_schedule_monthly(manager, "monthly_task", 15, 12, 0, my_function);
taskmanager_schedule_monthly_with_data(manager, "monthly_task", 15, 12, 0, my_function, my_data);
```

`day` — 1–31.

### Task Management

All functions return 1 on success, 0 if no task with that name is found.

#### Remove Task

```c
int taskmanager_unschedule(taskmanager_t* manager, const char* name);
```

Removes a scheduled task from the list entirely.

<br>

#### Immediate Trigger

```c
int taskmanager_trigger(taskmanager_t* manager, const char* name);
```

Triggers the task immediately without waiting for the schedule: it sets `next_run` to the past, so the task runs on the scheduler's next tick (within a second).

<br>

#### Enable/Disable

```c
int taskmanager_enable(taskmanager_t* manager, const char* name);
int taskmanager_disable(taskmanager_t* manager, const char* name);
```

Enables or disables a task without removing it. A disabled task stays in the list but is not fired by the scheduler.

<br>

#### Get Task Info

```c
scheduled_task_entry_t* taskmanager_get(taskmanager_t* manager, const char* name);
```

Returns a pointer to the task structure, or `NULL` if not found. The structure is protected by the scheduler mutex during lookup; modify fields deliberately.

<br>

### Time Calculation Functions

Compute `next_run` for a given schedule type. `base_time = 0` means "use the current time" — useful for testing and external use.

```c
time_t taskmanager_calc_next_daily(time_t base_time, int hour, int minute);
time_t taskmanager_calc_next_weekly(time_t base_time, int weekday, int hour, int minute);
time_t taskmanager_calc_next_monthly(time_t base_time, int day, int hour, int minute);
```

## Constants

### Schedule Type

```c
typedef enum {
    SCHEDULE_INTERVAL = 0,  // every N seconds
    SCHEDULE_DAILY,         // every day at a specific time
    SCHEDULE_WEEKLY,        // by day of week
    SCHEDULE_MONTHLY        // by day of month
} schedule_type_e;
```

### Weekdays

```c
typedef enum {
    MONDAY    = 1,
    TUESDAY   = 2,
    WEDNESDAY = 3,
    THURSDAY  = 4,
    FRIDAY    = 5,
    SATURDAY  = 6,
    SUNDAY    = 0
} weekday_e;
```

### Predefined Intervals

```c
typedef enum {
    INTERVAL_SECOND  = 1,
    INTERVAL_MINUTE  = 60,
    INTERVAL_HOURLY  = 3600,
    INTERVAL_DAILY   = 86400,
    INTERVAL_WEEKLY  = 604800
} task_interval_e;
```

### Async Task Statuses

```c
typedef enum {
    TASK_STATUS_PENDING   = 0,  // queued, waiting to run
    TASK_STATUS_RUNNING,        // running
    TASK_STATUS_COMPLETED,      // finished
    TASK_STATUS_FAILED          // error (reserved)
} task_status_e;
```

## Structures

### Scheduled Task

```c
typedef struct scheduled_task_entry {
    char            name[128];       // Task name
    task_fn_t       run;             // Execution function
    void*           data;            // Data passed to run
    int             interval;        // Interval in seconds (for SCHEDULE_INTERVAL)
    time_t          last_run;        // Last run time
    time_t          next_run;        // Next run time
    short           enabled;         // Whether the task is enabled (1/0)
    schedule_type_e schedule_type;   // Schedule type
    int             schedule_day;    // Day of week (0-6) or day of month (1-31)
    int             schedule_hour;   // Hour (0-23)
    int             schedule_min;    // Minute (0-59)

    struct scheduled_task_entry* next;  // Next task in the list
} scheduled_task_entry_t;
```

### Async Task

```c
typedef struct task {
    task_fn_t      run;        // Execution function
    task_free_fn_t free_data;  // Function to free data (or NULL)
    void*          data;       // Data passed to run
    task_status_e  status;     // Current status
} task_t;
```

## Usage Examples

### Cleanup Expired Sessions

```c
#include "log.h"
#include "dbquery.h"

void cleanup_sessions(void* data) {
    dbresult_t* result = dbquery(
        "postgresql.p1",
        "DELETE FROM sessions WHERE expires_at < NOW()",
        NULL
    );

    if (dbresult_ok(result)) {
        log_info("Expired sessions cleaned up\n");
    }

    dbresult_free(result);
}

// Register: every 5 minutes
taskmanager_schedule(manager, "cleanup_sessions", 300, cleanup_sessions);
```

### Send Daily Report

```c
#include "log.h"
#include "str.h"
#include "mail.h"

void send_daily_report(void* data) {
    str_t* report = generate_daily_report();   // your function

    mail_payload_t payload = {
        .from    = "noreply@example.com",
        .to      = "admin@example.com",
        .subject = "Daily Report",
        .body    = str_get(report)
    };

    send_mail(&payload);

    str_free(report);
}

// Register: every day at 08:00
taskmanager_schedule_daily(manager, "daily_report", 8, 0, send_daily_report);
```

### Weekly Backup

```c
#include "log.h"

void weekly_backup(void* data) {
    log_info("Starting weekly backup\n");
    system("pg_dump mydb > /backups/weekly_backup.sql");
    log_info("Backup completed\n");
}

// Register: every Sunday at 03:00
taskmanager_schedule_weekly(manager, "weekly_backup", SUNDAY, 3, 0, weekly_backup);
```
