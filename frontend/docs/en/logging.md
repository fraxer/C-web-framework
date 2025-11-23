---
outline: deep
description: C Web Framework logging system. Info and error levels, writing to syslog and console output for debugging.
---

# Logging

C Web Framework provides a simple logging system. This logging system allows you to store messages of two types `info` and `error`.

## Log messages

Log messages are written by calling one of the following methods:

log_info: logs a message containing some useful information.

log_error: logs a critical error that should be attended to as soon as possible.

Messages are stored in the system log `/var/log/syslog`.

---

log_print: prints a message to standard output without logging.

```C
#include "http1.h"
#include "log.h"

void get(http1request_t* request, http1response_t* response) {
    log_error("Error message\n");

    response->data(response, "Error");
}
```
