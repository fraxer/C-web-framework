---
outline: deep
description: Virtual host configuration in C Web Framework. Binding domains, subdomains and regular expressions to servers.
---

# Domains

The framework supports virtual hosts — multiple domains on a single server. Use exact names, wildcards, or regular expressions.

```json
"servers": {
    "s1": {
        "domains": [
            "example1.com",
            "*.example1.com",
            "(a1|a2|a3).example1.com",
            "(.1|.*|a3).example1.com"
        ],
        ...
    },
    ...
}
```

The search is performed by the value of the `Host` HTTP header. If the header is empty or missing, the client will receive a 404 Not found response.

## Wildcard names

A wildcard name can only contain an asterisk at the beginning or end of the name, and only on a period boundary.

The names `www.+.example.org` and `w*.example.org` are invalid. However, these names can be specified using regular expressions, such as `www(.+).example.org` and `^w(.*).example.org$`. An asterisk can match multiple parts of a name. The name `*.example.org` matches not only `www.example.org` but also `www.sub.example.org`.

## Regular expression names

The regular expressions used by C Web Framework are compatible with the expressions used in the Perl Programming Language (PCRE). For example, `^www\d+\.example\.net$`.
