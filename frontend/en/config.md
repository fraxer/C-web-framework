---
outline: deep
description: Description of the structure of the configuration file and an example of filling
---

# Configuration file

## Section main

### workers <Badge type="info" text="number"/>

Creates threads to handle connections, read/write data between client and server.

### threads <Badge type="info" text="number"/>

Creates threads for processing requests and forming the response body.

### reload <Badge type="info" text="soft | hard"/>

Reload mode. 

* `soft` - reloading the application while keeping active connections.
* `hard` - reloading the application with forced closing of connections.

### read_buffer <Badge type="info" text="number"/>

The buffer size for reading and writing data to the socket. Specified in bytes.

### client_max_body_size <Badge type="info" text="number"/>

The maximum size of uploaded content in bytes.

### tmp <Badge type="info" text="string"/>

Path to the temporary files directory.

### gzip <Badge type="info" text="string array"/>

List of mime types to compress. If content compression is not required, leave the field blank.

## Section migrations

### source_directory <Badge type="info" text="string"/>

Path to the directory with migrations.

## Section servers

The section lists the list of servers.

### domains <Badge type="info" text="string array"/>

In the list of domains, you can specify the fully qualified domain name or a template.

* `example.com`
* `*.example.com`
* `s(\\d+).example.com`
* `s(.*).example.com`

The character (*) can only be specified without a dot at the beginning or end of the pattern. This was done for ease of specifying subdomains.

Otherwise, the pattern is specified as a regular expression.

### ip <Badge type="info" text="string"/>

Server IP address, for example `127.0.0.1`

### port <Badge type="info" text="number"/>

Server port. Usually `80` or `443`.

### root <Badge type="info" text="string"/>

The absolute path to the `webroot` directory where your pages, scripts, styles, etc. are located.

### index <Badge type="info" text="string"/>

Search for a default file if a directory is specified as a resource.

When requesting the resource `https://example.com/directory`, the server will generate the address `https://example.com/directory/index.html`

### http <Badge type="info" text="object"/>

The section contains a list of routes with handlers attached to them and a list of redirects.

#### routes <Badge type="info" text="object"/>

