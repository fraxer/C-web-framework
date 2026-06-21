---
outline: deep
description: Hot reload of C Web Framework without stopping the server. Soft and hard modes for updating configuration and handlers.
---

# Hot reload

Hot reload lets you update configuration and handlers without stopping the server or
interrupting in-flight requests. On the `SIGUSR1` signal, the server re-reads
`config.json` and reinitializes its components: routes, `.so` handlers, database
connections, storages, sessions, and the task scheduler.

Hot reload updates configuration and handlers, but **not the `cpdy` binary itself** —
updating the framework core requires a full restart.

## Triggering a reload

The reload event is attached to the `SIGUSR1` signal. Send it to the server process
(`cpdy`) with either of:

```bash
pkill -USR1 cpdy        # by process name
kill  -USR1 <pid>       # by process ID
```

Before reloading, the server flushes its logging buffers to disk, so the latest log
entries are not lost.

## Configuration validation

The new `config.json` is parsed and validated first. If the file contains an error or
an invalid configuration, the reload is aborted and the server keeps running with the
old configuration. This lets you edit the configuration live without risking a
running server outage.

Repeated signals that arrive while a reload is in progress are ignored — a new reload
only starts after the current one finishes.

## Reload modes

The behavior of active connections during a reload is controlled by the `reload`
property in the `main` section (see [Configuration → reload](/en/config#reload)). The
default value is `soft`.

```json
{
    "main": {
        "reload": "hard",
        ...
    },
    ...
}
```

### soft <Badge type="tip" text="default"/>

The server stops accepting **new** connections but keeps serving the already active
ones — they complete naturally: to the end of the request, on a keep-alive timeout,
or when the client closes the connection. The reload runs as soon as the last active
connection is closed. No in-flight request is cut off.

### hard

The server forcibly closes all network connections (socket `shutdown`) immediately
after the signal, dropping in-flight requests. The reload completes as quickly as
possible. Use this mode when changes must be applied urgently and losing the current
requests is acceptable.
