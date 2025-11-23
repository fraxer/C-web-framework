---
# https://vitepress.dev/reference/default-theme-home-page
layout: home
title: C Web Framework — high-performance C web framework
titleTemplate: C Web Framework
description: Fast C web framework for Linux. HTTP/1.1, WebSocket, PostgreSQL, MySQL, Redis, ORM, authentication, S3 storage. Build scalable web applications.

hero:
  name: "The C framework\nfor web"
  tagline: Affordable, productive and versatile platform for creating web resources
  actions:
    - theme: brand
      text: Docs
      link: /en/introduction
    - theme: alt
      text: Github
      link: https://github.com/fraxer/C-web-framework
    - theme: alt
      text: Examples
      link: /en/examples-req-res

features:
  - title: High Performance
    icon: 💨
    details: Event-driven architecture on epoll, multithreading, connection pool and zero-copy operations
    link: /en/introduction
    linkText: Get started
  - title: HTTP/1.1 and WebSocket
    icon: 🌐
    details: Full HTTP/1.1 support with middleware, filters and gzip. WebSocket with broadcasting and channels
    link: /en/routing
    linkText: Routing
  - title: Databases
    icon: 🏛️
    details: PostgreSQL, MySQL, Redis with unified API. ORM models, Query Builder, migrations and prepared statements
    link: /en/db
    linkText: Database
  - title: Authentication and RBAC
    icon: 🔐
    details: Built-in authorization system, sessions (files/Redis), password hashing and role-based access
    link: /en/ssl-certs
    linkText: Security
  - title: File Storage
    icon: 📁
    details: Local storage and S3-compatible services. File upload via multipart/form-data
    link: /en/introduction
    linkText: To documentation
  - title: Email with DKIM
    icon: 📧
    details: SMTP client with DKIM signature support for sender authentication
    link: /en/introduction
    linkText: To documentation
  - title: Flexible Configuration
    icon: 🛠️
    details: JSON configuration, virtual hosts, dynamic handler loading from .so libraries
    link: /en/config
    linkText: Configuration
  - title: Hot Reload
    icon: 🔄
    details: Application update without stopping the server and interrupting request processing
    link: /en/hot-reload
    linkText: Reload
---

