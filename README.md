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

### WebSocket
* **WebSocket server** - full WebSocket protocol support
* **Broadcasting** - message broadcasting system for client groups
* **Custom channels** - named channel creation with recipient filtering
* **JSON over WebSocket** - built-in JSON message support
* **WebSocket middleware** - middleware handler system for WebSocket

### Databases
* **PostgreSQL** - native support with prepared statements
* **MySQL** - native support with SQL injection protection
* **Redis** - Redis support for caching and sessions
* **ORM-like operations** - model system for working with tables
* **Migrations** - database schema versioning system with up/down migrations
* **Query Builder** - safe SQL query building with parameterization
* **Prepared Statements** - SQL injection protection at framework level

### Authentication and Security
* **Authentication system** - built-in registration and authorization system
* **Sessions** - support for file-based sessions and Redis sessions
* **Password hashing** - secure password storage using salt and hash
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
* **SMTP client** - email sending via SMTP
* **DKIM signature** - DKIM signature support for sender authentication
* **Email templates** - email template support

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
* **Multithreading** - multiple worker thread support
* **Connection pool** - database connection reuse
* **Rate Limiting** - request rate limiting (DDoS protection)
* **Hot reload** - application updates without server restart

### Utilities and Data Structures
* **String (str_t)** - dynamic strings with SSO optimization
* **Array** - dynamic arrays
* **HashMap/Map** - associative arrays for fast lookup
* **JSON structures** - working with JSON as objects
* **Logging** - logging system with different levels
* **Base64** - Base64 encoding/decoding

### Additional Features
* **Modular architecture** - easy component connection and disconnection
* **Dynamic loading** - handler loading from shared libraries (.so)
* **JSON configuration** - centralized configuration in config.json
* **MIME types** - automatic content type detection
* **Domains and virtual hosts** - multiple domain support on a single server

## Project Architecture

```
project/
├── core/                          # Framework core
│   ├── framework/                 # Framework components
│   │   ├── database/             # Database operations (PostgreSQL, MySQL, Redis)
│   │   ├── model/                # ORM model system
│   │   ├── session/              # Session system
│   │   ├── storage/              # File storage (FS, S3)
│   │   ├── view/                 # Template engine
│   │   ├── middleware/           # Middleware system
│   │   └── query/                # Query builder
│   ├── protocols/                # Protocol implementations
│   │   ├── http/                 # HTTP/1.1 server and client
│   │   ├── websocket/            # WebSocket server
│   │   └── smtp/                 # SMTP client, DKIM
│   ├── src/                      # Core components
│   │   ├── broadcast/            # Broadcasting system
│   │   ├── connection/           # Connection management
│   │   ├── server/               # HTTP server
│   │   ├── route/                # Routing system
│   │   ├── thread/               # Multithreading
│   │   └── multiplexing/         # Epoll multiplexing
│   └── misc/                     # Utilities
│       ├── str.h                 # Dynamic strings
│       ├── array.h               # Arrays
│       ├── hashmap.h/map.h       # Associative arrays
│       ├── json.h                # JSON parser
│       ├── log.h                 # Logging
│       └── gzip.h                # Gzip compression
│
└── app/                          # User application
    ├── routes/                   # HTTP and WebSocket handlers
    │   ├── auth/                # Authentication (login, registration)
    │   ├── index/               # Main page
    │   ├── ws/                  # WebSocket handlers
    │   ├── files/               # File operations
    │   ├── models/              # Model API operations
    │   ├── email/               # Email sending
    │   └── db/                  # Database examples
    ├── models/                   # Data models
    │   ├── user.c               # User model
    │   ├── role.c               # Role model
    │   ├── permission.c         # Permission model
    │   └── *view.c              # View models for JOIN queries
    ├── middlewares/              # Custom middleware
    │   ├── httpmiddlewares.c    # HTTP middleware (auth, rate limit)
    │   └── wsmiddlewares.c      # WebSocket middleware
    ├── migrations/               # Database migrations
    │   ├── s1/                  # Migrations for server s1
    │   └── s2/                  # Migrations for server s2
    ├── broadcasting/             # Broadcasting channels
    │   └── mybroadcast.c        # Broadcasting channel example
    ├── auth/                     # Authentication module
    │   ├── auth.c               # Authentication functions
    │   ├── password_validator.c # Password validation
    │   └── email_validator.c    # Email validation
    ├── contexts/                 # Request contexts
    │   ├── httpcontext.c        # HTTP context
    │   └── wscontext.c          # WebSocket context
    └── views/                    # Templates
        ├── index.tpl            # Main page
        └── header.tpl           # Header

config.json                       # Application configuration
```

