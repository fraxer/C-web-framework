---
outline: deep
description: Template engine in C Web Framework. Rendering HTML with variables, conditions, loops and includes driven by a JSON document.
---

# Template Engine

The framework includes a built-in template engine for rendering text (HTML, emails, any text format) with dynamic data taken from a JSON document.

## How It Works

A template is a text file containing special tags. The data source is a `json_doc_t*`, and the file itself is read from a [storage](./storage.md) declared in `config.json`.

Parsing and compiling a template into a tag tree happens once; the result is cached in `viewstore` keyed by the file path. Subsequent `render()` calls with the same path reuse the compiled tree, so repeated rendering stays fast.

## Storage Configuration

Templates live in a regular storage (`filesystem` or `s3`). The storage name is then passed to the rendering functions.

```json
{
    "storages": {
        "views": {
            "type": "filesystem",
            "root": "/var/www/app/views"
        }
    }
}
```

## API

### `render()`

```c
#include "view.h"

char* render(json_doc_t* document, view_escape_e escape,
             const char* storage_name, const char* path_format, ...);
```

Renders a template and returns the resulting string.

**Parameters**

- `document` — JSON document with data for the template.
- `escape` — how <code v-pre>{{ }}</code> values are output (see below).
- `storage_name` — name of the storage where the template is located.
- `path_format` — path to the template file inside the storage. Supports `printf`-style formatting through variadic arguments.

**Return value**

Pointer to a string with the rendered result, or `NULL` on error (file not found, syntax error). The memory must be freed with `free()`.

**Escape modes**

| Mode | Use for | What happens to <code v-pre>{{ }}</code> values |
| --- | --- | --- |
| `VIEW_ESCAPE` | HTML pages, HTML emails | `&`, `<`, `>`, `"`, `'` become `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;` |
| `VIEW_RAW` | plain-text emails, other non-HTML formats | output as is |

