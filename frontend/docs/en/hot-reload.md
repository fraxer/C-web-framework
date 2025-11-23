---
outline: deep
description: Hot reload of C Web Framework without stopping the server. Soft and hard modes for updating configuration and handlers.
---

# Hot reload

Hot reload allows you to update configuration and handlers without stopping the server and interrupting request processing. The configuration file will be re-read with the new state.

The reload event is attached to the user signal `SIGUSR1`.

The application can be reloaded using the console command:

```bash
pkill -USR1 cpdy
```

There are two modes for handling active connections after a reload, they are `soft` and `hard`.
In the configuration file, the mode is set by the `reload` property.

```json
{
    "main": {
        "reload": "hard",
        ...
    },
    ...
}
```

In `soft` mode, active connections will be terminated when the connection expires or when the connection is closed by the client.

In `hard` mode, active connections will be forced to close as quickly as possible.
