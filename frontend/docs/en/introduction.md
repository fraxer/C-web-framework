---
outline: deep
description: C Web Framework — high-performance C web framework for Linux. HTTP/1.1, WebSocket, databases, authentication, S3 storage and email.
---

# Introduction

C Web Framework is a high-performance framework written in C, designed for developing web applications on Linux. Built on an event-driven epoll architecture that efficiently handles a large number of concurrent connections.

The framework provides a complete set of tools for developing modern web applications: from a basic HTTP server to database integration, WebSocket connections, authentication system, and file storage.

## Features

### HTTP
* Full HTTP/1.1 support
* Built-in HTTP client: HTTPS (TLS 1.2+), keep-alive connection pooling, automatic redirect following
* Flexible routing system with dynamic parameters
* Virtual hosts with regex domains and IDN support
* Middleware for HTTP and WebSocket requests
* Built-in filters: chunked encoding, range requests, gzip, cache control
* Multipart/form-data handling and file uploads
* Full cookie support with secure, httpOnly, sameSite
* Automatic gzip compression for supported content types
* TLS/SSL with configurable cipher suites
* Redirects with regular expressions and capture-group substitution

### WebSocket
* Full WebSocket protocol support
* Broadcasting system for client groups
* Named channels with recipient filtering
* Built-in JSON message support
* Middleware for WebSocket

### Databases
* PostgreSQL — native support with prepared statements
* MySQL — native support with SQL injection protection
* SQLite — embedded database that works directly with a file, no server required
* Redis — support for caching and sessions
* ORM-like operations — model system for working with tables
* Migrations — database schema versioning system
* Query Builder — safe SQL query construction

### Authentication and Security
* Built-in registration and authorization system
* Sessions on files, Redis, and database (secrets protected with AES-256-GCM)
* Password hashing with salt
* Validators for email, passwords, and other data
* Authentication middleware for route protection
* RBAC — role-based access control system
* Rate Limiting — request frequency limiting (DDoS protection)

### File Storage
* Local storage — file system operations
* S3-compatible storage — integration with S3 and compatible services
* File operations: create, read, update, delete
* Multipart upload handling with storage saving

### Email
* SMTP client for sending emails
* DKIM signature support for sender authentication
* Email templates

### Template Engine
* Built-in template engine
* Variables and loops for dynamic HTML generation
* Model integration

### Internationalization (i18n)
* Built on top of the standard gettext library
* Translations organized by domains to separate localization between modules
* Automatic language detection from the query string or Accept-Language header
* Plural forms and placeholder substitution
* Fallback chain: requested language → en → message identifier

### JSON
* High-performance JSON parser
* Data serialization to JSON
* Thread-safe operations
* Correct handling of Unicode and surrogate pairs

### Task Manager
* Async tasks — one-time tasks executed in a background thread
* Scheduled tasks — interval, daily, weekly, monthly
* Two dedicated threads: async worker and scheduler worker
* Tasks loaded as dynamic .so modules

### Performance and Scalability
* Event-driven architecture on epoll
* Multiple worker threads support
* Connection pool for databases
* Hot reload without stopping the server
* Zero-copy operations where possible

### Utilities and Data Structures
* str_t — dynamic strings with SSO optimization
* Array — dynamic arrays
* HashMap/Map — associative arrays
* JWT — token creation and verification
* UUID, Base64, SHA-1/256, AES-256-GCM
* Logging system with different levels

## Software Requirements

* **Glibc** 2.35 or higher
* **GCC** 9.5.0 or higher
* **CMake** 3.12.4 or higher
* **PCRE** 8.43 (regular expression library)
* **Zlib** 1.2.11 (data compression library)
* **OpenSSL** 1.1.1k or higher
* **LibXml2** 2.9.13

### Optional Dependencies
* **PostgreSQL** — development libraries (for PostgreSQL support)
* **MySQL/MariaDB** — development libraries (for MySQL support)
* **Redis** — for sessions and caching

## Use Cases

The framework is suitable for developing:

* **REST API** — high-performance APIs for mobile and web applications
* **Real-time applications** — chats, notifications, live updates via WebSocket
* **Web services** — microservices and monolithic applications
* **API Gateway** — request routing and proxying
* **Admin panels** — with authentication, RBAC, and CRUD operations
* **File upload services** — with local and cloud storage support
* **Email services** — transactional email sending with DKIM
