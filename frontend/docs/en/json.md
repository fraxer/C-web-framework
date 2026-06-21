---
outline: deep
description: C Web Framework JSON library. Parsing, generation, working with objects and arrays. Token pool, thread safety, and Unicode support.
---

# JSON library

A high-performance library for working with the JSON format: parsing, generation, modification, and serialization of documents.

Tokens are allocated from a **memory pool** (a chain of blocks holding `TOKENS_PER_BLOCK` slots each) and reused across operations, which avoids frequent `malloc`/`free` calls. The pool manager is initialized automatically the first time any library function is called.

::: tip Lifecycle
- Tokens are created **independently of the document** — the `json_create_*` functions do not take a document.
- The document holds the root token; when the document is destroyed (`json_free`), all of its tokens and the string buffer are released.
- The string returned by `json_stringify` is owned by the document and stays valid until the next call or `json_free`. To make the string outlive the document, use `json_stringify_detach` (then you must release it manually with `free`).
:::

## Core types

- `json_doc_t` — document: root token, serialization buffer, and the `ascii_mode` flag.
- `json_token_t` — a value-tree node: object, array, string, number, boolean, or `null`.
- `json_it_t` — iterator for traversing an object or an array.

The `json_doc_t::ascii_mode` field controls serialization: `0` (default) emits UTF-8 as-is, `1` encodes every non-ASCII character as `\uXXXX`.

## Document lifecycle

```c
json_doc_t* json_parse(const char* string);
```

Parse a string and build a document.

**Parameters**\
`string` — source JSON string (null-terminated).

**Return value**\
Pointer to the document, or `NULL` on allocation failure or invalid JSON.

<br>

```c
json_doc_t* json_create_empty(void);
```

Create an empty document with no root token. Assign the root separately via `json_set_root`.

**Return value**\
Pointer to the document, or `NULL` on allocation failure.

<br>

```c
json_doc_t* json_root_create_object(void);
json_doc_t* json_root_create_array(void);
```

Create a document with a ready-made root token — an object or an array, respectively. Equivalent to `json_create_empty()` + `json_set_root()`.

**Return value**\
Pointer to the document, or `NULL` on allocation failure.

<br>

```c
void json_set_root(json_doc_t* document, json_token_t* token);
```

Assign `token` as the document's root. Does nothing if `document` or `token` is `NULL`.

**Parameters**\
`document` — the document.\
`token` — the token to set as root.

<br>

```c
void json_clear(json_doc_t* document);
void json_free(json_doc_t* document);
```

`json_clear` frees the token tree and the string buffer but keeps the document structure itself — you can assign a new root to it.

`json_free` is `json_clear` plus freeing the document structure itself. Call it when you are done with the document.

**Parameters**\
`document` — the document (`NULL` is allowed).

<br>

```c
int json_copy(json_doc_t* from, json_doc_t* to);
```

Deep-copy document `from` into an existing document `to` (serialize + re-parse). The previous contents of `to` are not freed — call `json_clear(to)` first if needed.

**Parameters**\
`from` — source document.\
`to` — destination document.

**Return value**\
Nonzero on success, `0` on error.

<br>

## Root and values

```c
json_token_t* json_root(const json_doc_t* document);
```

The document's root token.

**Return value**\
Pointer to the token, or `NULL` if the document has no root.

<br>

```c
int         json_bool(const json_token_t* token);
const char* json_string(const json_token_t* token);
size_t      json_string_size(const json_token_t* token);
```

Direct reading of boolean and string values — with no `ok` out-parameter.

- `json_bool` — the boolean value (`0`/`1`); also `0` for `NULL` or a non-`bool`.
- `json_string` — pointer to a null-terminated string; `NULL` for `NULL` or a non-string. The pointer is owned by the token.
- `json_string_size` — string length in bytes **without** the null terminator; `0` for `NULL` or a non-string.

<br>

```c
int          json_int(const json_token_t* token, int* ok);
unsigned int json_uint(const json_token_t* token, int* ok);
long long    json_llong(const json_token_t* token, int* ok);
double       json_double(const json_token_t* token, int* ok);
```

Read a number into the desired type. The `ok` out-parameter accepts `NULL` or a pointer to `int`: it is set to nonzero only when the token is a number and the value fits the target type; otherwise it is set to `0`. On overflow the corresponding bound is returned (`INT_MAX`/`INT_MIN`, `UINT_MAX`, `LLONG_MAX`/`LLONG_MIN`, `DBL_MAX`).

::: tip Check the type first
Before reading, confirm the token is a number with `json_is_number(token)`, or inspect `ok`.
:::

<br>

```c
long double json_ldouble(const json_token_t* token);
```

The number at its native `long double` precision (the internal storage format). Has no `ok` parameter: returns `0.0L` for a non-number or `NULL`; the sign of infinity is preserved.

<br>

## Type checks

```c
int json_is_bool(const json_token_t* token);
int json_is_null(const json_token_t* token);
int json_is_string(const json_token_t* token);
int json_is_number(const json_token_t* token);
int json_is_object(const json_token_t* token);
int json_is_array(const json_token_t* token);
```

Check the value type of a token.

**Parameters**\
`token` — pointer to the token.

**Return value**\
Nonzero if the type matches, otherwise `0` (including for `NULL`).

<br>

## Creating tokens

```c
json_token_t* json_create_bool(int value);
json_token_t* json_create_null(void);
json_token_t* json_create_string(const char* value);
json_token_t* json_create_number(long double value);
json_token_t* json_create_object(void);
json_token_t* json_create_array(void);
```

