---
outline: deep
description: C Web Framework logging system. Full syslog level support, configurable filtering, writing to syslog and console output.
---

# Logging

The framework provides a comprehensive logging system with full support for all syslog levels. The system allows configurable filtering by severity level and enabling/disabling logging through the configuration file.

## Logging Levels

The framework supports all 8 standard syslog logging levels (from most critical to least critical):

### log_emerg <Badge type="danger" text="0"/>

System is unusable. Critical situation requiring immediate attention.

```C
log_emerg("System is unusable: %s\n", error_msg);
```

### log_alert <Badge type="danger" text="1"/>

Action must be taken immediately. Urgent response needed to prevent serious problems.

```C
log_alert("Database connection pool exhausted\n");
```

### log_crit <Badge type="danger" text="2"/>

Critical conditions. Serious problem requiring attention.

```C
log_crit("Critical error in authentication module\n");
```

### log_error <Badge type="warning" text="3"/>

Error conditions. Problems that need to be fixed, but the system continues to work.

```C
log_error("Failed to process request: %s\n", error);
```

### log_warning <Badge type="warning" text="4"/>

Warning conditions. Potential problems that should be checked.

```C
log_warning("Connection timeout increased to %d seconds\n", timeout);
```

### log_notice <Badge type="info" text="5"/>

Normal but significant condition. Normal but important events.

```C
log_notice("Configuration reloaded successfully\n");
```

### log_info <Badge type="info" text="6"/>

Informational messages. Useful information about system operation.

```C
log_info("Processing request from %s\n", client_ip);
```

### log_debug <Badge type="tip" text="7"/>

Debug-level messages. Detailed information for diagnostics.

```C
log_debug("Query executed in %d ms: %s\n", duration, query);
```

## Helper Functions

### print

Outputs a message only to standard output without writing to syslog. Useful for debugging.

```C
print("Debug output: %d\n", value);
```

## Logging Configuration

Logging is configured through the `log` section in `config.json`:

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

Enables or disables logging. When `false`, all messages are ignored.

### level <Badge type="info" text="string"/>

Minimum logging level. Messages with lower priority will be filtered out.

Valid values (in order of decreasing priority):
- `emerg` — only critical system failures
- `alert` — emergency situations
- `crit` — critical errors
- `err` or `error` — regular errors
- `warning` or `warn` — warnings
- `notice` — important notifications
- `info` — informational messages
- `debug` — all messages including debug

**Example:** if level is set to `warning`, only messages of levels `emerg`, `alert`, `crit`, `error`, and `warning` will be logged. Messages of `notice`, `info`, and `debug` will be ignored.

## Where Logs are Stored

All messages (except `print` calls) are written to the system syslog and are usually available in `/var/log/syslog`.

To view logs use:

```bash
# View all application logs
sudo tail -f /var/log/syslog

# Filter by level (e.g., errors only)
sudo grep "error" /var/log/syslog

# Using journalctl (systemd)
sudo journalctl -f
```

## Usage Example

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

## Usage Recommendations

1. **emerg/alert/crit** — use only for truly critical situations requiring immediate intervention
2. **error** — for errors that need fixing, but the system continues to work
3. **warning** — for potential problems or unusual situations
4. **notice** — for important events in the application lifecycle
5. **info** — for tracking normal operation (start, stop, request processing)
6. **debug** — for detailed diagnostics, disable in production

::: tip Tip
In production environments, it's recommended to set the level to `info` or `notice` to avoid excessive logging. For development and debugging, use `debug`.
:::
