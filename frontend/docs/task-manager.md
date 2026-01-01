---
outline: deep
description: Планировщик задач в C Web Framework. Асинхронные и запланированные задачи, cron-подобное расписание.
---

# Планировщик задач

Планировщик задач (Task Manager) — компонент для выполнения фоновых задач. Поддерживает два режима работы:

1. **Асинхронные задачи** — одноразовые задачи в фоновом потоке
2. **Запланированные задачи** — повторяющиеся задачи по расписанию

## Архитектура

Task Manager создаёт два выделенных потока:
- **Async worker** — обрабатывает одноразовые асинхронные задачи из очереди
- **Scheduler worker** — проверяет и выполняет запланированные задачи каждую секунду

## Типы расписания

| Тип | Описание | Пример |
|-----|----------|--------|
| `interval` | Запуск каждые N секунд | Каждые 5 секунд |
| `daily` | Запуск каждый день в определённое время | Каждый день в 21:55 |
| `weekly` | Запуск в определённый день недели | Каждый понедельник в 09:00 |
| `monthly` | Запуск в определённый день месяца | 1-го числа каждого месяца в 03:30 |

## Конфигурация

Задачи настраиваются в секции `task_manager.schedule` файла `config.json`:

### Интервальная задача

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

### Ежедневная задача

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

### Еженедельная задача

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

Доступные значения `weekday`: `sunday`, `monday`, `tuesday`, `wednesday`, `thursday`, `friday`, `saturday`.

### Ежемесячная задача

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

## Параметры конфигурации

| Параметр | Тип | Описание |
|----------|-----|----------|
| `name` | string | Уникальное имя задачи |
| `type` | string | Тип расписания: `interval`, `daily`, `weekly`, `monthly` |
| `interval` | int | Интервал в секундах (для типа `interval`) |
| `hour` | int | Час запуска (0-23) |
| `minute` | int | Минута запуска (0-59) |
| `weekday` | string | День недели (для типа `weekly`) |
| `day` | int | День месяца 1-31 (для типа `monthly`) |
| `file` | string | Путь к разделяемой библиотеке (.so) |
| `function` | string | Имя вызываемой функции |

## Создание задач

### Сигнатура функции

Функции задач должны иметь следующую сигнатуру:

```c
void my_task_function(void* data);
```

### Пример библиотеки задач

```c
// my_tasks.c
#include <stdio.h>
#include "log.h"

void cleanup_task(void* data) {
    log_info("Выполняется задача очистки\n");
    // Логика очистки
}

void daily_task(void* data) {
    log_info("Выполняется ежедневная задача\n");
    // Ежедневная логика
}

void report_task(void* data) {
    log_info("Генерация еженедельного отчёта\n");
    // Логика отчёта
}
```

### Компиляция

```bash
gcc -shared -fPIC -o libmy_tasks.so my_tasks.c
```

## Программный API

### Подключение

```c
#include "taskmanager.h"
```

### Асинхронные задачи

Выполнение одноразовой задачи в фоновом потоке:

```c
// Простой вызов
taskmanager_async(my_function, my_data);

// С функцией освобождения данных
taskmanager_async_with_free(my_function, my_data, my_free_function);
```

**Параметры**\
`my_function` — функция задачи.\
`my_data` — данные для передачи в функцию.\
`my_free_function` — функция для освобождения данных после выполнения.

**Возвращаемое значение**\
1 при успехе, 0 при ошибке.

<br>

### Запланированные задачи

#### Интервальное расписание

```c
// Каждые 60 секунд
taskmanager_schedule(manager, "my_task", 60, my_function);
taskmanager_schedule_with_data(manager, "my_task", 60, my_function, my_data);
```

#### Ежедневное расписание

```c
// Каждый день в 09:30
taskmanager_schedule_daily(manager, "daily_task", 9, 30, my_function);
taskmanager_schedule_daily_with_data(manager, "daily_task", 9, 30, my_function, my_data);
```

