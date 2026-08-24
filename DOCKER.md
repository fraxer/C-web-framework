# Docker for cwfr

This document describes building and running cwfr in Docker containers.

## Table of Contents

- [Requirements](#requirements)
- [Building the image](#building-the-image)
- [Running the container](#running-the-container)
- [Docker Compose](#docker-compose)
- [PostgreSQL](#postgresql)
- [Migrations](#migrations)
- [Volumes](#volumes)
- [Production mode](#production-mode)
- [Troubleshooting](#troubleshooting)

## Requirements

- Docker 20.10+
- Docker Compose 2.0+ (for docker-compose.yml)

## Building the image

The `Dockerfile` is multi-stage: the `builder` stage compiles the framework
from `backend/` (Release, PostgreSQL/MySQL/Redis/SQLite enabled) on Ubuntu,
the `runtime` stage installs only the shared libraries needed at runtime,
creates the unprivileged `cwfr` user and copies the frontend build
(`frontend/docs/.vitepress/dist`).

Basic image build:

```bash
docker build -t cwfr:latest .
```

Build with version tag:

```bash
docker build -t cwfr:1.0.0 .
```

Build only runtime part (using pre-built binaries):

```bash
docker build --target runtime -t cwfr:runtime .
```

## Running the container

The image has `ENTRYPOINT ["cwfr"]`, so arguments are passed straight to the
binary:

```bash
docker run -d \
  --name cwfr-app \
  -p 80:8080 \
  -v $(pwd)/config.json:/opt/cwfr/config.json:ro \
  cwfr:latest -c /opt/cwfr/config.json -f
```

Run with the built frontend mounted instead of the baked-in copy:

```bash
docker run -d \
  --name cwfr-app \
  -p 80:8080 \
  -v $(pwd)/config.json:/opt/cwfr/config.json:ro \
  -v $(pwd)/frontend/docs/.vitepress/dist:/opt/cwfr/frontend:ro \
  cwfr:latest -c /opt/cwfr/config.json -f
```

## Docker Compose

Recommended way to run the application. The compose file defines two services:
`app` (cwfr server) and `postgres` (PostgreSQL 18), both on the
`cwfr-network` network.

```bash
# Start all services (app waits for postgres to become healthy)
docker compose up -d

# Stop
docker compose down

# View logs
docker compose logs -f app

# Rebuild after changes
docker compose up -d --build

# Rebuild from scratch
docker compose build --no-cache app && docker compose up -d
```

Convenience wrappers are available via `make -f Makefile.docker`
(`dc-up`, `dc-down`, `dc-logs`, `dc-rebuild`, `dc-ps`).

## PostgreSQL

The `postgres` service is preconfigured with:

| Setting | Value |
|---------|-------|
| Image | `postgres:18` |
| User / password / database | `cwfr` / `cwfr` / `cwfr` |
| Data volume | named volume `postgres-data` |
| Host port | `5432` |
| Hostname inside the network | `postgres` (container name `cwfr-postgres`) |

To use it from cwfr, add a database entry to the `databases` section of
`config.json` and point its host at `postgres`:

```json
"databases": {
  "postgresql.p1": {
    "driver": "postgresql",
    "host": "postgres",
    "port": 5432,
    "user": "cwfr",
    "password": "cwfr",
    "dbname": "cwfr"
  }
}
```

For development, port `5432` is published to the host; remove the `ports`
entry for production.

## Migrations

The `migrate` binary takes positional arguments (no `-c` flag):

```bash
# create <name> <config> <target_dir>
# up [number|all] <config> <db_host> <server_id>
```

Inside the container the config lives at `/opt/cwfr/config.json`. With the
compose file's app mount (`./backend/app` → `/opt/cwfr/app`), migrations sit
in `/opt/cwfr/app/migrations/s1`.

Create a new migration:

```bash
docker compose run --rm --entrypoint migrate app \
  create add_users_table /opt/cwfr/config.json /opt/cwfr/app/migrations/s1
```

Apply pending migrations (`db_host` is the databases id from config, e.g.
`postgresql.p1`; `s1` is the migration folder):

```bash
docker compose run --rm --entrypoint migrate app \
  up all /opt/cwfr/config.json postgresql.p1 s1
```

Without compose, remember to attach the container to `cwfr-network` so it can
reach the postgres service.

## Volumes

The compose file mounts:

| Host path | Container path | Purpose |
|----------|---------------|---------|
| `./config.json` | `/opt/cwfr/config.json:ro` | Configuration file |
| `./backend/app` | `/opt/cwfr/app:ro` | Application handlers (development) |
| `./frontend/docs/.vitepress/dist` | `/opt/cwfr/frontend:ro` | Frontend static files |

Plus the named volume `postgres-data` for the database.

### Frontend

Build the frontend before running:

```bash
cd frontend
npm run docs:build
cd ..
```

The output in `frontend/docs/.vitepress/dist` is mounted to
`/opt/cwfr/frontend` in the container (and also copied into the image at
build time).

### SSL Certificates

To enable TLS, place certificates on the host and add mounts for them in
compose or `docker run`, then reference the paths in `config.json`:

```bash
mkdir -p ssl/certs ssl/private

# Self-signed certificate for development
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout ssl/private/localhost.key \
  -out ssl/certs/localhost.crt \
  -subj "/CN=localhost"
```

```yaml
    volumes:
      - ./ssl/certs:/etc/ssl/certs:ro
      - ./ssl/private:/etc/ssl/private:ro
```

## Production mode

For production environment, it's recommended to:

1. **Use a separate config**
   ```yaml
   volumes:
     - ./config.prod.json:/opt/cwfr/config.json:ro
   ```

2. **Limit resources**
   ```yaml
   deploy:
     resources:
       limits:
         memory: 2g
         cpus: "4"
   ```

3. **Store data in volumes**
   Mount directories for any data and logs your handlers write:
   ```yaml
   volumes:
     - cwfr_data:/opt/cwfr/data
   ```
   (plus `postgres-data`, already configured by compose)

4. **Don't publish the database port**
   Remove the `ports` entry of the `postgres` service so it is reachable only
   from `cwfr-network`.

## Troubleshooting

### Container won't start

Check logs:
```bash
docker logs cwfr-app
```

### App can't connect to postgres

Make sure both services share the `cwfr-network` network (they do by default)
and that the `databases` section of `config.json` points at the hostname
`postgres`, not `localhost` — inside a container `localhost` is the container
itself.

### Rebuild after code changes

```bash
docker compose build --no-cache app
docker compose up -d
```

### "library not found" error

Ensure all runtime libraries are installed in the runtime stage of Dockerfile.

### Permission denied on port 80

Standard ports require root privileges. Use `sudo` or map an alternative port:

```bash
docker run -d -p 8080:8080 cwfr:latest ...
```
