# About framework

The C web framework is a high performance, event-driven web server and application framework for Linux, written in C. It is designed to be fast and scalable, with support for multithreading, websockets, gzip compression, and interaction with databases such as PostgreSQL, MySQL, and Redis.

The framework is built on top of the epoll event-driven architecture, which allows it to handle a large number of simultaneous connections efficiently. It also provides support for database migrations, routing, redirects, and dynamic reloading of the application.

The framework is designed to be easy to use and flexible, with a modular architecture that allows developers to easily add or customize functionality as needed. It includes support for a wide range of protocols, including HTTP/1.1, WebSockets, and more.

Documentation for the framework is available [here](https://cwebframework.tech/en/introduction.html).


## Possibilities

* HTTP/1.1
* WebSockets
* Multithreading
* Broadcasting by websockets
* Gzip compression
* Interaction with databases PostgreSQL, MySQL, Redis
* Database migrations
* Routing
* Redirects
* Reload application on the fly
* Event Driven Architecture (epoll)

## Software requirements

* Glibc 2.35 library
* Compiler GCC 9.5.0
* CMake 3.12.4 assembler
* PCRE Regular Expression Library 8.43
* Zlib data compression library 1.2.11
* Library Openssl 1.1.1k