#### Еженедельное расписание

```c
// Каждый понедельник в 10:00
taskmanager_schedule_weekly(manager, "weekly_task", MONDAY, 10, 0, my_function);
taskmanager_schedule_weekly_with_data(manager, "weekly_task", MONDAY, 10, 0, my_function, my_data);
```

#### Ежемесячное расписание

```c
// 15-го числа каждого месяца в 12:00
taskmanager_schedule_monthly(manager, "monthly_task", 15, 12, 0, my_function);
taskmanager_schedule_monthly_with_data(manager, "monthly_task", 15, 12, 0, my_function, my_data);
```

### Управление задачами

#### Удаление задачи

```c
int taskmanager_unschedule(taskmanager_t* manager, const char* name);
```

Удаляет запланированную задачу.

**Возвращаемое значение**\
1 при успехе, 0 если задача не найдена.

<br>

#### Немедленный запуск

```c
int taskmanager_trigger(taskmanager_t* manager, const char* name);
```

Запускает задачу немедленно, не дожидаясь расписания.

<br>

#### Включение/выключение

```c
int taskmanager_enable(taskmanager_t* manager, const char* name);
int taskmanager_disable(taskmanager_t* manager, const char* name);
```

Включает или выключает выполнение задачи без её удаления.

<br>

#### Получение информации

```c
scheduled_task_entry_t* taskmanager_get(taskmanager_t* manager, const char* name);
```

Возвращает указатель на структуру задачи или `NULL`, если задача не найдена.

## Константы

### Дни недели

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

### Предопределённые интервалы

```c
typedef enum {
    INTERVAL_SECOND  = 1,
    INTERVAL_MINUTE  = 60,
    INTERVAL_HOURLY  = 3600,
    INTERVAL_DAILY   = 86400,
    INTERVAL_WEEKLY  = 604800
} task_interval_e;
```

## Примеры использования

### Очистка устаревших сессий

```c
void cleanup_sessions(void* data) {
    dbresult_t* result = db_query(
        "postgresql.p1",
        "DELETE FROM sessions WHERE expires_at < NOW()",
        NULL
    );

    if (result) {
        log_info("Удалено %d устаревших сессий\n", db_result_affected_rows(result));
        db_result_free(result);
    }
}

// Регистрация: каждые 5 минут
taskmanager_schedule(manager, "cleanup_sessions", 300, cleanup_sessions);
```

### Отправка ежедневного отчёта

```c
void send_daily_report(void* data) {
    // Формирование отчёта
    str_t* report = generate_daily_report();

    // Отправка по email
    mail_send((mail_t){
        .to = "admin@example.com",
        .subject = "Ежедневный отчёт",
        .body = str_get(report)
    });

    str_free(report);
}

// Регистрация: каждый день в 08:00
taskmanager_schedule_daily(manager, "daily_report", 8, 0, send_daily_report);
```

### Еженедельный бэкап

```c
void weekly_backup(void* data) {
    log_info("Запуск еженедельного бэкапа\n");

    // Выполнение бэкапа
    system("pg_dump mydb > /backups/weekly_backup.sql");

    log_info("Бэкап завершён\n");
}

// Регистрация: каждое воскресенье в 03:00
taskmanager_schedule_weekly(manager, "weekly_backup", SUNDAY, 3, 0, weekly_backup);
```

## Структура задачи

```c
typedef struct scheduled_task_entry {
    char            name[128];       // Имя задачи
    task_fn_t       run;             // Функция выполнения
    void*           data;            // Данные
    int             interval;        // Интервал (для SCHEDULE_INTERVAL)
    time_t          last_run;        // Время последнего запуска
    time_t          next_run;        // Время следующего запуска
    short           enabled;         // Включена ли задача
    schedule_type_e schedule_type;   // Тип расписания
    int             schedule_day;    // День (недели или месяца)
    int             schedule_hour;   // Час
    int             schedule_min;    // Минута
} scheduled_task_entry_t;
```
