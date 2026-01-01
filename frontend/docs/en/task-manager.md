---
outline: deep
description: Task Manager in C Web Framework. Async and scheduled tasks, cron-like scheduling.
---

# Task Manager

Task Manager is a component for executing background tasks. It supports two modes of operation:

1. **Async tasks** — one-time tasks executed in a background thread
2. **Scheduled tasks** — recurring tasks executed according to a schedule

## Architecture

Task Manager creates two dedicated threads:
- **Async worker** — processes one-time async tasks from a queue
- **Scheduler worker** — checks and executes scheduled tasks every second

## Schedule Types

| Type | Description | Example |
|------|-------------|---------|
| `interval` | Runs every N seconds | Every 5 seconds |
| `daily` | Runs every day at a specific time | Every day at 21:55 |
| `weekly` | Runs on a specific day of the week | Every Monday at 09:00 |
| `monthly` | Runs on a specific day of the month | 1st of every month at 03:30 |

## Configuration

Tasks are configured in the `task_manager.schedule` section of `config.json`:

### Interval Task

```json
{
    "task_manager": {
        "schedule": [
            {
                "name": "cleanup_expired_tokens",
                "type": "interval",
                "interval": 5,
                "file": "/path/to/libmy_tasks.so",
                "function": "cleanup_task"
            }
        ]
    }
}
```

### Daily Task

```json
{
    "name": "daily_cleanup",
    "type": "daily",
    "hour": 21,
    "minute": 55,
    "file": "/path/to/libmy_tasks.so",
    "function": "daily_task"
}
```

### Weekly Task

```json
{
    "name": "weekly_report",
    "type": "weekly",
    "weekday": "monday",
    "hour": 9,
    "minute": 0,
    "file": "/path/to/libmy_tasks.so",
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
    "file": "/path/to/libmy_tasks.so",
    "function": "monthly_task"
}
```

## Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `name` | string | Unique task name |
| `type` | string | Schedule type: `interval`, `daily`, `weekly`, `monthly` |
| `interval` | int | Interval in seconds (for `interval` type) |
| `hour` | int | Hour of execution (0-23) |
| `minute` | int | Minute of execution (0-59) |
| `weekday` | string | Day of week (for `weekly` type) |
| `day` | int | Day of month 1-31 (for `monthly` type) |
| `file` | string | Path to shared library (.so) |
| `function` | string | Function name to call |

## Creating Tasks

### Function Signature

Task functions must have the following signature:

```c
void my_task_function(void* data);
```

### Example Task Library

```c
// my_tasks.c
#include <stdio.h>
#include "log.h"

void cleanup_task(void* data) {
    log_info("Running cleanup task\n");
    // Cleanup logic
}

void daily_task(void* data) {
    log_info("Running daily task\n");
    // Daily logic
}

void report_task(void* data) {
    log_info("Generating weekly report\n");
    // Report logic
}
```

### Compilation

```bash
gcc -shared -fPIC -o libmy_tasks.so my_tasks.c
```

## Programmatic API

### Include

```c
#include "taskmanager.h"
```

### Async Tasks

Execute a one-time task in a background thread:

```c
// Simple call
taskmanager_async(my_function, my_data);

// With data free function
taskmanager_async_with_free(my_function, my_data, my_free_function);
```

**Parameters**\
`my_function` — task function.\
`my_data` — data to pass to the function.\
`my_free_function` — function to free data after execution.

**Return value**\
1 on success, 0 on error.

<br>

### Scheduled Tasks

#### Interval Schedule

```c
// Every 60 seconds
taskmanager_schedule(manager, "my_task", 60, my_function);
taskmanager_schedule_with_data(manager, "my_task", 60, my_function, my_data);
```

#### Daily Schedule

```c
// Every day at 09:30
taskmanager_schedule_daily(manager, "daily_task", 9, 30, my_function);
taskmanager_schedule_daily_with_data(manager, "daily_task", 9, 30, my_function, my_data);
```

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

### Task Management

#### Remove Task

```c
int taskmanager_unschedule(taskmanager_t* manager, const char* name);
```

Removes a scheduled task.

**Return value**\
1 on success, 0 if task not found.

<br>

#### Immediate Trigger

```c
int taskmanager_trigger(taskmanager_t* manager, const char* name);
```

Triggers the task immediately without waiting for the schedule.

<br>

#### Enable/Disable

```c
int taskmanager_enable(taskmanager_t* manager, const char* name);
int taskmanager_disable(taskmanager_t* manager, const char* name);
```

Enables or disables task execution without removing it.

<br>

#### Get Task Info

```c
scheduled_task_entry_t* taskmanager_get(taskmanager_t* manager, const char* name);
```

Returns a pointer to the task structure or `NULL` if not found.

## Constants

### Weekdays

```c
typedef enum {
    SUNDAY    = 0,
    MONDAY    = 1,
    TUESDAY   = 2,
    WEDNESDAY = 3,
    THURSDAY  = 4,
    FRIDAY    = 5,
    SATURDAY  = 6
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

## Usage Examples

### Cleanup Expired Sessions

```c
void cleanup_sessions(void* data) {
    dbresult_t* result = db_query(
        "postgresql.p1",
        "DELETE FROM sessions WHERE expires_at < NOW()",
        NULL
    );

    if (result) {
        log_info("Deleted %d expired sessions\n", db_result_affected_rows(result));
        db_result_free(result);
    }
}

// Register: every 5 minutes
taskmanager_schedule(manager, "cleanup_sessions", 300, cleanup_sessions);
```

### Send Daily Report

```c
void send_daily_report(void* data) {
    // Generate report
    str_t* report = generate_daily_report();

    // Send via email
    mail_send((mail_t){
        .to = "admin@example.com",
        .subject = "Daily Report",
        .body = str_get(report)
    });

    str_free(report);
}

// Register: every day at 08:00
taskmanager_schedule_daily(manager, "daily_report", 8, 0, send_daily_report);
```

### Weekly Backup

```c
void weekly_backup(void* data) {
    log_info("Starting weekly backup\n");

    // Perform backup
    system("pg_dump mydb > /backups/weekly_backup.sql");

    log_info("Backup completed\n");
}

// Register: every Sunday at 03:00
taskmanager_schedule_weekly(manager, "weekly_backup", SUNDAY, 3, 0, weekly_backup);
```

## Task Structure

```c
typedef struct scheduled_task_entry {
    char            name[128];       // Task name
    task_fn_t       run;             // Execution function
    void*           data;            // Data
    int             interval;        // Interval (for SCHEDULE_INTERVAL)
    time_t          last_run;        // Last run time
    time_t          next_run;        // Next run time
    short           enabled;         // Whether task is enabled
    schedule_type_e schedule_type;   // Schedule type
    int             schedule_day;    // Day (of week or month)
    int             schedule_hour;   // Hour
    int             schedule_min;    // Minute
} scheduled_task_entry_t;
```
