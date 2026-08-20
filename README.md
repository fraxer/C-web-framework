# C Web Framework

A high-performance web server and framework for building web applications in C for Linux. Built on an event-driven epoll architecture that efficiently handles a large number of concurrent connections.

The framework provides a complete set of tools for developing modern web applications: from a basic HTTP server to database integration, WebSocket connections, authentication system, and file storage.

Documentation is available at: [https://cwebframework.tech/en/introduction.html](https://cwebframework.tech/en/introduction.html)

## Key Features

### HTTP Server
* **HTTP/1.1** - full HTTP/1.1 protocol support
* **Routing** - flexible routing system with dynamic parameter support
* **Redirects** - configurable redirect rules with regular expression support
* **Middleware** - middleware handler system for HTTP and WebSocket requests
* **Filters** - built-in filters for chunked encoding, range requests, gzip, cache control
* **Multipart/Form-data** - file upload and form processing
* **Cookie** - full cookie support with secure, httpOnly, sameSite settings
* **Gzip compression** - automatic response compression for supported content types
* **TLS/SSL** - secure connections with configurable cipher suites
* **IPv4 and IPv6** - the `ip` field of a server accepts either address family
* **HTTP client** - outbound HTTP/1.1 client for API calls and proxying

### HTTP/2
* **HTTP/2 server** (RFC 9113) - negotiated via TLS ALPN, or cleartext upgrade (h2c: `Upgrade` header and prior-knowledge preface sniffing)
* **Multiplexing** - concurrent streams over one connection with per-stream flow control and a configurable receive window
* **HPACK** - own header compression with static table and Huffman coding
* **Trailers and 103 Early Hints** - response trailers and early hints staged on the stream
* **WebSocket over HTTP/2** - Extended CONNECT (RFC 8441): the same WebSocket handlers run inside an h2 tunnel
* **Lifecycle** - two-stage GOAWAY drain, PING keep-alive, SETTINGS ACK timeout
* **Hardening** - MAX_HEADER_LIST_SIZE, CONTINUATION frame cap, rapid-reset budget; all `http2_*` limits are tunable at runtime

### HTTP/3 over QUIC (opt-in: `-DINCLUDE_HTTP3=yes`)
* **Own QUIC stack** - TLS 1.3 handshake driven through the OpenSSL QUIC TLS API (`SSL_set_quic_tls_cbs`, needs OpenSSL >= 3.5), ACK processing, loss recovery, congestion control with pacing
* **Connection migration** - connection IDs and endpoint rebinding
* **HTTP/3 server** (RFC 9114) with QPACK header compression
* **0-RTT early data** - enabled with `http3_early_data`
* **QUIC v2 (RFC 9369) and compatible version negotiation (RFC 9368)** - behind `http3_version_2`
* **Keep-alive and idle timeout**, UDP GSO batching for outbound datagrams
* **qlog** - JSON-SEQ traces viewable in qvis (`http3_qlog_dir`)

### WebSocket
* **WebSocket server** - full WebSocket protocol support
* **Broadcasting** - message broadcasting system for client groups
* **Custom channels** - named channel creation with recipient filtering
* **JSON over WebSocket** - built-in JSON message support
* **WebSocket middleware** - middleware handler system for WebSocket
* **Transport independence** - the same handlers serve HTTP/1.1 upgrade and HTTP/2 Extended CONNECT tunnels

### Databases
* **PostgreSQL** - native support with prepared statements
* **MySQL** - native support with SQL injection protection
* **Redis** - Redis support for caching and sessions
* **SQLite** - embedded database support (no external server required)
* **ORM-like operations** - model system for working with tables
* **Migrations** - database schema versioning system (scaffold, apply pending/all/N)
* **Query Builder** - safe SQL query building with parameterization
* **Prepared Statements** - SQL injection protection at framework level
* **Transactions** - `begin`/`commit`/`rollback` with isolation levels

### Authentication and Security
* **Authentication system** - built-in registration and authorization system
* **Sessions** - file-based, Redis, and database-backed sessions, each encrypted with **AES-256-GCM**
* **Password hashing** - secure password storage using PBKDF2-HMAC-SHA256 (salt + hash)
* **JWT** - JSON Web Token signing and verification
* **Validation** - validators for email, passwords, and other data
* **Authentication middleware** - route protection with session verification
* **RBAC** - Role-Based Access Control system

### File Storage
* **Local storage** - file system operations
* **S3-compatible storage** - integration with S3 and S3-compatible services
* **File operations** - create, read, update, delete files
* **Temporary files** - automatic temporary file management
* **File upload** - multipart upload processing with storage saving

### Email
* **SMTP client** - email sending via SMTP, including asynchronous sending
* **DKIM signature** - DKIM signature support for sender authentication
* **Email templates** - email template support

### Internationalization
* **i18n** - translation system with locale catalogs (gettext-style domains)
* **Domain translation** - configurable translation sources per domain in `config.json`
* **Internationalized Domain Names (IDN)** - Punycode handling for IDN domains

### Task Scheduling
* **Task Manager** - scheduled tasks loaded from shared libraries (`.so`)
* **Interval tasks** - run handlers at fixed intervals (e.g. periodic cleanup)
* **Dynamic loading** - tasks reference a `.so` file + function, like routes

### Template Engine
* **Template Engine** - built-in template engine
* **Variables and loops** - dynamic HTML generation
* **Model integration** - direct model passing to templates

### JSON
* **JSON parser** - high-performance JSON parser
* **JSON generator** - data serialization to JSON
* **Thread safety** - thread-safe JSON operations
* **Unicode support** - proper handling of Unicode characters and surrogate pairs

### Performance and Scalability
* **Event-Driven Architecture** - epoll-based architecture
* **Multiprocessing and multithreading** - worker processes with per-worker handler threads
* **Connection pool** - database connection reuse
* **Rate Limiting** - request rate limiting (DDoS protection)
* **Hot reload** - application updates without server restart (`"reload": "soft"` keeps connections, `"hard"` restarts workers)
* **Observability** - concurrency counters exposed as JSON at `GET /metrics` (off unless `"metrics": true`)

### Utilities and Data Structures
* **String (str_t)** - dynamic strings with SSO optimization
* **Array** - dynamic arrays
* **HashMap/Map** - associative arrays for fast lookup
* **Arena** - region allocator for per-request objects
* **Buffer objects (bufo) / queue (cqueue)** - reusable buffers and FIFO queue
* **JSON structures** - working with JSON as objects
* **JWT** - JSON Web Token signing/verification
* **UUID** - UUID generation
* **Hashing** - SHA-1, SHA-256, PBKDF2-HMAC-SHA256
* **AES-256-GCM** - authenticated encryption (session secrets)
* **UTF-8** - proper Unicode and surrogate-pair handling
* **Base64** - Base64 encoding/decoding
* **Logging** - logging system with configurable levels

### Additional Features
* **Modular architecture** - easy component connection and disconnection
* **Dynamic loading** - handler loading from shared libraries (.so)
* **JSON configuration** - centralized configuration in config.json
* **MIME types** - automatic content type detection
* **Domains and virtual hosts** - multiple domain support on a single server

## Project Architecture

```
project/
├── backend/
│   ├── core/                          # Framework core (git submodule)
│   │   ├── apps/                      # Executables
│   │   │   ├── server/                #   → cwfr (the web server)
│   │   │   └── migrate/               #   → migrate (DB migration CLI)
│   │   ├── protocols/                 # Protocol implementations
│   │   │   ├── http/                  # HTTP/1.1 server + client, h2c upgrade
│   │   │   ├── http2/                 # HTTP/2 server (frames, HPACK, WebSocket-over-h2)
│   │   │   ├── http3/                 # HTTP/3 server (frames, QPACK)
│   │   │   ├── quic/                  # QUIC transport (crypto, ACK, loss, congestion control)
│   │   │   ├── hq/                    # HTTP/0.9 interop shim (build flag, not for production)
│   │   │   ├── websocket/             # WebSocket server + broadcast wrappers
│   │   │   └── smtp/                  # SMTP client, DKIM
│   │   ├── framework/                 # Framework components
│   │   │   ├── database/              # PostgreSQL, MySQL, Redis, SQLite
│   │   │   ├── model/                 # ORM model system (mfield / mschema / mparams)
│   │   │   ├── session/               # Sessions (file / redis / db) + AES-256-GCM
│   │   │   ├── storage/               # File storage (FS, S3)
│   │   │   ├── view/                  # Template engine
│   │   │   ├── middleware/            # Middleware system
│   │   │   ├── taskmanager/           # Scheduled task runner
│   │   │   └── translation/           # i18n translation
│   │   ├── src/                       # Core runtime
│   │   │   ├── server/                # HTTP server
│   │   │   ├── multiplexing/          # Epoll multiplexing
│   │   │   ├── thread/                # Multithreading
│   │   │   ├── connection/            # Connection management
│   │   │   ├── socket/                # Socket abstraction
│   │   │   ├── udp/                   # UDP socket + QUIC endpoint (GSO)
│   │   │   ├── route/                 # Routing system
│   │   │   ├── domain/                # Virtual hosts / domains (regex, IDN)
│   │   │   ├── config/                # Config loading (JSON)
│   │   │   ├── ratelimiter/           # Rate limiting
│   │   │   ├── moduleloader/          # Dynamic .so loading + reload
│   │   │   ├── broadcast/             # Broadcasting system
│   │   │   ├── metrics/               # Concurrency counters (/metrics)
│   │   │   ├── mimetype/              # MIME type detection
│   │   │   ├── openssl/               # OpenSSL helpers
│   │   │   └── signal/                # Signal handling
│   │   ├── misc/                      # Utilities (headers + small .c)
│   │   │   ├── str.h                  # Dynamic strings (SSO)
│   │   │   ├── array.h                # Arrays
│   │   │   ├── hashmap.h / map.h      # Associative arrays
│   │   │   ├── json.h                 # JSON parser/generator
│   │   │   ├── jwt.h                  # JSON Web Tokens
│   │   │   ├── uuid.h                 # UUIDs
│   │   │   ├── sha1.h / sha256.h      # Hashing
│   │   │   ├── base64.h               # Base64
│   │   │   ├── ipaddr.h               # IPv4/IPv6 address parsing
│   │   │   ├── i18n.h / idn_utils.h   # i18n + IDN domains
│   │   │   ├── arena.h, bufo.h, cqueue.h  # Arena, buffer objects, FIFO queue
│   │   │   ├── log.h                  # Logging
│   │   │   └── gzip.h                 # Gzip compression
│   │   └── tests/                     # Unit/integration tests + ci.sh gate
│   │
│   ├── app/                           # User application
│   │   ├── routes/                    # HTTP / WebSocket handlers (compiled to .so)
│   │   │   ├── auth/                  # Authentication (login, registration)
│   │   │   ├── index/                 # Main page / WebSocket entry
│   │   │   ├── ws/                    # WebSocket handlers
│   │   │   ├── files/                 # File operations
│   │   │   ├── models/                # Model CRUD API
│   │   │   ├── email/                 # Email sending
│   │   │   ├── db/                    # Database examples
│   │   │   ├── json/                  # JSON handling examples
│   │   │   ├── httpclient/            # Outbound HTTP client examples
│   │   │   ├── middleware/            # Middleware examples
│   │   │   └── bench/                 # /metrics counter endpoint
│   │   ├── models/                    # Data models (ORM)
│   │   │   ├── user.c / role.c / permission.c
│   │   │   ├── user_role.c / role_permission.c   # junction tables (RBAC)
│   │   │   ├── *view.c                            # view models (JOIN queries)
│   │   │   └── prepare_statements.c               # named prepared statements
│   │   ├── middlewares/               # Custom middleware
│   │   │   ├── httpmiddlewares.c      # HTTP middleware (auth, rate limit)
│   │   │   ├── wsmiddlewares.c        # WebSocket middleware
│   │   │   └── middlewarelist.c       # registers middleware by name for config.json
│   │   ├── contexts/                  # Request contexts (httpctx.c, wsctx.c)
│   │   ├── auth/                      # Authentication module
│   │   │   ├── auth.c                 # password hashing, authenticate()
│   │   │   ├── password_validator.c   # password validation
│   │   │   └── email_validator.c      # email validation
│   │   ├── migrations/                # Database migrations (int up(const char* dbid))
│   │   │   ├── s1/                    # Migrations for server s1
│   │   │   └── s2/                    # Migrations for server s2
│   │   ├── broadcasting/              # Broadcasting channels
│   │   └── views/                     # Templates (.tpl)
│   │
│   └── config.json                    # Application configuration
│
└── frontend/                          # VitePress documentation site
    └── docs/                          # Russian docs at the root, English mirror in docs/en/
```

## Usage Examples

### HTTP Handler

```c
#include "http.h"
#include "query.h"

void my_handler(httpctx_t* ctx) {
    // Get query parameter
    int ok = 0;
    const char* name = query_param_char(ctx->request->query_, "name", &ok);

    // Get JSON from request body
    json_doc_t* doc = ctx->request->get_payload_json(ctx->request);

    // Work with JSON
    json_token_t* root = json_root(doc);
    json_object_set(root, "status", json_create_string("success"));

    // Set header
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");

    // Send response
    ctx->response->send_data(ctx->response, json_stringify(doc));

    json_free(doc);
}
```

### WebSocket Handler with Broadcasting

```c
#include "websockets.h"

void channel_join(wsctx_t* ctx) {
    mybroadcast_id_t* id = mybroadcast_id_create();

    // Subscribe: on HTTP/1.1 the connection joins the channel, on an RFC 8441
    // tunnel (WebSocket over h2) it is that one stream
    websockets_broadcast_add("my_channel", ctx->request, id, mybroadcast_send_data);

    ctx->response->send_data(ctx, "Joined channel");
}

void channel_send(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol =
        (websockets_protocol_resource_t*)ctx->request->protocol;

    char* message = protocol->get_payload(protocol);

    // Send message to the channel, suppressing the sender's own subscription
    websockets_broadcast_send("my_channel", ctx->request,
                              message, strlen(message), NULL, NULL);

    free(message);
}
```

### Database Operations

```c
#include "db.h"
#include "model.h"

void get_users(httpctx_t* ctx) {
    // Parameterized query (SQL injection protection)
    array_t* params = array_create();
    mparams_fill_array(params,
        mparam_int(id, 123),
        mparam_text(email, "user@example.com")
    );

    // dbid is "<driver>.<host_id>" from config.json, e.g. postgresql.p1
    dbresult_t* result = dbquery("postgresql.p1",
        "SELECT * FROM users WHERE id = :id OR email = :email",
        params);

    array_free(params);

    if (!dbresult_ok(result)) {
        ctx->response->send_data(ctx->response, "Query error");
        dbresult_free(result);
        return;
    }

    // Process results
    for (int row = 0; row < dbresult_query_rows(result); row++) {
        const db_table_cell_t* field = dbresult_cell(result, row, 0);
        // Work with data...
    }

    dbresult_free(result);
}
```

### Working with Models (ORM)

```c
#include "user.h"

void create_user_example(httpctx_t* ctx) {
    // Create model instance
    user_t* user = user_instance();

    // Set values
    user_set_email(user, "newuser@example.com");
    user_set_name(user, "John Doe");

    // Generate password hash
    str_t* secret = generate_secret("password123");
    user_set_secret(user, str_get(secret));
    str_free(secret);

    // Save to database
    if (!user_create(user)) {
        ctx->response->send_data(ctx->response, "Error creating user");
        user_free(user);
        return;
    }

    // Output model as JSON
    ctx->response->send_model(ctx->response, user,
                        display_fields("id", "email", "name"));

    user_free(user);
}
```

### Middleware

```c
#include "httpmiddlewares.h"

// Protected route with middleware
void secret_page(httpctx_t* ctx) {
    // Authentication check via middleware
    middleware(
        middleware_http_auth(ctx)
    )

    // This code runs only if user is authorized
    user_t* user = httpctx_get_user(ctx);
    ctx->response->send_data(ctx->response, "Welcome to secret page!");
}
```

### File Storage

```c
#include "storage.h"

void upload_file(httpctx_t* ctx) {
    // Get uploaded file
    file_content_t content = ctx->request->get_payload_filef(ctx->request, "myfile");

    if (!content.ok) {
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Save to S3 storage
    storage_file_content_put("remote", &content,
                            "uploads/%s", content.filename);

    ctx->response->send_data(ctx->response, "File uploaded");
}

void download_file(httpctx_t* ctx) {
    // Stream a file straight out of a named storage
    ctx->response->send_filef(ctx->response, "local", "uploads/report.txt");
}
```

### Sending Email with DKIM

```c
#include "mail.h"

void send_email(httpctx_t* ctx) {
    mail_payload_t payload = {
        .from = "noreply@example.com",
        .from_name = "My Application",
        .to = "user@example.com",
        .subject = "Welcome!",
        .body = "Welcome to our service!"
    };

    if (!send_mail(&payload)) {
        ctx->response->send_data(ctx->response, "Error sending email");
        return;
    }

    ctx->response->send_data(ctx->response, "Email sent");
}
```

## Configuration

The framework uses `config.json` for centralized configuration. Runtime tuning keys (`metrics`, the `http2_*` and `http3_*` limits) are read from the `main.env` map and from the process environment:

```json
{
  "main": {
    "workers": 4,
    "threads": 2,
    "reload": "hard",
    "buffer_size": 16384,
    "client_max_body_size": 110485760,
    "tmp": "/tmp",
    "gzip": ["text/html", "text/css", "application/json", "application/javascript"],
    "log": { "enabled": true, "level": "info" },
    "env": { "refresh_token_expiration": 15552000 }
  },
  "migrations": {
    "source_directory": "/path/to/migrations"
  },
  "translations": [
    { "domain": "backend", "path": "/app/locale" }
  ],
  "task_manager": [
    {
      "name": "cleanup_expired_tokens",
      "type": "interval",
      "interval": 60,
      "file": "path/to/libtasks.so",
      "function": "cleanup_authorization_codes"
    }
  ],
  "servers": {
    "s1": {
      "domains": ["example.com", "*.example.com", "(api|www).example.com", "mail.*"],
      "ip": "127.0.0.1",
      "port": 443,
      "root": "/var/www/html",
      "index": "index.html",
      "ratelimits": {
        "default": { "burst": 15, "rate": 15 },
        "strict": { "burst": 1, "rate": 0 }
      },
      "http": {
        "ratelimit": "default",
        "redirects": {
          "/old-path": "/new-path",
          "/user/(\\d+)/(.+)": "/profile/{1}/{2}"
        },
        "middlewares": ["middleware_auth"],
        "routes": {
          "/api/users": {
            "GET": { "file": "path/to/handler.so", "function": "get_users", "ratelimit": "strict" }
          },
          "/api/users/create": {
            "POST": { "file": "path/to/handler.so", "function": "create_user" }
          }
        }
      },
      "websockets": {
        "default": { "file": "path/to/ws_handler.so", "function": "default_handler" },
        "routes": {
          "/ws": {
            "GET": { "file": "path/to/ws_handler.so", "function": "echo" },
            "POST": { "file": "path/to/ws_handler.so", "function": "broadcast" }
          }
        }
      },
      "tls": {
        "fullchain": "/path/to/fullchain.pem",
        "private": "/path/to/privkey.pem",
        "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256"
      }
    }
  },
  "databases": {
    "postgresql": [{
      "host_id": "p1",
      "ip": "127.0.0.1",
      "port": 5432,
      "dbname": "mydb",
      "user": "dbuser",
      "password": "dbpass",
      "connection_timeout": 3
    }],
    "mysql": [{
      "host_id": "m1",
      "ip": "127.0.0.1",
      "port": 3306,
      "dbname": "mydb",
      "user": "dbuser",
      "password": "dbpass"
    }],
    "redis": [{
      "host_id": "r1",
      "ip": "127.0.0.1",
      "port": 6379,
      "dbindex": 0,
      "user": "",
      "password": ""
    }],
    "sqlite": [{
      "host_id": "s1",
      "path": "/var/lib/cwfr/app.db"
    }]
  },
  "storages": {
    "files": {
      "type": "filesystem",
      "root": "/var/www/storage"
    },
    "s3": {
      "type": "s3",
      "access_id": "your_access_id",
      "access_secret": "your_access_secret",
      "protocol": "https",
      "host": "s3.amazonaws.com",
      "port": "443",
      "bucket": "my-bucket",
      "region": "us-east-1"
    }
  },
  "sessions": {
    "backend": {
      "driver": "filesystem",
      "storage_name": "sessions",
      "secret": "change-me"
    },
    "scheduler": {
      "driver": "redis",
      "host_id": "redis.r1",
      "secret": "change-me"
    },
    "admin": {
      "driver": "database",
      "host_id": "postgresql.p1",
      "secret": "change-me"
    }
  },
  "mail": {
    "dkim_private": "/path/to/dkim_private.pem",
    "dkim_selector": "mail",
    "host": "example.com"
  },
  "mimetypes": {
    "text/html": ["html", "htm"],
    "text/css": ["css"],
    "application/json": ["json"],
    "application/javascript": ["js"],
    "image/png": ["png"],
    "image/jpeg": ["jpeg", "jpg"]
  }
}
```

## System Requirements

* **Glibc** 2.35 or higher
* **GCC** 9.5.0 or higher (10+ enables the static analyzer in debug builds)
* **CMake** 3.12.4 or higher
* **PCRE** 8.43 (Regular Expression Library)
* **Zlib** 1.2.11 (data compression library)
* **OpenSSL** 1.1.1k or higher (3.5+ required for HTTP/3)
* **LibXml2** 2.9.13
* **libidn2** (internationalized domain names)
* **libunistring** (Unicode string handling)
* **Argon2** (linked by the framework shared library)

### Optional Dependencies:
* **PostgreSQL** development libraries (for PostgreSQL support)
* **MySQL/MariaDB** development libraries (for MySQL support)
* **hiredis** (for Redis sessions and caching)
* **SQLite 3** development libraries (for embedded SQLite support)

## Building the Project

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake (enable the databases you need)
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes \
         -DINCLUDE_SQLITE=yes

# HTTP/3 over QUIC is opt-in and needs OpenSSL >= 3.5
# cmake .. -DCMAKE_BUILD_TYPE=Release -DINCLUDE_HTTP3=yes

# Build
cmake --build . -j4

# Run
<workspaceFolder>/build/exec/cwfr -c <path_to_config>/config.json

# Apply database migrations (positional args — see the migrate CLI)
<workspaceFolder>/build/exec/migrate up <path_to_config>/config.json postgresql.p1 s1
```

### Build Modes:
* **Release** - optimized version for production
* **Debug** - debug symbols, AddressSanitizer + leak detection, `-fanalyzer` on GCC >= 10
* **RelWithDebInfo** - optimized version with debug information (also sanitized)
* **ThreadSanitizer** - `-DSANITIZE=thread` (mutually exclusive with ASan)
* **Tests** - `-DBUILD_TESTS=yes` builds the core unit/integration tests

## Key Highlights

### Performance
* **Zero-copy** operations where possible
* **Connection pool** to databases to minimize overhead
* **Epoll** for efficient I/O multiplexing
* **UDP GSO** batching for QUIC datagrams
* **SSO (Small String Optimization)** for strings
* **Lazy-loading** of configuration and modules

### Security
* **Prepared statements** for SQL injection protection
* **Input data validation** at all levels
* **HTTPS/TLS** with modern cipher suites (TLS 1.3 for HTTP/3)
* **Secure cookie** with httpOnly and sameSite flags
* **Rate limiting** for DDoS protection
* **DKIM** signatures for email authentication
* **Password hashing** using cryptographically strong functions

### Scalability
* **Horizontal scaling** through multiple worker processes
* **Vertical scaling** through multithreading
* **Virtual host support** for multiple domains
* **Broadcasting** for efficient WebSocket message distribution
* **Asynchronous request processing**

### Developer Experience
* **Modular architecture** - connect only the components you need
* **Dynamic loading** of handlers without server restart
* **Migration system** for database versioning
* **ORM-like models** for simplified data operations
* **Middleware system** for logic reuse
* **Template Engine** for dynamic content generation
* **Centralized configuration** via JSON

## Use Cases

The framework is suitable for developing:

* **REST API** - high-performance APIs for mobile and web applications
* **Real-time applications** - chats, notifications, live updates via WebSocket
* **Web services** - microservices and monolithic applications
* **API Gateway** - request routing and proxying
* **Admin panels** - with authentication, RBAC, and CRUD operations
* **File upload services** - with local and cloud storage support
* **Email services** - transactional email sending with DKIM

## Documentation

Full documentation is available at: [https://cwebframework.tech/en/introduction.html](https://cwebframework.tech/en/introduction.html)