Routes are specified as objects.

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
            "POST": ["/var/www/server/build/exec/handlers/libindexpage.so", "post"],
            "DELETE": ["/var/www/server/build/exec/handlers/libindexpage.so", "delete"]
        },
        "/update": {
            "PATCH": ["/var/www/server/build/exec/handlers/libupdatepage.so", "patch"]
        }
        ...
    },
    ...
},
...
```

The path to the resource can be specified as a regular expression.

#### redirects <Badge type="info" text="object"/>

Redirects are represented as a pair `existing route`: `new route`.

```json
{
    ...
    "redirects": {
        "/number/(\\d)/(\\d)": "/digit/{1}/{2}",
        "(.*)": "https://example.com{1}"
    },
    ...
}
```

An existing route is either a regular expression pattern or an exact path to a resource.

New route - a path to a resource with the ability to insert a group of characters from a regular expression.

This is done using curly braces with group number `{1}`.

The group number starts with `1`.

### websockets <Badge type="info" text="object"/>

The section structure is the same as in http.

### databases <Badge type="info" text="object"/>

* ip <Badge type="info" text="general"/> - database server ip address
* port <Badge type="info" text="general"/> - port
* user <Badge type="info" text="general"/> - user login
* password <Badge type="info" text="general"/> - user password

* dbname <Badge type="info" text="postgresql, mysql"/> - database name
* connection_timeout <Badge type="info" text="postgresql"/> - waiting time in seconds before establishing a connection.
* migration <Badge type="info" text="postgresql, mysql"/> - apply migrations to database
* dbindex <Badge type="info" text="redis"/> - database index

```json
...
"databases": {
    "postgresql": [
        {
            "ip": "127.0.0.1",
            "port": 5432,
            "dbname": "dbname",
            "user": "root",
            "password": "",
            "connection_timeout": 3,
            "migration": true
        }
    ],
    "mysql": [
        {
            "ip": "127.0.0.1",
            "port": 3306,
            "dbname": "dbname",
            "user": "root",
            "password": "",
            "migration": false
        }
    ],
    "redis": [
        {
            "ip": "127.0.0.1",
            "port": 6379,
            "dbindex": 0,
            "user": "",
            "password": ""
        }
    ],
},
...
```

### tls <Badge type="info" text="object"/>

To configure a secure connection, you must specify the path to the certificate, private key, and enumerate the ciphers.

If a secure connection is not required, then remove the section from the configuration file.

#### fullchain <Badge type="info" text="string"/>

The path to the certificate or certificate chain.

#### private <Badge type="info" text="string"/>

Path to the private key file.

#### ciphers <Badge type="info" text="string"/>

List of ciphers separated by spaces.

## Section mimetypes

The section contains a list of mime types and their corresponding file extensions.

## Example file

```json
{
    "main": {
        "workers": 1,
        "threads": 1,
        "reload": "hard",
        "read_buffer": 16384,
        "client_max_body_size": 110485760,
        "tmp": "/tmp",
        "gzip": [
            "text/plain",
            "text/html",
            "text/css",
            "application/javascript",
            "application/json"
        ]
    },
    "migrations": {
        "source_directory": "/var/www/server/migrations"
    },
    "servers": {
        "s1": {
            "domains": [
                "example1.com",
                "*.example1.com",
                "(a1|a2|a3).example1.com",
                "(.1|.*|a3).example1.com"
            ],
            "ip": "127.0.0.1",
            "port": 80,
            "root": "/var/www/www.example1.com/web",
            "index": "index.html",
            "http": {
                "routes": {
                    "/": {
                        "GET": ["/var/www/server/build/exec/handlers/libindexpage.so", "get"]
                    },
                    "/wss": {
                        "GET": ["/var/www/server/build/exec/handlers/libindexpage.so", "websocket"]
                    }
                },
                "redirects": {
                    "/section1/(\\d+)/section2/(\\d+)/section3": "/one/{1}/two/{2}/three",
                    "/one/\\d+/two/\\d+/three": "/",
                    "/user": "/persons",
                    "/user(.*)/(\\d)": "/user-{1}-{2}"
                }
            },
            "websockets": {
                "routes": {
                    "/": {
                        "GET": ["/var/www/server/build/exec/handlers/libwsindexpage.so", "echo"]
                    }
                }
            },
            "databases": {
                "postgresql": [
                    {
                        "ip": "127.0.0.1",
                        "port": 5432,
                        "dbname": "dbname",
                        "user": "root",
                        "password": "",
                        "connection_timeout": 3,
                        "migration": true
                    }
                ],
                "mysql": [
                    {
                        "ip": "127.0.0.1",
                        "port": 3306,
                        "dbname": "dbname",
                        "user": "root",
                        "password": "",
                        "migration": false
                    }
                ],
                "redis": [
                    {
                        "ip": "127.0.0.1",
                        "port": 6379,
                        "dbindex": 0,
                        "user": "",
                        "password": ""
                    }
                ]
            },
            "tls": {
                "fullchain": "/var/www/server/cert/fullchain.pem",
                "private": "/var/www/server/cert/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 ..."
            }
        },
        "s2": {
            "domains": [
                "example2.com:8080"
            ],
            "ip": "127.0.0.1",
            "port": 8080,
            "root": "/var/www/example2.com/web",
            "index": "index.html",
            "http": {
                "routes": {
                    "/": {
                        "GET": ["/var/www/server/build/exec/handlers/libindexpage.so", "index"]
                    }
                }
            }
        }
    },
    "mimetypes": {
        "text/plain": ["txt"],
        "text/html": ["html", "htm", "shtml"],
        "text/css": ["css"],
        "text/xml": ["xml"],
        "image/gif": ["gif"],
        "image/jpeg": ["jpeg", "jpg"],
        "application/json": ["json"],
        "application/javascript": ["js"],

        "image/png": ["png"],
        "image/svg+xml": ["svg", "svgz"],
        "image/tiff": ["tif", "tiff"],
        "image/vnd.wap.wbmp": ["wbmp"],
        "image/webp": ["webp"],
        "image/x-icon": ["ico"],
        "image/x-jng": ["jng"],
        "image/x-ms-bmp": ["bmp"],

        "audio/mpeg": ["mp3"],
        "audio/ogg": ["ogg"],
        "audio/x-m4a": ["m4a"],

        "video/mp4": ["mp4"],
        "video/mpeg": ["mpeg", "mpg"],
        "video/quicktime": ["mov"],
        "video/webm": ["webm"],
        "video/x-ms-wmv": ["wmv"],
        "video/x-msvideo": ["avi"]
    }
}
```