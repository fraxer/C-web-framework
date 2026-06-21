---
outline: deep
description: Планировщик задач в C Web Framework. Асинхронные и запланированные задачи, cron-подобное расписание.
---

# Планировщик задач

Планировщик задач (Task Manager) — компонент для выполнения фоновых задач. Поддерживает два режима работы:

1. **Асинхронные задачи** — одноразовые задачи, выполняемые в выделенном фоновом потоке
2. **Запланированные задачи** — повторяющиеся задачи, выполняемые по расписанию

## Архитектура

Task Manager создаёт два выделенных потока:

- **Async worker** — извлекает одноразовые асинхронные задачи из очереди и выполняет их. Поток ожидает появления задач на условной переменной с таймаутом 1 секунда.
- **Scheduler worker** — каждую секунду проверяет запланированные задачи и запускает те, у которых наступило время `next_run`.

Оба потока полностью **потокобезопасны**: доступ к очереди асинхронных задач защищён `async_mutex` (+ условная переменная `async_cond`), доступ к списку запланированных задач — `scheduler_mutex`. Задачи выполняются **последовательно** в своём потоке — длительная задача блокирует остальные в том же потоке.

## Типы расписания

| Тип | Описание | Поля | Пример |
|-----|----------|------|--------|
| `interval` | Запуск каждые N секунд | `interval` | Каждые 5 секунд |
| `daily` | Запуск каждый день в указанное время | `hour`, `minute` | Каждый день в 21:55 |
| `weekly` | Запуск в определённый день недели | `weekday`, `hour`, `minute` | Каждый понедельник в 09:00 |
| `monthly` | Запуск в определённый день месяца | `day`, `hour`, `minute` | 1-го числа каждого месяца в 03:30 |

## Конфигурация

Задачи настраиваются в секции `task_manager` файла `config.json` — это массив объектов. Для **любого** типа обязательны четыре общих поля:

| Поле | Тип | Описание |
|------|-----|----------|
| `name` | string | Уникальное имя задачи (дубликаты отклоняются) |
| `type` | string | Тип расписания: `interval`, `daily`, `weekly`, `monthly` |
| `file` | string | Путь к разделяемой библиотеке (`.so`) |
| `function` | string | Имя вызываемой функции в библиотеке |