## Usage Examples

### HTTP Handler

```c
#include "http.h"

void my_handler(httpctx_t* ctx) {
    // Get query parameter
    const char* name = ctx->request->query(ctx->request, "name");

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
#include "broadcast.h"

void ws_join_channel(wsctx_t* ctx) {
    // Join broadcasting channel
    broadcast_add("my_channel", ctx->request->connection,
                  user_id_struct, send_callback);

    ctx->response->send_data(ctx, "Joined channel");
}

void ws_send_message(wsctx_t* ctx) {
    websockets_protocol_resource_t* protocol =
        (websockets_protocol_resource_t*)ctx->request->protocol;

    char* message = protocol->get_payload(protocol);

    // Send message to everyone in channel
    broadcast_send("my_channel", ctx->request->connection,
                   message, strlen(message), filter_struct, compare_func);

    free(message);
}
```

### Database Operations

```c
#include "db.h"
#include "model.h"

void get_users(httpctx_t* ctx) {
    // Parameterized query (SQL injection protection)
    mparams_create_array(params,
        mparam_int(id, 123),
        mparam_text(email, "user@example.com")
    );

    dbresult_t* result = dbquery("postgresql",
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
    file_content_t content = ctx->request->payload_filef(ctx->request, "myfile");

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
    // Get file from local storage
    file_t file = storage_file_get("local", "/path/to/file.txt");

    if (!file.ok) {
        ctx->response->send_data(ctx->response, "File not found");
        return;
    }

    // Send file to client
    ctx->response->send_file(ctx->response, file.path);
    file.close(&file);
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

The framework uses `config.json` for centralized configuration:

```json
{
  "main": {
    "workers": 4,
    "threads": 2,
    "reload": "hard",
    "buffer_size": 16384,
    "client_max_body_size": 110485760,
    "tmp": "/tmp",
    "gzip": ["text/html", "text/css", "application/json", "application/javascript"]
  },
  "migrations": {
    "source_directory": "/path/to/migrations"
  },
  "servers": {
    "s1": {
      "domains": ["example.com", "*.example.com", "(api|www).example.com"],
      "ip": "127.0.0.1",
      "port": 443,
      "root": "/var/www/html",
      "ratelimits": {
        "default": { "burst": 15, "rate": 15 },
        "strict": { "burst": 1, "rate": 0 }
      },
      "http": {
        "ratelimit": "default",
        "redirects": {
          "/old-path": "/new-path"
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
    "driver": "storage",
    "host_id": "redis.r1",
    "storage_name": "sessions",
    "lifetime": 3600
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
* **GCC** 9.5.0 or higher
* **CMake** 3.12.4 or higher
* **PCRE** 8.43 (Regular Expression Library)
* **Zlib** 1.2.11 (data compression library)
* **OpenSSL** 1.1.1k or higher
* **LibXml2** 2.9.13

### Optional Dependencies:
* **PostgreSQL** development libraries (for PostgreSQL support)
* **MySQL/MariaDB** development libraries (for MySQL support)
* **Redis** (for Redis sessions and caching)

## Building the Project

```bash
# Create build directory
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release \
         -DINCLUDE_POSTGRESQL=yes \
         -DINCLUDE_MYSQL=yes \
         -DINCLUDE_REDIS=yes

# Build
cmake --build . -j4

# Run
<workspaceFolder>/build/exec/cpdy
```

### Build Modes:
* **Release** - optimized version for production
* **Debug** - version with debug symbols and AddressSanitizer
* **RelWithDebInfo** - optimized version with debug information

## Key Highlights

### Performance
* **Zero-copy** operations where possible
* **Connection pool** to databases to minimize overhead
* **Epoll** for efficient I/O multiplexing
* **SSO (Small String Optimization)** for strings
* **Lazy-loading** of configuration and modules

### Security
* **Prepared statements** for SQL injection protection
* **Input data validation** at all levels
* **HTTPS/TLS** with modern cipher suites
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

