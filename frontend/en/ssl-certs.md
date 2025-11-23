---
outline: deep
description: An SSL certificate allows you to recognize the server and confirm the security of the site. The page describes how to connect a certificate to a site.
---

# SSL certificates

An SSL certificate is a digital document that authenticates a website and allows an encrypted connection to be used. It contains data about the organization, its owner and confirms their existence. Allows you to recognize the server and confirm the security of the site.

The abbreviation SSL stands for Secure Sockets Layer, a security protocol that creates an encrypted connection between a web server and a web browser.

Companies and organizations need to add SSL certificates to websites to secure online transactions and keep customer data private and secure.

In order for the site to work using the HTTPS secure connection protocol, you need an SSL certificate.

The certificate consists of two files:
* The first is a file with data about the certificate, who issued it, for how long, to whom, etc.
* The second is the secret key, which must be stored on the server without outside access.

In the configuration file, add the `tls` section for the server:

```json
{
    ...
    "servers": {
        "s1": {
            "domains": [
                "example1.com"
            ],
            ...
            "tls": {
                "fullchain": "/var/www/server/cert/fullchain.cert",
                "private": "/var/www/server/cert/privkey.pem",
                "ciphers": "TLS_AES_256_GCM_SHA384 ..."
            }
        }
    }
}
```

`ciphers` is a list of ciphers needed to establish a secure client-server connection.

```
TLS_AES_256_GCM_SHA384 TLS_CHACHA20_POLY1305_SHA256 TLS_AES_128_GCM_SHA256 TLS_AES_128_CCM_8_SHA256 TLS_AES_128_CCM_SHA256 ECDHE-ECDSA-AES256-GCM-SHA384:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-CHACHA20-POLY1305:ECDHE-RSA-CHACHA20-POLY1305:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA256
```