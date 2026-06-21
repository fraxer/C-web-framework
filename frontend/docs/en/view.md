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

char* render(json_doc_t* document, const char* storage_name, const char* path_format, ...);
```

Renders a template and returns the resulting string.

**Parameters**

- `document` — JSON document with data for the template.
- `storage_name` — name of the storage where the template is located.
- `path_format` — path to the template file inside the storage. Supports `printf`-style formatting through variadic arguments.

**Return value**

Pointer to a string with the rendered result, or `NULL` on error (file not found, syntax error). The memory must be freed with `free()`.

### `send_view()`

A method on the response object — renders a template and immediately sends the result to the client.

```c
void (*send_view)(struct httpresponse* response, json_doc_t* document,
                  const char* storage_name, const char* path_format, ...);
```

```c
ctx->response->send_view(ctx->response, document, "views", "/index.tpl");
```

## Syntax

The engine recognizes three kinds of tags:

| Tag | Purpose | Example |
| --- | --- | --- |
| <code v-pre>{{ ... }}</code> | Output a variable value | <code v-pre>{{ user.name }}</code> |
| `{% ... %}` | Control structures (`if`, `for`) | `{% if active %}` |
| `{* include ... *}` | Include another template | `{* include /header.tpl *}` |

### Variables

A value is printed with <code v-pre>{{ }}</code>. Nested fields are accessed with a dot, and array elements by index in square brackets.

```html
<p>Hello, {{ name }}!</p>
<p>Email: {{ user.email }}</p>
<p>First item: {{ items[0] }}</p>
<p>First user's name: {{ users[0].name }}</p>
```

::: tip Strings and numbers only
The values of string and numeric JSON fields are printed. If a value is missing or is an object/array, nothing is output.
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

A condition is true when the variable equals boolean `true`. The condition expression supports nested field and index access just like regular variables:

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

    ctx->response->send_view(ctx->response, doc, "views", "/pages/home.tpl");

    json_free(doc);
}
```

### Rendering with a Formatted Path

The path supports `printf`-style formatting — handy when the template name depends on input:

```c
char* html = render(doc, "views", "/pages/%s.tpl", page_name);

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

    ctx->response->send_view(ctx->response, doc, "views", "/pages/products.tpl");

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

    ctx->response->send_view(ctx->response, doc, "views", "/pages/profile.tpl");

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

::: warning Values are not escaped automatically
The engine outputs string values verbatim, without HTML escaping. If data comes from user input and is inserted into HTML, it must be escaped beforehand — otherwise arbitrary HTML/JavaScript may execute (XSS).
:::

Outputting values that may contain special characters (`<`, `>`, `&`, `"`) is only safe after sanitizing them on the handler side.
