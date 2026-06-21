---
outline: deep
description: Virtual host configuration in C Web Framework — exact names, wildcards, regular expressions, IDN and SNI support.
---

# Domains

The framework supports virtual hosts — multiple domains on a single server (or several servers sharing the same `ip:port`). Domains are declared as an array of strings and can be exact names, contain `*` wildcards, or be full regular expressions.

```json
"servers": {
    "s1": {
        "domains": [
            "example.com",
            "www.example.com",
            "*.example.com",
            "mail.*",
            "(api|www).example.com"
        ],
        "ip": "0.0.0.0",
        "port": 80,
        ...
    },
    ...
}
```

## How matching works

The virtual server is selected by the value of the HTTP `Host` header. Before comparison, the value is normalized:

1. **Port is stripped** — `example.com:8080` becomes `example.com`.
2. **IPv6 literals** follow RFC 3986: brackets in `[::1]:8080` are removed and the bare address `::1` is used for matching.
3. **IDN conversion** — a UTF-8 domain (e.g. `пример.рф`) is converted to Punycode (`xn--80akhbyknj4f.xn--p1ai`); see [Internationalized domains (IDN)](#internationalized-domains-idn).

The value is then compared, in turn, against the domains of every server bound to the same `ip:port`. **First match wins**: there is no priority between exact names, wildcards, and regexes — ordering is determined solely by the sequence in the `domains` array. Therefore, more specific names should be placed above general ones.

::: tip Order matters
Because `*.example.com` captures both `www.example.com` and deeper addresses, place exact names and narrower patterns above the wildcard so they are not "shadowed".
:::

If no matching server is found, the client receives **404 Not Found**. An empty or missing `Host` header (mandatory in HTTP/1.1) results in **400 Bad Request**.

## Wildcard names

The asterisk `*` is allowed **only at the beginning or end** of the name and expands to the regular expression `.*`:

```json
"domains": [
    "*.example.com",
    "mail.*"
]
```

- `*.example.com` matches both `www.example.com` and `www.sub.example.com` — the asterisk spans multiple levels at once.
- `mail.*` matches `mail.com`, `mail.org`, and so on.
- Names such as `w*.example.com` or `www.*.example.org` are invalid — an asterisk in the middle causes a configuration load error. Such cases are expressed with a regular expression (see below).

Inside parenthesized/bracketed groups `(...)` and `[...]`, the symbols `*` and `.` keep their regex meaning (quantifier and "any character" respectively) and are not auto-transformed.

## Regular expression names

A regular expression compatible with **PCRE** (Perl Compatible Regular Expressions) may be used as a domain name:

```json
"domains": [
    "(api|www).example.com",
    "^www(\\d+).example.net$",
    "(.1|.*|a3).example.com"
]
```

Processing details:

- **Dots are escaped automatically.** A `.` outside brackets becomes `\.`, so `example.com` can be written as is, without escaping. Inside `(...)` / `[...]`, `.` keeps its "any character" meaning.
- **Automatic anchoring.** If the expression contains neither `^` at the start nor `$` at the end, the pattern is automatically wrapped in `^...$` (full match). Providing at least one anchor disables auto-anchoring entirely.
- **Case sensitivity.** Matching is case-sensitive; browsers typically send the `Host` header in lowercase.

## Internationalized domains (IDN)

Support for Cyrillic and other national domains is implemented via `libidn2`. Domains may be specified in UTF-8 in the configuration — they are converted to Punycode at load time, and the same conversion is applied to the `Host` header (and the TLS SNI) during comparison:

```json
"domains": [
    "пример.рф",
    "*.пример.рф"
]
```

If the `Host` header contains a malformed IDN that cannot be converted, the client receives **404 Not Found**.

## TLS and SNI

On TLS connections, the virtual server is initially selected by SNI (Server Name Indication). Both the SNI value and the subsequent `Host` header undergo IDN conversion and are matched against the same patterns.

Per RFC 9110, on a TLS connection with SNI the `Host` header must correspond to the server selected by SNI. If they disagree, the client receives **404 Not Found**.

## Quick reference

| Situation | Response |
|-----------|----------|
| Empty or missing `Host` (HTTP/1.1) | 400 Bad Request |
| No domain matched | 404 Not Found |
| `Host` does not match the SNI-selected server (TLS) | 404 Not Found |
| Malformed IDN in `Host` | 404 Not Found |
