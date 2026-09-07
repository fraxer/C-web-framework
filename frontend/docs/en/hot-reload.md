---
outline: deep
description: Hot reload of C Web Framework without stopping the server. Soft and hard modes, picking up rebuilt handlers, and what needs a restart.
---

# Hot reload

Hot reload lets you update the configuration without stopping the server or
interrupting in-flight requests. On the `SIGUSR1` signal, the server re-reads
`config.json` and reinitializes its components: routes, domains and redirects,
database connections, storages, sessions, rate limiters, TLS, and the task
scheduler.

## What a reload updates, and what it does not

A reload re-reads the **configuration**. It does not pick up every kind of code:

| | Picked up by `SIGUSR1` |
|---|---|
| Any value in `config.json` | yes |
| A route pointed at a different `.so` or a different function | yes |
| A `.so` at a path that has not been loaded before | yes |
| **A handler `.so` rebuilt in place** | **yes** |
| **The application module from [`main.modules`](/en/config#modules), rebuilt in place** | **yes** |
| The `cwfr` binary, that is, the core itself | no |

### A rebuilt handler

Rebuild a handler over the same file, send `SIGUSR1`, and the new configuration
generation runs the new code. Nothing beyond the signal is needed — the path in
`config.json` stays as it is.

On its own this would not work. `dlopen()` decides a library is already loaded
first by the path string — that is, **without opening the file** — and only then by
the device/inode pair. So overwriting the file at the same path, a symlink and a
hard link are all equally useless: it takes a new name AND a new inode together.

Which is what the server arranges. Having noticed that the file on disk has changed
while the old path is still loaded, it puts a **shadow copy** next to the original
and loads that:

```
handlers/models/lib_modeluser.so
handlers/models/.cwfr-shadow-4211-1-lib_modeluser.so   ← the copy it runs from
```

What is worth knowing about it:

* **The copy sits next to the original**, not in `tmp`, and that matters: `$ORIGIN`
  in the library's `RPATH` still points at its own directory, so private
  dependencies living beside the handler are found. System libraries are located
  through `ldconfig` and are not affected at all.
* **The directory holding the `.so` has to be writable.** Where it is not — `/opt`
  being the usual case — the copy goes to [`main.tmp`](/en/config#tmp), and the log
  says that `$ORIGIN` does not apply to that load.
* **The copy outlives the library's unload** and is removed only when the process
  exits. That is deliberate: while the server is alive, every generation's file is
  on disk, so an AddressSanitizer report or a core dump symbolises in full — function
  names, files and lines, including code that has already been unloaded.
* **Mapped memory does not grow**: the mapping goes with the generation, and it is
  only the file on disk that outlives the unload.
* **Copies do accumulate on disk** for the life of the process — at most one per
  rebuild. A normal shutdown removes all of them; what a `kill -9` or a crash leaves
  behind is swept by the next start, which skips files belonging to live processes.

::: warning Read a core dump before restarting
Starting a new process removes the previous one's shadow copies. To investigate the
core of a crashed server, do it before the restart — or save the directory holding
the `.cwfr-shadow-*` files first.
:::

### The application module

This works for the module in [`main.modules`](/en/config#modules) too: rebuild it
in place, send `SIGUSR1`, and the handlers of the new generation call the new
code while the old module is unloaded along with the old generation.

A shadow copy alone is not enough here, for a reason of its own. A handler is
linked against the module and records not its path but its `SONAME` — the name a
library declares for itself inside the ELF. When the handler is loaded, the
dynamic loader looks that name up **among the objects already loaded**, finds a
match before the file is even opened, and never looks at an inode at all. A copy
of the module carries the same `SONAME`, and the old module comes first in that
list — so it would answer every time.

Each generation is therefore given a **name of its own**: `DT_SONAME` is rewritten
in the module's copy, and `DT_NEEDED` in the copies of the handlers that need it.
The string is overwritten where it lies and keeps its length, so nothing in the
file moves: `libapp.so` becomes, say, `libapp.0f`.

What follows from that:

* **Dependent handlers are copied too**, even when they have not changed
  themselves — their `DT_NEEDED` has to point at this generation's module.
  Handlers that never call the module are unaffected: with `--as-needed` the
  linker leaves them no such entry, and they are not copied.
* **`ldd` and a debugger will show the tagged name.** That is cosmetic: the file
  path and the `build-id` are untouched, so symbolisation, separate debug files
  and AddressSanitizer reports work as they always did.
* **State in the module's static variables does not carry across generations.**
  Each generation runs the `app_init()` of its own instance and has its own
  statics. Anything that must survive a reload belongs in the database, a cache
  or a session.
* **The context destructors now belong to the generation.** Registered through
  `httpctx_set_user_data_free()`/`wsctx_set_user_data_free()`, they are held in
  the configuration rather than globally: a rebuilt module registers a different
  address, and a request still running on the old generation is freed by the old
  generation's destructor.

::: warning When the module is not picked up after all
The tag is written into the `SONAME`, so a module that has none — built without
`-Wl,-soname`, or with `NO_SONAME` in CMake — cannot be renamed. Neither can a
name the linker merged into another, which happens when a handler depends both on
`libapp.so` and on something whose name ends in `libapp.so`.

In both cases the reload is **not** cancelled: the server keeps the module it is
running, logs the reason and applies everything else. A restart picks the new
module up — or the older trick of a versioned name does:

```json
"main": {
    "modules": ["libapp.2.so"]
}
```
:::

### Signal after the build, not during it

A linker does not write a `.so` atomically: it unlinks the old file and creates a
new one, so between the start and the end of the write the path holds an incomplete
ELF. A `SIGUSR1` arriving at that moment copies a truncated file — `dlopen()`
rejects it, the reload is refused at the validation stage, and the server carries on
with the old configuration.

Nothing breaks and the next reload sorts itself out, but the wasted signal is worth
avoiding: send `SIGUSR1` once the build has finished, or build into a temporary
directory and publish with `mv`, which renames atomically.

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

The check is a **full dry run** on a configuration that is then thrown away: the
modules from [`main.modules`](/en/config#modules) are opened, their `app_init()` is
called, the handlers are loaded and the middleware names are resolved. A typo in a
path, a missing `app_init`, a broken `.so`, an unknown middleware name — each of
them refuses the reload before the old generation is taken down. Without that check,
`reload: hard` would have closed the sockets first, and a failed load would leave a
live process serving nothing.

That is also what lets one reload add both a new middleware in `app_init()` and a
route naming it: the validation pass runs the `app_init()` of the **new** build
rather than consulting the running one's registry. The running generation's registry
is set aside for the duration and put back afterwards — what the dry run registers
points into modules it unloads a moment later.

The price is that `app_init()` runs twice per reload. It has always had to be
idempotent, and both runs start from an empty registry so no duplicate arises; but
any side effect it has happens twice.

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