The mode applies to the whole render, including templates pulled in with `{* include *}`. Escaping happens at output time, not at parse time, so one cached template serves both modes. See [Escaping and Security](#escaping-and-security).

### `send_view()`

A method on the response object — renders a template and immediately sends the result to the client.

```c
void (*send_view)(struct httpresponse* response, json_doc_t* document, view_escape_e escape,
                  const char* storage_name, const char* path_format, ...);
```

```c
ctx->response->send_view(ctx->response, document, VIEW_ESCAPE, "views", "/index.tpl");
```

::: warning Signature change
The `escape` parameter was added to `render()` and `send_view()` before the storage name. A call in the old form (`render(doc, "views", "/index.tpl")`) does not build: the core, handlers and migrations built through the framework's CMake functions compile with `-Werror=int-conversion`. If your application code is built by its own rules with an old gcc (before 14), enable that flag yourself — without it such a call builds with a warning and crashes at run time.
:::

## Syntax

The engine recognizes three kinds of tags:

| Tag | Purpose | Example |
| --- | --- | --- |
| <code v-pre>{{ ... }}</code> | Output the value of an expression | <code v-pre>{{ user.name }}</code> |
| `{% ... %}` | Control structures (`if`, `for`) | `{% if active %}` |
| `{* include ... *}` | Include another template | `{* include /header.tpl *}` |

### Variables

A value is printed with <code v-pre>{{ }}</code>. Nested fields are accessed with a dot, and array elements by index in square brackets. The tag may hold any [expression](#expressions), and [modifiers](#modifiers) can be applied to the value.

```html
<p>Hello, {{ name }}!</p>
<p>Email: {{ user.email }}</p>
<p>First item: {{ items[0] }}</p>
<p>First user's name: {{ users[0].name }}</p>
```

How a value of each type is printed:

| Type | Output |
| --- | --- |
| string | the text (escaped in `VIEW_ESCAPE` mode); a NUL byte prints as `�` (U+FFFD) |
| number | integers without a fractional part (`42`), fractions in the shortest exact form (`9.99`) |
| bool | `true` / `false` |
| null, missing value | empty string |
| object / array | empty string (print its fields or iterate over it) |

### Expressions

**Literals:** numbers (`42`, `9.99`, `-5.5e2`), strings in single or double quotes (`'text'`, `"text"`, with `\'`, `\"`, `\\`, `\n`, `\t`, `\r`), `true`, `false`, `null`.

**Variables:** a name (`[A-Za-z_][A-Za-z0-9_]*`), then a path of `.field` and `[index]`. An index is any expression: a number for an array, a string for an object (`obj[key]`, `obj['literal']`). An out-of-range index, a field of a scalar, a negative or fractional index give `null`, not an error.

**Operators** (highest precedence first):

| Precedence | Operators | Notes |
| --- | --- | --- |
| 1 | `\|` | [modifier](#modifiers), binds to the nearest operand |
| 2 | `!` `-` (unary) | `!` is logical negation, `-` works on numbers only |
| 3 | `*` `/` `%` | numbers only; `/ 0` and `% 0` give `null` |
| 4 | `+` `-` | numbers only (no string concatenation) |
| 5 | `<` `<=` `>` `>=` | numbers numerically, strings lexicographically; values of different types are unordered (`false`) |
| 6 | `==` `!=` | values of different types are not equal (`'1' != 1`) |
| 7 | `&&` | short-circuit |
| 8 | `\|\|` | short-circuit |

Parentheses `( ... )` group as usual. Fields and indexes can follow parentheses too: `(obj).list[0]`. Operations on incompatible types (`'a' + 1`) give `null`: the render does not fail, the value prints empty. Expression nesting is limited to 32 levels and one expression to 1024 operands and modifiers; more is a parse error.

```html
{% if (a + b) * 2 > limit && !(banned || muted) %} ... {% endif %}
<p>Price: {{ price * (1 - discount) }}</p>
<p>Next: {{ items[pos + 1].title }}</p>
```

### Modifiers

A modifier transforms a value and is written after it with a vertical bar: `value | name` or `value | name(arguments)`. Modifiers chain and apply left to right:

```html
<h1>{{ title | trim | upper }}</h1>
<p>{{ description | truncate(120) }}</p>
<p>{{ price | number(2, '.', ',') }} USD</p>
```

A modifier belongs to the nearest operand — a variable, a literal or an expression in parentheses — and binds tighter than any operator:

```text
{{ price * qty | round(2) }}     →  price * (qty | round(2))
{{ (price * qty) | round(2) }}   →  the whole product is rounded
{{ a || b | upper }}             →  a || (b | upper)
{{ count | default(0) + 1 }}     →  (count | default(0)) + 1
{{ -x | abs }}                   →  -(x | abs)
```

Arguments are any expressions, modifiers included. Parentheses of a modifier without arguments may be omitted: `upper` and `upper()` are the same. A path to a field or element of the result can follow a modifier:

```html
<p>{{ (users | first).name }}</p>
<p>{{ users | last.name }}</p>
```

Modifiers work in conditions and loops too:

```html
<p>{{ name | default(login | upper) }}</p>
{% if items | length > 3 %} ... {% endif %}
{% for tag in post.tags | default(fallback_tags) %} ... {% endfor %}
```

String modifiers accept numbers and bools as well — the value is first converted to text by the output rules (`42`, `9.99`, `true`). Lengths and cuts count UTF-8 characters, not bytes. Document strings may contain NUL bytes (`"a\u0000b"`): modifiers and comparisons treat them as an ordinary character, `json` writes them as `\u0000`, `url` as `%00`, and output into the template replaces NUL with `�`.

#### Output control

| Modifier | What it does |
| --- | --- |
| `raw` | Outputs the value without escaping. Allowed only at the root of a <code v-pre>{{ }}</code> tag, last in the chain |
| `nl2br` | Escapes the value and turns line breaks (`\r\n`, `\n`, `\r`) into `<br>`. Allowed only at the root of a <code v-pre>{{ }}</code> tag, last in the chain. In `VIEW_RAW` mode it outputs the text as is |
| `url` | Encodes the value for a URL: everything except `A-Z a-z 0-9 - _ . ~` becomes `%XX`, a space becomes `%20` |
| `json` | Serializes a value of any type, objects and arrays included, as JSON. `<`, `>`, `&`, `'`, U+2028, U+2029 are encoded as `\u003c` and so on, so the result with `raw` is safe inside `<script>`. Nesting deeper than 32 gives empty output |

#### Strings

| Modifier | What it does |
| --- | --- |
| `upper`, `lower` | Upper / lower case, Cyrillic included. Broken UTF-8 bytes become `�` (U+FFFD); the other string modifiers keep such bytes, counting each as one character |
| `capitalize` | Upper-cases the first letter, keeps the rest (`iPhone` → `IPhone`) |
| `trim` | Removes whitespace at both ends, no-break space included |
| `truncate(n, suffix='…')` | Cuts to `n` characters and appends `suffix` if anything was cut. The suffix is not counted in `n` |
| `replace(from, to)` | Replaces every occurrence of `from` with `to`, left to right |
| `default(value)` | Substitutes `value` when the value is `null` or an empty string. `0` and `false` are kept |

#### Numbers

Numbers only: a numeric string (for example `"50000.00"` from a database) gives empty output.

| Modifier | What it does |
| --- | --- |
| `abs` | Absolute value |
| `round(n=0)` | Rounds to `n` places (0–15), half away from zero. The result is a number: `9.50` with `round(2)` prints as `9.5` |
| `number(decimals=0, dec_sep='.', thousands_sep='')` | Formats with fixed places (0–15) and separators: `50000` with `number(2, '.', ',')` → `50,000.00` |

#### Collections

| Modifier | What it does |
| --- | --- |
| `length` | String length in characters, number of array or object elements; `0` for `null` |
| `first`, `last` | First / last array element, object value or string character; `null` for a number or bool |
| `join(sep='')` | Joins array elements (object values) with a separator |

::: info Errors
An unknown modifier, a wrong number of arguments, `raw` or `nl2br` not at the root of a <code v-pre>{{ }}</code> tag (including inside `{% if %}` and `{% for %}`) are parse errors: `render()` returns `NULL` and logs the reason. A value or argument of an unsuitable type (`upper` of an array, `round` of a string) is not an error: the modifier returns `null` and the tag prints an empty string.
:::

### Conditions

```html
{% if is_authenticated %}
    <p>Welcome, {{ user.name }}!</p>
{% elseif is_guest %}
    <p>You are logged in as a guest</p>
{% else %}
    <p>Please log in</p>
{% endif %}
```

Condition inversion with `!`:

```html
{% if !is_authenticated %}
    <a href="/login">Log in</a>
{% endif %}
```

A condition is any [expression](#expressions). **True:** a non-empty string, a non-zero number, `true`, a non-empty object or array. **False:** `''`, `0`, `false`, `null`, an empty container, a missing value. A condition can access nested fields and indexes and apply modifiers:

```html
{% if users[0].authorized %}
    {{ users[0].name }} is authorized
{% endif %}
```

### Loops

Iterating over an array:

```html
<ul>
{% for product in products %}
    <li>{{ product.name }} — {{ product.price }}</li>
{% endfor %}
</ul>
```

With access to the index (the second name after the comma):

```html
<ol>
{% for product, i in products %}
    <li>{{ i }}: {{ product.name }}</li>
{% endfor %}
</ol>
```

If the index name is omitted, it defaults to `index`:

```html
{% for product in products %}
    <p>#{{ index }}: {{ product.name }}</p>
{% endfor %}
```

Loops also iterate over **objects** — in that case the index holds the key name (a string), and the element holds the property value:

```html
{% for value, key in settings %}
    <p>{{ key }} = {{ value }}</p>
{% endfor %}
```

Inside a loop, variables named after the element and the index become available; their fields are accessed like any other variable.

### Including Templates

Including another file is done with the `{* include ... *}` tag. The path is given as-is, without quotes — it is a path within the template storage.

```html
{* include /partials/header.tpl *}

<main>
    <h1>{{ title }}</h1>
</main>

{* include /partials/footer.tpl *}
```

::: warning Include depth limit
Include depth is limited to 100 levels. Circular includes (a template including itself unconditionally) will fail at parse time.
:::

## Usage Examples

### Simple Rendering

```c
#include "http.h"
#include "view.h"

void home(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Home"));
    json_object_set(root, "username", json_create_string("John"));

    ctx->response->send_view(ctx->response, doc, VIEW_ESCAPE, "views", "/pages/home.tpl");

    json_free(doc);
}
```

### Rendering with a Formatted Path

The path supports `printf`-style formatting — handy when the template name depends on input:

```c
char* html = render(doc, VIEW_ESCAPE, "views", "/pages/%s.tpl", page_name);

if (html == NULL) {
    ctx->response->send_default(ctx->response, 404);
    return;
}

ctx->response->header_add(ctx->response, "Content-Type", "text/html; charset=utf-8");
ctx->response->send_data(ctx->response, html);
free(html);
```

### Product List

```c
void products_list(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Products"));

    json_token_t* products = json_create_array(doc);

    json_token_t* p1 = json_create_object(doc);
    json_object_set(p1, "name", json_create_string("Laptop"));
    json_object_set(p1, "price", json_create_number(50000));
    json_array_append(products, p1);

    json_token_t* p2 = json_create_object(doc);
    json_object_set(p2, "name", json_create_string("Phone"));
    json_object_set(p2, "price", json_create_number(30000));
    json_array_append(products, p2);

    json_object_set(root, "products", products);

    ctx->response->send_view(ctx->response, doc, VIEW_ESCAPE, "views", "/pages/products.tpl");

    json_free(doc);
}
```

Template `/pages/products.tpl`:

```html
<!DOCTYPE html>
<html>
<head><title>{{ title }}</title></head>
<body>
    <h1>{{ title }}</h1>

    {% if products %}
        <table>
            <tr><th>#</th><th>Name</th><th>Price</th></tr>
            {% for product, i in products %}
            <tr>
                <td>{{ i }}</td>
                <td>{{ product.name }}</td>
                <td>{{ product.price }}</td>
            </tr>
            {% endfor %}
        </table>
    {% else %}
        <p>No products found</p>
    {% endif %}
</body>
</html>
```

### Rendering with Model Data

```c
void user_profile(httpctx_t* ctx) {
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "user", model_to_json(user, NULL));
    json_object_set(root, "title", json_create_string("User Profile"));

    user_free(user);

    ctx->response->send_view(ctx->response, doc, VIEW_ESCAPE, "views", "/pages/profile.tpl");

    json_free(doc);
}
```

### Nested Loops

Suppose there is a document with users and their tasks:

```json
{
    "company": "My Company",
    "users": [
        {
            "name": "Alex",
            "authorized": true,
            "tasks": ["Task 1", "Task 2", "Task 3"]
        },
        {
            "name": "Bonnie",
            "authorized": false,
            "tasks": ["Task 4", "Task 5"]
        }
    ]
}
```

A template iterating over users and their tasks:

```html
{* include /header.tpl *}

Users of {{ company }}:
{% for user, i in users %}
    {{ i }}. {{ user.name }}
    Authorized: {% if user.authorized %}yes{% else %}no{% endif %}
    Tasks:
    {% for task, j in user.tasks %}
        - {{ j }}: {{ task }}
    {% endfor %}
{% endfor %}
```

The outer loop declares the element `user` and the index `i`; the inner loop declares the element `task` and the index `j`.

## Template Organization

Recommended storage layout:

```
views/
├── layouts/
│   └── base.tpl
├── partials/
│   ├── header.tpl
│   └── footer.tpl
└── pages/
    ├── home.tpl
    ├── products.tpl
    └── profile.tpl
```

### partials/header.tpl

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="utf-8">
    <title>{{ title }}</title>
    <link rel="stylesheet" href="/css/style.css">
</head>
<body>
    <header>
        <nav>
            <a href="/">Home</a>
            {% if is_authenticated %}
                <a href="/profile">Profile</a>
                <a href="/logout">Logout</a>
            {% else %}
                <a href="/login">Login</a>
            {% endif %}
        </nav>
    </header>
```

### partials/footer.tpl

```html
    <footer>
        <p>&copy; 2024 Example App</p>
    </footer>
</body>
</html>
```

## Escaping and Security

In `VIEW_ESCAPE` mode every <code v-pre>{{ }}</code> value is HTML-escaped: `&`, `<`, `>`, `"`, `'` become `&amp;`, `&lt;`, `&gt;`, `&quot;`, `&#39;`. That is enough for element text and for a quoted attribute value. Escaping happens after all modifiers, so `truncate`, for example, cuts the original text and cannot split `&amp;` in half.

How to output a value depending on where it goes:

```html
<p>{{ comment }}</p>                                    <!-- element text -->
<input value="{{ name }}">                              <!-- quoted attribute -->
<a href="/search?q={{ query | url }}">Search</a>        <!-- URL parameter -->
<p>{{ message | nl2br }}</p>                            <!-- multi-line text -->
<div data-config="{{ config | json }}"></div>           <!-- JSON in an attribute -->
<script>const data = {{ data | json | raw }};</script>  <!-- JSON in a script -->
<div>{{ trusted_html | raw }}</div>                     <!-- markup you trust -->
```

HTML entities are not decoded inside `<script>`, so it needs `json | raw`: the `json` modifier encodes `<`, `>`, `&` and `'` itself, and the sequence `</script>` cannot appear in the output.

::: warning raw turns protection off
Use `raw` only for markup the server produced itself. A value from a request, a database or another user printed with `raw` is XSS.
:::

::: warning Address in href
Escaping does not check the URL scheme: the value `javascript:alert(1)` in `href` passes unchanged because it contains no escaped characters. Check the address with `validate_url()` before rendering.
:::

`VIEW_RAW` mode escapes nothing. Use it only for formats where HTML is not interpreted: plain-text emails, plain text.
