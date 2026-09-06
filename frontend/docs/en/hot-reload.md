---
outline: deep
description: Hot reload of C Web Framework without stopping the server. Soft and hard modes, what a reload updates and what needs a restart.
---

# Hot reload

Hot reload lets you update the configuration without stopping the server or
interrupting in-flight requests. On the `SIGUSR1` signal, the server re-reads
`config.json` and reinitializes its components: routes, domains and redirects,
database connections, storages, sessions, rate limiters, TLS, and the task
scheduler.

## What a reload updates, and what it does not

A reload re-reads the **configuration**, not the code. The difference matters more
than it looks:

| | Picked up by `SIGUSR1` |
|---|---|
| Any value in `config.json` | yes |
| A route pointed at a different `.so` or a different function | yes |
| A `.so` at a path that has not been loaded before | yes |
| **The code inside an already-loaded `.so`** — a handler or the application module | **no** |
| The `cwfr` binary, that is, the core itself | no |

::: warning Rebuilding a `.so` in place is not enough
Rebuild a handler or the application module over the same file, send `SIGUSR1`, and
the process keeps running the **old code** — no number of signals changes that. It
takes a server restart.
:::

There are two distinct reasons. The application module from
[`main.modules`](/en/config#modules) is never unloaded: old-generation workers may be
executing a middleware from it at that moment, and the registered function pointers
lead into its text, so unmapping it would pull the code out from under a running
thread. Handlers are unloaded, but a new configuration generation loads them
**before** the old one is released, so the reference count never reaches zero and
`dlopen()` returns the object already mapped instead of reading the file again.

### Updating code without a restart

Since the limitation is only about an **already-loaded path**, versioned file names
get around it: put the new build next to the old one under a new name and point the
configuration at it.

```json
"/api/users": {
    "GET": { "file": "handlers/models/lib_modeluser.2.so", "function": "list" }
}
```

That path has not been loaded, so the reload takes the new file. Overwriting the old
`.so` does not work — the name is the same.

The same trick works for the application module: name a new path in `main.modules`,
and the reload loads the new `.so` whose `app_init()` fills the registry that was
just cleared.

One caveat: the previous module stays mapped for the life of the process — as
above, it cannot be unloaded. Swapping repeatedly therefore accumulates mapped
memory, and a restart is eventually needed anyway.

## Triggering a reload

The reload event is attached to the `SIGUSR1` signal. Send it to the server process
(`cwfr`) with either of:

```bash
pkill -USR1 cwfr        # by process name
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
