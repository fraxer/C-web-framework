---
outline: deep
description: Система логирования C Web Framework. Поддержка всех уровней syslog, настраиваемая фильтрация, запись в syslog и вывод в консоль.
---

# Логирование

Фреймворк предоставляет полноценную систему логирования с поддержкой всех уровней syslog. Система позволяет настраивать фильтрацию по уровню важности и включать/выключать логирование через конфигурационный файл.

## Уровни логирования

Фреймворк поддерживает все 8 стандартных уровней логирования syslog (от наиболее критичных к наименее критичным):

### log_emerg <Badge type="danger" text="0"/>

Система неработоспособна. Критическая ситуация, требующая немедленного внимания.

```C
log_emerg("System is unusable: %s\n", error_msg);
```

### log_alert <Badge type="danger" text="1"/>

Требуется немедленное действие. Необходима срочная реакция для предотвращения серьезных проблем.

```C
log_alert("Database connection pool exhausted\n");
```

### log_crit <Badge type="danger" text="2"/>

Критическое состояние. Серьезная проблема, требующая внимания.

```C
log_crit("Critical error in authentication module\n");
```

### log_error <Badge type="warning" text="3"/>

Ошибки. Проблемы, которые нужно исправить, но система продолжает работать.

```C
log_error("Failed to process request: %s\n", error);
```

### log_warning <Badge type="warning" text="4"/>

Предупреждения. Потенциальные проблемы, которые стоит проверить.

```C
log_warning("Connection timeout increased to %d seconds\n", timeout);
```

### log_notice <Badge type="info" text="5"/>

Важные уведомления. Нормальные, но важные события.

```C
log_notice("Configuration reloaded successfully\n");
```

### log_info <Badge type="info" text="6"/>

Информационные сообщения. Полезная информация о работе системы.

```C
log_info("Processing request from %s\n", client_ip);
```

### log_debug <Badge type="tip" text="7"/>

Отладочные сообщения. Детальная информация для диагностики.

```C
log_debug("Query executed in %d ms: %s\n", duration, query);
```

## Вспомогательные функции

### print

Выводит сообщение только в стандартный вывод без записи в журнал syslog. Полезно для отладки.

```C
print("Debug output: %d\n", value);
```

## Настройка логирования

Система логирования настраивается через секцию `log` в файле `config.json`:

```json
{
    "main": {
        "log": {
            "enabled": true,
            "level": "info"
        }
    }
}
```

### enabled <Badge type="info" text="boolean"/>

Включает или отключает логирование. При `false` все сообщения игнорируются.

### level <Badge type="info" text="string"/>

Минимальный уровень логирования. Сообщения с более низким приоритетом будут отфильтрованы.

Допустимые значения (в порядке убывания приоритета):
- `emerg` — только критические сбои системы
- `alert` — экстренные ситуации
- `crit` — критические ошибки
- `err` или `error` — обычные ошибки
- `warning` или `warn` — предупреждения
- `notice` — важные уведомления
- `info` — информационные сообщения
- `debug` — все сообщения, включая отладочные

**Пример:** если установлен уровень `warning`, будут записываться только сообщения уровней `emerg`, `alert`, `crit`, `error` и `warning`. Сообщения `notice`, `info` и `debug` будут проигнорированы.

## Где хранятся логи

Все сообщения (кроме вызовов `print`) записываются в системный журнал syslog и обычно доступны в файле `/var/log/syslog`.

Для просмотра логов используйте:

```bash
# Просмотр всех логов приложения
sudo tail -f /var/log/syslog

# Фильтрация по уровню (например, только ошибки)
sudo grep "error" /var/log/syslog

# Использование journalctl (systemd)
sudo journalctl -f
```

## Пример использования

```C
#include "http1.h"
#include "log.h"

void get(http1request_t* request, http1response_t* response) {
    log_info("Processing GET request\n");

    if (validate_request(request)) {
        log_debug("Request validation passed\n");
        response->data(response, "OK");
    } else {
        log_error("Request validation failed\n");
        response->status(response, 400);
        response->data(response, "Bad Request");
    }
}
```

## Рекомендации по использованию

1. **emerg/alert/crit** — используйте только для действительно критических ситуаций, требующих немедленного вмешательства
2. **error** — для ошибок, которые нужно исправить, но система продолжает работать
3. **warning** — для потенциальных проблем или нестандартных ситуаций
4. **notice** — для важных событий в жизненном цикле приложения
5. **info** — для отслеживания нормальной работы (запуск, остановка, обработка запросов)
6. **debug** — для детальной диагностики, отключайте в production

::: tip Совет
В production окружении рекомендуется устанавливать уровень `info` или `notice`, чтобы избежать избыточного логирования. Для разработки и отладки используйте `debug`.
:::