Create a new token of the given type. All numeric values are created with a single function, `json_create_number` — the storage type is uniform (`long double`), and conversion to the desired type happens on read (`json_int`, `json_double`, …).

**Parameters**\
`value` — value for the token.

**Return value**\
Pointer to the token, or `NULL` on allocation failure.

<br>

## Arrays

```c
int           json_array_prepend(json_token_t* array, json_token_t* token);
int           json_array_append(json_token_t* array, json_token_t* token);
int           json_array_append_to(json_token_t* array, int index, json_token_t* token);
int           json_array_erase(json_token_t* array, int index, int count);
int           json_array_clear(json_token_t* array);
int           json_array_size(const json_token_t* array);
json_token_t* json_array_get(const json_token_t* array, int index);
```

Array management.

- `json_array_prepend` — insert at the front (index 0).
- `json_array_append` — append to the end.
- `json_array_append_to` — insert at `index`; if `index` is past the end, appends to the end.
- `json_array_erase` — remove `count` elements starting at `index`.
- `json_array_clear` — remove all elements (the array token itself is kept).
- `json_array_size` — number of elements.
- `json_array_get` — element at `index`, or `NULL`.

**Parameters**\
`array` — the array. `token` — token to insert. `index` — element position. `count` — number of elements.

**Return value**\
For mutating operations and `json_array_size` — nonzero/value on success, `0` on invalid arguments. `json_array_get` — pointer to the token, or `NULL`.

<br>

## Objects

```c
int           json_object_set(json_token_t* object, const char* key, json_token_t* token);
json_token_t* json_object_get(const json_token_t* object, const char* key);
int           json_object_remove(json_token_t* object, const char* key);
int           json_object_size(const json_token_t* object);
int           json_object_clear(json_token_t* object);
```

Object management.

- `json_object_set` — add a `key` → `token` pair. If the key already exists, the old value is replaced and freed recursively (along with nested tokens).
- `json_object_get` — value by key, or `NULL`.
- `json_object_remove` — remove the pair by key.
- `json_object_clear` — remove all pairs.
- `json_object_size` — number of pairs.

**Parameters**\
`object` — the object. `key` — property name. `token` — the value.

**Return value**\
For mutating operations and `json_object_size` — nonzero/value on success, `0` on invalid arguments. `json_object_get` — pointer to the token, or `NULL`.

<br>

## Changing a token's value

```c
void json_token_set_bool(json_token_t* token, int value);
void json_token_set_null(json_token_t* token);
void json_token_set_string(json_token_t* token, const char* value);
void json_token_set_int(json_token_t* token, int value);
void json_token_set_uint(json_token_t* token, unsigned int value);
void json_token_set_llong(json_token_t* token, long long value);
void json_token_set_double(json_token_t* token, double value);
void json_token_set_ldouble(json_token_t* token, long double value);
void json_token_set_object(json_token_t* token, json_token_t* object);
void json_token_set_array(json_token_t* token, json_token_t* array);
```

Replace the value of an existing token: both the type and the contents change. With `json_token_set_object` / `json_token_set_array`, the token begins to reference the passed container.

**Parameters**\
`token` — the token to modify. `value` — the new value. `object` / `array` — the container.

**Return value**\
None

<br>

## Iterator

```c
json_it_t     json_init_it(const json_token_t* token);
int           json_end_it(const json_it_t* iterator);
const void*   json_it_key(const json_it_t* iterator);
json_token_t* json_it_value(const json_it_t* iterator);
json_it_t     json_next_it(json_it_t* iterator);
void          json_it_erase(json_it_t* iterator);
```

A single iterator for traversing either an object or an array.

- `json_init_it` — create an iterator over a container token.
- `json_end_it` — returns `1` (true) when the traversal is finished.
- `json_it_key` — for an object returns the key (`const char*`), for an array returns a pointer to the current index (`int*`), otherwise `NULL`.
- `json_it_value` — the current element's value.
- `json_next_it` — advance to the next element.
- `json_it_erase` — remove the current element while iterating.

**Parameters**\
`token` — container to initialize the iterator from. `iterator` — the iterator.

A typical traversal loop:

```c
for (json_it_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
    const char* key   = json_it_key(&it);        // object key
    json_token_t* val = json_it_value(&it);
    /* ... */
}
```

<br>

## Serialization

```c
const char* json_stringify(json_doc_t* document);
size_t      json_stringify_size(json_doc_t* document);
char*       json_stringify_detach(json_doc_t* document);
```

- `json_stringify` — build a string from the document. Returns a pointer owned by the document: it stays valid until the next `json_stringify` call or `json_free`. `NULL` if the document has no root or on error.
- `json_stringify_size` — length in bytes of the last produced string, without the null terminator.
- `json_stringify_detach` — build the string and return a separate copy whose ownership is transferred to the caller; the document's buffer is cleared in the process. Free the returned pointer with `free`.

Set `document->ascii_mode = 1` before serializing to encode every non-ASCII character as `\uXXXX`.

**Parameters**\
`document` — the document.

**Return value**\
`json_stringify` — constant pointer to the string, or `NULL`. `json_stringify_size` — the size. `json_stringify_detach` — pointer to the copy (freed by the caller), or `NULL`.

<br>

Ready-to-use examples for parsing, building, and serializing are in the [JSON examples](/en/examples-json) section.
