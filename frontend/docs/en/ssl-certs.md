---
outline: deep
description: HTTPS configuration in C Web Framework. Connecting SSL/TLS certificates, Let's Encrypt, self-signed certificates, cipher suites.
---

# SSL certificates

SSL/TLS certificates provide an encrypted HTTPS connection between the client and server, protecting transmitted data from interception.

## Configuration

To enable HTTPS, add a `tls` section to the server configuration:

```json
{
    "servers": {
        "s1": {
            "domains": ["example.com", "www.example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "root": "/var/www/example.com/web",
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256"
            },
            "http": {
                "routes": { ... }
            }
        }
    }
}
```

### Parameters

| Parameter | Description |
|-----------|-------------|
| `fullchain` | Path to the certificate file (including intermediate certificates) |
| `private` | Path to the private key |
| `ciphers` | List of supported ciphers |

## Obtaining a certificate

### Let's Encrypt (free)

[Let's Encrypt](https://letsencrypt.org/) provides free SSL certificates valid for 90 days with automatic renewal.

#### Installing Certbot

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install certbot

# CentOS/RHEL
sudo yum install certbot
```

#### Obtaining a certificate

```bash
# Standalone mode (server must be stopped)
sudo certbot certonly --standalone -d example.com -d www.example.com

# Webroot mode (server is running)
sudo certbot certonly --webroot -w /var/www/example.com/web -d example.com -d www.example.com
```

After successful issuance, certificates will be located at:
- `/etc/letsencrypt/live/example.com/fullchain.pem` — certificate
- `/etc/letsencrypt/live/example.com/privkey.pem` — private key

#### Configuration with Let's Encrypt

```json
{
    "tls": {
        "fullchain": "/etc/letsencrypt/live/example.com/fullchain.pem",
        "private": "/etc/letsencrypt/live/example.com/privkey.pem",
        "ciphers": "TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384"
    }
}
```

#### Automatic renewal

```bash
# Test renewal
sudo certbot renew --dry-run

# Add to cron (runs twice daily)
echo "0 0,12 * * * root certbot renew --quiet" | sudo tee /etc/cron.d/certbot-renew
```

### Self-signed certificate

For development and testing, you can use a self-signed certificate:

```bash
# Create directory
sudo mkdir -p /etc/ssl/private

# Generate private key and certificate
sudo openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
    -keyout /etc/ssl/private/selfsigned.key \
    -out /etc/ssl/certs/selfsigned.crt \
    -subj "/C=US/ST=California/L=San Francisco/O=Development/CN=localhost"

# Set permissions
sudo chmod 600 /etc/ssl/private/selfsigned.key
```

#### Configuration

```json
{
    "tls": {
        "fullchain": "/etc/ssl/certs/selfsigned.crt",
        "private": "/etc/ssl/private/selfsigned.key",
        "ciphers": "TLS_AES_256_GCM_SHA384 TLS_AES_128_GCM_SHA256"
    }
}
```

::: warning Warning
Self-signed certificates trigger browser warnings. Use them only for development.
:::

### Commercial certificate

When purchasing a certificate from a Certificate Authority (CA), you will receive:
- Domain certificate (`.crt`)
- Intermediate certificates (intermediate/chain)
- Private key (`.key`)

#### Combining certificates

```bash
# Create fullchain from certificate and intermediates
cat domain.crt intermediate.crt > fullchain.pem
```

## Recommended ciphers

### Modern configuration (TLS 1.3 + TLS 1.2)

```
TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256
```

### Maximum compatibility

```
TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 TLS_AES_128_CCM_8_SHA256 TLS_AES_128_CCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256
```

## HTTP and HTTPS on the same server

To support both protocols, create two servers:

```json
{
    "servers": {
        "http": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 80,
            "http": {
                "redirects": {
                    "/(.*)": "https://example.com/{1}"
                }
            }
        },
        "https": {
            "domains": ["example.com"],
            "ip": "0.0.0.0",
            "port": 443,
            "tls": {
                "fullchain": "/etc/ssl/certs/fullchain.pem",
                "private": "/etc/ssl/private/privkey.pem",
                "ciphers": "..."
            },
            "http": {
                "routes": { ... }
            }
        }
    }
}
```

## Verifying the setup

### Checking the certificate

```bash
# View certificate information
openssl x509 -in /etc/ssl/certs/fullchain.pem -text -noout

# Check expiration date
openssl x509 -in /etc/ssl/certs/fullchain.pem -enddate -noout

# Verify key and certificate match
openssl x509 -noout -modulus -in fullchain.pem | openssl md5
openssl rsa -noout -modulus -in privkey.pem | openssl md5
# Hashes should match
```

### Checking the connection

```bash
# Check SSL connection
openssl s_client -connect example.com:443 -servername example.com

# Check supported protocols
openssl s_client -connect example.com:443 -tls1_3
openssl s_client -connect example.com:443 -tls1_2
```

### Online verification

- [SSL Labs](https://www.ssllabs.com/ssltest/) — detailed SSL configuration analysis
- [SSL Checker](https://www.sslshopper.com/ssl-checker.html) — quick certificate check

## Security

### File permissions

```bash
# Certificate (can be public)
sudo chmod 644 /etc/ssl/certs/fullchain.pem

# Private key (root only)
sudo chmod 600 /etc/ssl/private/privkey.pem
sudo chown root:root /etc/ssl/private/privkey.pem
```

### Security headers

Add security headers via middleware:

```c
int middleware_security_headers(httpctx_t* ctx) {
    httpresponse_t* res = ctx->response;

    // Force HTTPS usage
    res->add_header(res, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");

    // Clickjacking protection
    res->add_header(res, "X-Frame-Options", "SAMEORIGIN");

    // XSS protection
    res->add_header(res, "X-Content-Type-Options", "nosniff");

    return 1;
}
```

## Troubleshooting

### Error "certificate verify failed"

Make sure the `fullchain.pem` file contains the complete certificate chain (domain certificate + intermediates).

### Error "key values mismatch"

The private key does not match the certificate. Verify the match using the command above.

### Error "permission denied"

Check the permissions on the certificate and key files. The server process must have read access.

### Certificate not updating

After updating the certificate, perform a hot reload of the server:

```bash
kill -SIGUSR1 $(pidof server)
```
