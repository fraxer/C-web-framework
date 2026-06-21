---
outline: deep
description: List of HTTP status codes supported by cpdy for use in responses
---

# HTTP status codes

The following status codes are supported by the framework. They can be assigned to the response through
`ctx->response->statusCode` or passed to `send_default()`. Any code not listed here yields an empty
status line, so use only the codes below.

For guidance on applying these codes in handlers, see [HTTP responses](/en/responses).

### Informational

| Code | Description |
| --- | --- |
| 100 Continue | The client should continue sending the request body |
| 101 Switching Protocols | The server is switching protocols as requested (e.g. WebSocket upgrade) |
| 102 Processing | The server has received the request and is still processing it |
| 103 Early Hints | Preliminary headers sent before the final response |

### Successful

| Code | Description |
| --- | --- |
| 200 OK | The request succeeded |
| 201 Created | The request succeeded and a new resource was created |
| 202 Accepted | The request has been accepted for processing but not yet completed |
| 203 Non-Authoritative Information | The returned data is a transformed copy of the origin's response |
| 204 No Content | The request succeeded; the body is intentionally empty |
| 205 Reset Content | The client should reset the document that sent the request |
| 206 Partial Content | Only part of the resource is delivered (range request) |
| 207 Multi-Status | The body carries several separate status codes (WebDAV) |
| 208 Already Reported | The members of a binding have already been enumerated (WebDAV) |
| 226 IM Used | The server fulfilled a GET using an instance manipulation |

### Redirection

| Code | Description |
| --- | --- |
| 300 Multiple Choices | The request has more than one possible response |
| 301 Moved Permanently | The resource has permanently moved to a new URL |
| 302 Found | The resource is temporarily under a different URL |
| 303 See Other | The client should fetch the response at another URL using GET |
| 304 Not Modified | The cached response is still valid; no body is transferred |
| 305 Use Proxy | The resource must be accessed through a proxy (deprecated) |
| 306 Switch Proxy | Formerly meant to switch proxies; no longer used |
| 307 Temporary Redirect | Repeat the request at another URL, keeping the same method |
| 308 Permanent Redirect | The resource is permanently at another URL, keeping the same method |

### Client errors

| Code | Description |
| --- | --- |
| 400 Bad Request | The server cannot process the request due to malformed syntax |
| 401 Unauthorized | Authentication is required and was not provided or has failed |
| 402 Payment Required | Reserved for future use; sometimes used for paid access |
| 403 Forbidden | The client does not have access rights to the content |
| 404 Not Found | No matching resource was found on the server |
| 405 Method Not Allowed | The request method is not supported for this resource |
| 406 Not Acceptable | The response does not satisfy the client's `Accept` criteria |
| 407 Proxy Authentication Required | Authentication with a proxy is required |
| 408 Request Timeout | The server timed out waiting for the request |
| 409 Conflict | The request conflicts with the current state of the server |
| 410 Gone | The resource is gone and will not be available again |
| 411 Length Required | The request must define a `Content-Length` |
| 412 Precondition Failed | A precondition header evaluated to false |
| 413 Payload Too Large | The request body exceeds the maximum allowed size |
| 414 URI Too Long | The requested URI is longer than the server will interpret |
| 415 Unsupported Media Type | The request media type is not supported |
| 416 Range Not Satisfiable | The requested byte range cannot be fulfilled |
| 417 Expectation Failed | The `Expect` header cannot be satisfied |
| 418 I'm a teapot | The server refuses to brew coffee because it is a teapot (April Fools' RFC) |
| 421 Misdirected Request | The request was directed at a server unable to produce a response |
| 422 Unprocessable Entity | The request is well-formed but semantically invalid (WebDAV) |
| 423 Locked | The resource is locked (WebDAV) |
| 424 Failed Dependency | The request failed due to a previous dependency (WebDAV) |
| 426 Upgrade Required | The client must switch to a different protocol |
| 428 Precondition Required | The origin server requires the request to be conditional |
| 429 Too Many Requests | The client has sent too many requests in a given period |
| 431 Request Header Fields Too Large | The request header fields are too large |
| 451 Unavailable For Legal Reasons | The resource is unavailable for legal or regulatory reasons |

### Server errors

| Code | Description |
| --- | --- |
| 500 Internal Server Error | The server encountered an unexpected error |
| 501 Not Implemented | The server does not support the request method |
| 502 Bad Gateway | A gateway received an invalid response from the upstream |
| 503 Service Unavailable | The server is temporarily unable to handle the request |
| 504 Gateway Timeout | The upstream server did not respond in time |
| 505 HTTP Version Not Supported | The HTTP version used in the request is not supported |
| 506 Variant Also Negotiates | Transparent content negotiation failed |
| 507 Insufficient Storage | The server cannot store the representation (WebDAV) |
| 508 Loop Detected | The server detected an infinite loop processing the request (WebDAV) |
| 510 Not Extended | Further protocol extensions are required |
| 511 Network Authentication Required | Network authentication is required to gain access |
