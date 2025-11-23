---
outline: deep
description: Route description rules for Http and WebSockets protocols
---

# Routing

Routing is used in requests for HTTP and WebSockets protocols. The rules for describing routes for both protocols are the same.

```json
...
"http": {
    "routes": {
        // Path to resource
        "/": {
            // Available methods of interaction with the resource
            "GET": [
                // Path to shared file with resource handlers
                "/var/www/server/build/exec/handlers/libindexpage.so",
                // The name of the method (function) that will be called by the request
                "get"
            ],
            "POST": ["/var/www/server/build/exec/handlers/libindexpage.so", "post"]
        },
        "/admin/users": {
            "GET": ["/var/www/server/build/exec/handlers/libindexpage.so", "users_list"],
        },
        "/admin/users/{id|\\d+}": {
            "PATCH": ["/var/www/server/build/exec/handlers/libindexpage.so", "users_update"]
        },
        "^/managers/{name|[a-z]+}$": {
            "PATCH": ["/var/www/server/build/exec/handlers/libindexpage.so", "managers_update"]
        },
        "/wss": {
            "GET": ["/var/www/server/build/exec/handlers/libindexpage.so", "websocket"]
        }
    },
    ...
},
"websockets": {
    "routes": {
        "/": {
            "GET": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "get"],
            "POST": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "post"],
            "PATCH": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "path"],
            "DELETE": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "delete"]
        }
    }
},
...
```

The path to the resource can be specified with the exact value `/admin/users` or the pseudo-regular expression `/admin/users/{id|\\d+}`.

Template parameters can be retrieved using the `query` method, for example:

```C
const char* id = ctx->request->query(ctx, "id");
```