Остальные поля зависят от `type` и описаны ниже. Та же задача может быть зарегистрирована программно через API — см. [Программный API](#программный-api).

### Интервальная задача

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

`interval` — целое число секунд, **≥ 1**.

### Ежедневная задача

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

### Еженедельная задача

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

Доступные значения `weekday`: `sunday`, `monday`, `tuesday`, `wednesday`, `thursday`, `friday`, `saturday`.

### Ежемесячная задача

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

`day` — 1–31. Если в месяце нет указанного дня (например, `30` в феврале), задача в этом месяце не выполняется и переносится на следующий подходящий месяц.

## Создание задач

### Сигнатура функции

Функции задач должны иметь следующую сигнатуру:

```c
void my_task_function(void* data);
```

Параметр `data` — произвольный указатель, переданный при регистрации задачи (через `*_with_data`-варианты API или `NULL`, если данные не нужны).

### Пример библиотеки задач

```c
// tasks.c
#include "log.h"

void cleanup_authorization_codes(void* data) {
    log_info("Очистка устаревших кодов авторизации\n");
    // Логика очистки...
}

void send_report(void* data) {
    log_info("Отправка ночного отчёта\n");
    // Формирование и отправка отчёта...
}

void report_task(void* data) {
    log_info("Генерация еженедельного отчёта\n");
    // Логика отчёта...
}
```

### Компиляция

```bash
gcc -shared -fPIC -o libtasks.so tasks.c
```

Библиотека линкуется с тем же `core`, что и сервер, поэтому доступен весь framework API (`log.h`, `dbquery.h`, `mail.h` и т. д.).

## Программный API

### Подключение

```c
#include "taskmanager.h"
```

### Жизненный цикл

Сервер создаёт и освобождает Task Manager автоматически при старте/остановке — вручную эти функции в задачах вызывать не нужно.

```c
taskmanager_t* taskmanager_init(void);
void taskmanager_free(taskmanager_t* manager);
int taskmanager_create_threads(appconfig_t* config);
```

`taskmanager_create_threads` запускает оба рабочих потока (async + scheduler) и возвращает 1 при успехе, 0 при ошибке создания потока.

### Асинхронные задачи

Выполнение одноразовой задачи в фоновом потоке:

```c
// Простой вызов
taskmanager_async(my_function, my_data);

// С функцией освобождения данных
taskmanager_async_with_free(my_function, my_data, my_free_function);
```

**Параметры**\
`run` — функция задачи.\
`data` — данные для передачи в функцию.\
`free_fn` — функция освобождения данных, вызывается после выполнения задачи (или `NULL`).

**Возвращаемое значение**\
1 при успехе, 0 при ошибке (менеджер не инициализирован, `run == NULL` или нехватка памяти).

<br>

### Запланированные задачи

Все функции регистрации возвращают 1 при успехе и 0 при ошибке (неверный аргумент, нехватка памяти или **задача с таким именем уже существует**). Время следующего запуска вычисляется автоматически при регистрации.

#### Интервальное расписание

```c
// Каждые 60 секунд
taskmanager_schedule(manager, "my_task", 60, my_function);
taskmanager_schedule_with_data(manager, "my_task", 60, my_function, my_data);
```

`interval` — целое число секунд, **≥ 1**.

#### Ежедневное расписание

```c
// Каждый день в 09:30
taskmanager_schedule_daily(manager, "daily_task", 9, 30, my_function);
taskmanager_schedule_daily_with_data(manager, "daily_task", 9, 30, my_function, my_data);
```

`hour` — 0–23, `minute` — 0–59.

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

`day` — 1–31.

### Управление задачами

Все функции возвращают 1 при успехе, 0 если задача с таким именем не найдена.

#### Удаление задачи

```c
int taskmanager_unschedule(taskmanager_t* manager, const char* name);
```

Полностью удаляет запланированную задачу из списка.

<br>

#### Немедленный запуск

```c
int taskmanager_trigger(taskmanager_t* manager, const char* name);
```

Запускает задачу немедленно, не дожидаясь расписания: устанавливает `next_run` в прошлое, поэтому задача выполнится на следующем тике планировщика (в течение секунды).

<br>

#### Включение/выключение

```c
int taskmanager_enable(taskmanager_t* manager, const char* name);
int taskmanager_disable(taskmanager_t* manager, const char* name);
```

Включает или выключает выполнение задачи без её удаления. Выключенная задача остаётся в списке, но не запускается планировщиком.

<br>

#### Получение информации

```c
scheduled_task_entry_t* taskmanager_get(taskmanager_t* manager, const char* name);
```

Возвращает указатель на структуру задачи или `NULL`, если задача не найдена. Структура защищена мьютексом планировщика во время поиска; изменяйте поля осознанно.

<br>

### Функции расчёта времени

Вычисляют `next_run` для заданного типа расписания. `base_time = 0` означает «использовать текущее время» — это удобно для тестирования и внешнего использования.

```c
time_t taskmanager_calc_next_daily(time_t base_time, int hour, int minute);
time_t taskmanager_calc_next_weekly(time_t base_time, int weekday, int hour, int minute);
time_t taskmanager_calc_next_monthly(time_t base_time, int day, int hour, int minute);
```

## Константы

### Тип расписания

```c
typedef enum {
    SCHEDULE_INTERVAL = 0,  // каждые N секунд
    SCHEDULE_DAILY,         // каждый день в указанное время
    SCHEDULE_WEEKLY,        // по дням недели
    SCHEDULE_MONTHLY        // по дням месяца
} schedule_type_e;
```

### Дни недели

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

### Статусы асинхронной задачи

```c
typedef enum {
    TASK_STATUS_PENDING   = 0,  // в очереди, ожидает выполнения
    TASK_STATUS_RUNNING,        // выполняется
    TASK_STATUS_COMPLETED,      // завершена
    TASK_STATUS_FAILED          // ошибка (зарезервировано)
} task_status_e;
```

## Структуры

### Запланированная задача

```c
typedef struct scheduled_task_entry {
    char            name[128];       // Имя задачи
    task_fn_t       run;             // Функция выполнения
    void*           data;            // Данные, передаваемые в run
    int             interval;        // Интервал в секундах (для SCHEDULE_INTERVAL)
    time_t          last_run;        // Время последнего запуска
    time_t          next_run;        // Время следующего запуска
    short           enabled;         // Включена ли задача (1/0)
    schedule_type_e schedule_type;   // Тип расписания
    int             schedule_day;    // День недели (0-6) или день месяца (1-31)
    int             schedule_hour;   // Час (0-23)
    int             schedule_min;    // Минута (0-59)

    struct scheduled_task_entry* next;  // Следующая задача в списке
} scheduled_task_entry_t;
```

### Асинхронная задача

```c
typedef struct task {
    task_fn_t      run;        // Функция выполнения
    task_free_fn_t free_data;  // Функция освобождения data (или NULL)
    void*          data;       // Данные, передаваемые в run
    task_status_e  status;     // Текущий статус
} task_t;
```

## Примеры использования

### Очистка устаревших сессий

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
        log_info("Удалены устаревшие сессии\n");
    }

    dbresult_free(result);
}

// Регистрация: каждые 5 минут
taskmanager_schedule(manager, "cleanup_sessions", 300, cleanup_sessions);
```

### Отправка ежедневного отчёта

```c
#include "log.h"
#include "str.h"
#include "mail.h"

void send_daily_report(void* data) {
    str_t* report = generate_daily_report();   // ваша функция

    mail_payload_t payload = {
        .from    = "noreply@example.com",
        .to      = "admin@example.com",
        .subject = "Ежедневный отчёт",
        .body    = str_get(report)
    };

    send_mail(&payload);

    str_free(report);
}

// Регистрация: каждый день в 08:00
taskmanager_schedule_daily(manager, "daily_report", 8, 0, send_daily_report);
```

### Еженедельный бэкап

```c
#include "log.h"

void weekly_backup(void* data) {
    log_info("Запуск еженедельного бэкапа\n");
    system("pg_dump mydb > /backups/weekly_backup.sql");
    log_info("Бэкап завершён\n");
}

// Регистрация: каждое воскресенье в 03:00
taskmanager_schedule_weekly(manager, "weekly_backup", SUNDAY, 3, 0, weekly_backup);
```
