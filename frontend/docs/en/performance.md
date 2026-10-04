# Performance measurements

JSON: updated 2026-10-04. One application worker and eight execution threads.

Database measurements: [PostgreSQL — selecting 20 rows from 1,000,000 records](#postgresql-benchmark). PostgreSQL and static-file results use their own deployments, described in those sections.

## JSON request processing

### Nine-framework series — 2026-10-03, cwfr updated 2026-10-04

The POST `/count` handler receives exactly **1,048,576 bytes (1 MiB)** of UTF-8 JSON, fully parses the document and returns `{"count":4097}`. The object contains nested objects/arrays, Unicode, escaped strings and mixed value types. Top-level keys are counted; neither the result nor the JSON tree is cached between requests.

### PC characteristics

Intel Core i7-12700F, Linux host network.

Applications and nginx:
* CPU 0–7 (four physical P cores with SMT),
* 2 GiB per application container
* nofile 65536

wrk:
* eight threads on CPU 8–15

The shared nginx has two workers and one upstream address per application, limits the body to 2 MiB, does not buffer bodies and does not cache responses.\
TLS, compression, access logs and databases are disabled.

cwfr keeps the request body in memory.\
Rails uses ActionController::Metal.\
Laravel uses the parsed request JSON plus a json_validate pass.\
facil.io uses native FIOBJ JSON;\
Kore keeps the request body in memory and builds the entire Jansson tree in a background task.

**90 measured runs in one series:** nine frameworks, two concurrency levels and five repeats.

The entire comparison was repeated with RSS collection: 30 s initial warmup, 5 s before each run, five 15 s repeats at 16/64 keep-alive connections, shuffled with seed 20261003.

### 16 connections

| Framework | req/s | min–max req/s | p50, ms | p95, ms | p99, ms | App CPU/request, µs | Gateway CPU/request, µs | RSS median, MiB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| cwfr (C) | 1,997 | 1,835–2,055 | 7.85 | 11.28 | 13.01 | 3,211 | 449 | 60.82 |
| Spring Boot (Java) | 1,723 | 1,721–1,759 | 8.97 | 12.24 | 14.40 | 4,107 | 381 | 586.71 |
| Laravel (PHP) | 840 | 758–874 | 18.86 | 22.57 | 29.93 | 9,023 | 446 | 293.07 |
| facil.io (C) | 734 | 474–862 | 16.35 | 1,120.07 | 1,637.76 | 6,414 | 378 | 34.00 |
| Fiber (Go) | 638 | 568–657 | 23.14 | 46.09 | 61.49 | 11,632 | 374 | 82.55 |
| Fastify (JavaScript) | 618 | 593–626 | 22.55 | 62.03 | 91.59 | 11,682 | 420 | 490.80 |
| Ruby on Rails (Ruby) | 283 | 196–286 | 54.03 | 69.68 | 94.19 | 4,108 | 318 | 226.82 |
| Kore + Jansson (C) | 217 | 177–218 | 73.24 | 91.56 | 104.44 | 20,635 | 314 | 34.18 |
| Django (Python) | 194 | 179–197 | 81.26 | 102.92 | 114.28 | 5,523 | 306 | 86.62 |

### 64 connections

| Framework | req/s | min–max req/s | p50, ms | p95, ms | p99, ms | App CPU/request, µs | Gateway CPU/request, µs | RSS median, MiB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| cwfr (C) | 1,908 | 1,809–1,978 | 33.40 | 39.58 | 46.13 | 3,352 | 470 | 84.82 |
| Spring Boot (Java) | 1,727 | 1,397–1,778 | 37.46 | 41.31 | 43.92 | 4,124 | 379 | 579.70 |
| facil.io (C) | 828 | 626–925 | 68.22 | 1,157.88 | 1,896.61 | 6,802 | 378 | 37.80 |
| Laravel (PHP) | 823 | 680–865 | 76.68 | 83.70 | 112.99 | 9,082 | 474 | 315.58 |
| Fiber (Go) | 679 | 652–683 | 75.31 | 325.90 | 493.56 | 11,145 | 381 | 200.59 |
| Fastify (JavaScript) | 595 | 475–627 | 91.26 | 314.57 | 568.00 | 12,648 | 417 | 540.44 |
| Ruby on Rails (Ruby) | 267 | 189–271 | 236.56 | 274.93 | 295.04 | 4,350 | 315 | 241.94 |
| Kore + Jansson (C) | 210 | 186–217 | 295.67 | 388.88 | 448.16 | 21,976 | 320 | 73.18 |
| Django (Python) | 179 | 130–183 | 347.78 | 374.67 | 517.21 | 6,000 | 327 | 88.33 |

The common cgroup memory limit triggered during this series: facil.io (C): memory.events max=165877, oom=0, oom_kill=0.
Reclaim may affect latency; its precise contribution was not isolated. These counters do not represent process RSS.

### Memory and measurement limits

RSS is the sum of VmRSS for all application processes in the container, including supervisors; threads are not summed separately. Samples are taken every **200 ms during measured load**. The tables show the median of the five per-run RSS medians, in MiB (1,048,576 bytes).

### Measured versions

- **cwfr (C)**: 5da69b013eec43a06f9cb61f4e2c681fadeb3f9e; Release; 1 × (1 event worker + 8 handler threads)
- **Fastify (JavaScript)**: v24.21.0; Fastify 5.6.2; 1 × 8 Worker Threads; V8 old/young heap limits 64/8 MiB per thread
- **Django (Python)**: Python 3.13.16; Django 5.2.7; Gunicorn 23.0.0; 1 × 8 gthread
- **Spring Boot (Java)**: Spring Boot 3.5.7; openjdk version "21.0.12.1" 2026-08-18 LTS; 1 × 8 Tomcat threads
- **Fiber (Go)**: go1.25.14; Fiber 3.0.0; encoding/json; 1 × GOMAXPROCS=8
- **Ruby on Rails (Ruby)**: Ruby 3.4.11; Rails 8.0.2; Puma 6.6.1; 1 × 8 threads
- **Laravel (PHP)**: PHP 8.4.26 ZTS; Laravel 12.69.3; Octane v2.20.0; FrankenPHP v1.12.7 PHP 8.4.26 Caddy v2.11.4 h1:XKxkMTgNSizEvKG6QHue6cAsFOteU2qA61w2tKkCWi0=; 1 × 8 PHP worker threads; cached request JSON + json_validate
- **facil.io (C)**: facil.io 0.7.6; 512a354dbd31e1895647df852d1565f9d408ed91; native FIOBJ JSON; 1 process + 8 reactor/handler threads
- **Kore + Jansson (C)**: Kore 4.3.0; source SHA256 94f767eb113f2c97a133b9609c78c4e3795c420dc8726154edce49965df6b8ed; Jansson 2.14-2; 1 worker + 8 task threads
- **gateway**: nginx version: nginx/1.28.0; 2 workers

## PostgreSQL: raw SQL SELECT {#postgresql-benchmark}

Series date: 2026-10-04.

CWFR core: `5da69b013eec43a06f9cb61f4e2c681fadeb3f9e`. `main.body_store`: `mode: "auto"`, `file_threshold: 5000000` bytes. The small POST body stays in memory; the body limit is 2097152 bytes.

POST `/db` accepts `{"category":42,"after":1000}` and returns twenty ordered rows with id, category, name, price_cents and active.\
The dedicated PostgreSQL 17.6 contains **1,000,000 synthetic records** with a composite B-tree index on `(category, id)`.\
Both values are passed through driver bind parameters. No ORM, application response cache, TLS or PgBouncer is used.\
SQL execution, reading all twenty rows, type conversion and JSON serialization are included.

```sql
SELECT id, category, name, price_cents, active
FROM bench_items
WHERE category = $1::integer AND id > $2::integer
ORDER BY id
LIMIT 20;
```

The query and projection are equivalent in all applications.\
Placeholder syntax is adapted to the driver.\
The eight client threads use eight different fixed parameter pairs spanning the whole table, one pair per thread.\
Every response is compared against the full expected twenty-row document for that thread.\
An empty result and invalid parameters were checked on the direct application port and through the proxy.

### PostgreSQL · 16 connections

| Framework | req/s | min–max req/s | p99, ms | App CPU/request, µs | DB CPU/request, µs | App anon median, MiB | App working-set peak, MiB | App RSS median, MiB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Fiber (Go) | 52,763 | 50,348–54,180 | 0.82 | 53 | 105 | 7.5 | 19.2 | 17.32 |
| facil.io (C) | 48,180 | 47,251–48,524 | 0.68 | 63 | 100 | 16.3 | 19.2 | 23.32 |
| cwfr (C) | 46,070 | 43,551–49,423 | 2.27 | 72 | 106 | 11.5 | 16.2 | 23.09 |
| Kore + Jansson (C) | 41,388 | 38,724–41,659 | 0.89 | 87 | 99 | 7.6 | 15.1 | 19.21 |
| Spring Boot (Java) | 40,291 | 38,462–44,875 | 1.58 | 87 | 109 | 373.5 | 416.3 | 396.48 |
| Fastify (JavaScript) | 31,829 | 28,445–33,259 | 2.07 | 144 | 94 | 356.6 | 373.3 | 405.11 |
| Laravel (PHP) | 5,884 | 5,835–5,960 | 4.40 | 1,193 | 130 | 357.0 | 577.2 | 453.18 |
| Django (Python) | 2,488 | 2,365–2,497 | 9.36 | 462 | 78 | 45.2 | 68.2 | 72.71 |
| Ruby on Rails (Ruby) | 1,952 | 1,735–2,089 | 57.46 | 606 | 104 | 63.9 | 106.4 | 78.84 |

### PostgreSQL · 64 connections

| Framework | req/s | min–max req/s | p99, ms | App CPU/request, µs | DB CPU/request, µs | App anon median, MiB | App working-set peak, MiB | App RSS median, MiB |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| Fiber (Go) | 48,916 | 48,244–49,243 | 1.94 | 57 | 112 | 7.8 | 19.9 | 17.50 |
| cwfr (C) | 47,929 | 47,218–48,603 | 3.18 | 69 | 109 | 11.5 | 16.4 | 23.08 |
| facil.io (C) | 44,055 | 41,626–46,317 | 2.40 | 65 | 111 | 16.1 | 19.3 | 23.21 |
| Spring Boot (Java) | 43,115 | 37,807–43,117 | 2.26 | 82 | 109 | 372.7 | 416.0 | 395.71 |
| Kore + Jansson (C) | 38,835 | 37,157–41,290 | 3.25 | 89 | 111 | 8.0 | 15.4 | 19.58 |
| Fastify (JavaScript) | 34,098 | 30,049–36,747 | 6.03 | 161 | 108 | 354.2 | 373.6 | 402.64 |
| Laravel (PHP) | 5,561 | 5,105–5,844 | 14.90 | 1,245 | 158 | 482.7 | 670.6 | 578.88 |
| Django (Python) | 2,497 | 2,406–2,503 | 30.04 | 462 | 79 | 45.2 | 68.5 | 72.77 |
| Ruby on Rails (Ruby) | 1,965 | 1,862–2,060 | 302.08 | 612 | 109 | 61.7 | 103.6 | 76.59 |

### Memory before and after load

Idle snapshots taken before the explicit warmup and after warming all frameworks are retained, as are snapshots before/after each run.\
Database memory includes the server and idle connections of all frameworks.\
In the raw data, current/anon/working-set medians and peaks exist separately for the application, database and proxy; each PID's RSS and per-container sums are stored separately.

| Container | Idle before warmup, MiB | After warmup, MiB | After last run, MiB | Load peak, MiB |
|---|---:|---:|---:|---:|
| cwfr (C) | 12.9 | 12.9 | 15.7 | 16.4 |
| Fastify (JavaScript) | 370.5 | 256.2 | 372.7 | 373.6 |
| Django (Python) | 65.9 | 66.0 | 67.8 | 68.5 |
| Spring Boot (Java) | 388.3 | 412.4 | 397.9 | 416.3 |
| Fiber (Go) | 16.8 | 16.9 | 18.3 | 19.9 |
| Ruby on Rails (Ruby) | 99.0 | 100.4 | 87.0 | 106.4 |
| Laravel (PHP) | 235.8 | 331.0 | 671.2 | 670.6 |
| facil.io (C) | 15.4 | 15.4 | 18.4 | 19.3 |
| Kore + Jansson (C) | 16.1 | 12.6 | 13.9 | 15.4 |
| postgres | 208.8 | 209.7 | 209.6 | 217.4 |
| gateway | 9.6 | 10.0 | 11.7 | 14.1 |

### Measured PostgreSQL versions

- **cwfr**: {"core_source": "5da69b013eec43a06f9cb61f4e2c681fadeb3f9e", "libpq": "libpq5:amd64\t18.6-0ubuntu0.26.04.1"}
- **fastify**: {"node":"v24.21.0","fastify":"5.6.2","pg":"8.16.3"}
- **django**: 3.13.16 5.2.7 3.2.13
- **spring**: openjdk version "21.0.12.1" 2026-08-18 LTS
OpenJDK Runtime Environment Temurin-21.0.12.1+1 (build 21.0.12.1+1-LTS)
OpenJDK 64-Bit Server VM Temurin-21.0.12.1+1 (build 21.0.12.1+1-LTS, mixed mode, sharing)
- **fiber**: {"fiber": "3.0.0", "pgx": "5.7.6", "go": "go1.25.14"}
- **rails**: 3.4.11; 8.0.2; 1.6.2; 6.6.1
- **laravel**: PHP 8.4.26; Laravel 12.69.3; Octane v2.20.0; pdo_pgsql

PDO Driver for PostgreSQL => enabled
PostgreSQL(libpq) Version => 15.19
- **facil**: facil.io 0.7.6; 512a354dbd31e1895647df852d1565f9d408ed91; libpq/Jansson; 1 process + 8 reactor/handler threads; libjansson4:amd64	2.14-2
libpq5:amd64	15.19-0+deb12u1
- **kore**: Kore 4.3.0; SHA256 94f767eb113f2c97a133b9609c78c4e3795c420dc8726154edce49965df6b8ed; libpq/Jansson; 1 worker + 8 task threads; libjansson4:amd64	2.14-2
libpq5:amd64	15.19-0+deb12u1
- **postgres**: postgres (PostgreSQL) 17.6 (Debian 17.6-2.pgdg12+1)
- **gateway**: nginx version: nginx/1.28.0
- **spring_jars**: ["BOOT-INF/lib/spring-boot-3.5.7.jar", "BOOT-INF/lib/spring-boot-autoconfigure-3.5.7.jar", "BOOT-INF/lib/HikariCP-6.3.3.jar", "BOOT-INF/lib/postgresql-42.7.8.jar", "BOOT-INF/lib/spring-boot-jarmode-tools-3.5.7.jar"]

## Static files: cwfr and nginx

Direct HTTP/1.1 keep-alive requests, four workers, four physical P cores (CPU 0,2,4,6), 512 MiB per server, 16 KiB buffers, page cache, no gzip/access log. The client uses CPU 8,10,12,14.\
Five 15 s repeats with 1 s warmup per scenario.\
Files: 200 B, 20 KiB, 1 MiB.\
Ranges: prefix half, suffix quarter, multipart of the first and last quarter.\
Below are the final cwfr and nginx results at 64 connections.\
The home section allows all four connection counts and range modes. The settings differ from the JSON series.

| File | Response | cwfr, req/s | nginx, req/s | cwfr p99, ms | nginx p99, ms |
|---|---|---:|---:|---:|---:|
| 200 B | full | 480,307 | 500,375 | 0.263 | 0.667 |
| 200 B | prefix_half | 464,506 | 492,762 | 0.277 | 0.206 |
| 200 B | suffix_quarter | 463,087 | 491,316 | 0.257 | 0.207 |
| 200 B | multipart_quarters | 388,871 | 400,835 | 0.273 | 0.210 |
| 20 KiB | full | 432,924 | 464,927 | 0.366 | 1.138 |
| 20 KiB | prefix_half | 428,125 | 469,011 | 0.313 | 0.194 |
| 20 KiB | suffix_quarter | 431,635 | 473,936 | 0.265 | 0.200 |
| 20 KiB | multipart_quarters | 333,483 | 379,143 | 0.364 | 0.247 |
| 1 MiB | full | 25,798 | 25,781 | 2.508 | 2.514 |
| 1 MiB | prefix_half | 51,016 | 50,679 | 1.267 | 1.265 |
| 1 MiB | suffix_quarter | 110,552 | 101,434 | 1.786 | 0.687 |
| 1 MiB | multipart_quarters | 51,481 | 52,062 | 1.273 | 1.252 |
