---
outline: deep
description: Template engine in C Web Framework. Rendering HTML templates with variables, conditions, loops and includes.
---

# Template Engine

The framework includes a built-in template engine for rendering HTML pages with dynamic data.

## Basics

Templates are HTML files with special tags for inserting variables, conditions, and loops. Templates are stored in storage and rendered using the `render()` function.

## API

```c
#include "view.h"

char* render(json_doc_t* document, const char* storage_name, const char* path_format, ...);
```

Renders a template with the specified data.

**Parameters**\
`document` — JSON document with data for the template.\
`storage_name` — name of the storage where the template is located.\
`path_format` — path to the template file (supports formatting).

**Return Value**\
Pointer to a string with rendered HTML. Memory must be freed with `free()`.

## Template Syntax

### Variables

Output a variable value:

```html
<p>Hello, {{ name }}!</p>
<p>Email: {{ user.email }}</p>
<p>First item: {{ items[0] }}</p>
```

### Conditions

```html
{{ if is_authenticated }}
    <p>Welcome, {{ user.name }}!</p>
{{ elseif is_guest }}
    <p>You are logged in as a guest</p>
{{ else }}
    <p>Please log in</p>
{{ endif }}
```

Condition inversion:

```html
{{ if !is_authenticated }}
    <a href="/login">Log in</a>
{{ endif }}
```

### Loops

Iterating over an array:

```html
<ul>
{{ for item in items }}
    <li>{{ item.name }} - {{ item.price }}</li>
{{ endfor }}
</ul>
```

With index access:

```html
{{ for item, index in items }}
    <li>{{ index }}: {{ item.name }}</li>
{{ endfor }}
```

### Including Other Templates

```html
{{ include "partials/header.html" }}

<main>
    <h1>{{ title }}</h1>
</main>

{{ include "partials/footer.html" }}
```

## Usage Examples

### Simple Rendering

```c
#include "http.h"
#include "view.h"

void home(httpctx_t* ctx) {
    // Create data for template
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Home"));
    json_object_set(root, "username", json_create_string("John"));

    // Render template
    char* html = render(doc, "templates", "pages/home.html");
    json_free(doc);

    if (html == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->header_add(ctx->response, "Content-Type", "text/html; charset=utf-8");
    ctx->response->send_data(ctx->response, html);
    free(html);
}
```

### Template with List

```c
void products_list(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Products"));

    // Create products array
    json_token_t* products = json_create_array(doc);

    // Add products
    json_token_t* product1 = json_create_object(doc);
    json_object_set(product1, "name", json_create_string("Laptop"));
    json_object_set(product1, "price", json_create_number(50000));
    json_array_append(products, product1);

    json_token_t* product2 = json_create_object(doc);
    json_object_set(product2, "name", json_create_string("Phone"));
    json_object_set(product2, "price", json_create_number(30000));
    json_array_append(products, product2);

    json_object_set(root, "products", products);

    char* html = render(doc, "templates", "pages/products.html");
    json_free(doc);

    ctx->response->header_add(ctx->response, "Content-Type", "text/html; charset=utf-8");
    ctx->response->send_data(ctx->response, html);
    free(html);
}
```

### products.html Template

```html
<!DOCTYPE html>
<html>
<head>
    <title>{{ title }}</title>
</head>
<body>
    <h1>{{ title }}</h1>

    {{ if products }}
        <table>
            <tr>
                <th>#</th>
                <th>Name</th>
                <th>Price</th>
            </tr>
            {{ for product, i in products }}
            <tr>
                <td>{{ i }}</td>
                <td>{{ product.name }}</td>
                <td>{{ product.price }}</td>
            </tr>
            {{ endfor }}
        </table>
    {{ else }}
        <p>No products found</p>
    {{ endif }}
</body>
</html>
```

### Rendering with Model Data

```c
void user_profile(httpctx_t* ctx) {
    // Get user
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Convert model to JSON
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_token_t* user_json = model_to_json(user, NULL);
    json_object_set(root, "user", user_json);
    json_object_set(root, "title", json_create_string("User Profile"));

    user_free(user);

    char* html = render(doc, "templates", "pages/profile.html");
    json_free(doc);

    ctx->response->header_add(ctx->response, "Content-Type", "text/html; charset=utf-8");
    ctx->response->send_data(ctx->response, html);
    free(html);
}
```

### profile.html Template

```html
<!DOCTYPE html>
<html>
<head>
    <title>{{ title }}</title>
</head>
<body>
    {{ include "partials/header.html" }}

    <main>
        <h1>{{ title }}</h1>

        {{ if user }}
            <div class="profile">
                <p><strong>ID:</strong> {{ user.id }}</p>
                <p><strong>Email:</strong> {{ user.email }}</p>
                <p><strong>Name:</strong> {{ user.name }}</p>
            </div>
        {{ else }}
            <p>User not found</p>
        {{ endif }}
    </main>

    {{ include "partials/footer.html" }}
</body>
</html>
```

## Template Organization

Recommended structure:

```
storages/
└── templates/
    ├── layouts/
    │   └── base.html
    ├── partials/
    │   ├── header.html
    │   ├── footer.html
    │   └── navigation.html
    └── pages/
        ├── home.html
        ├── products.html
        └── profile.html
```

### partials/header.html

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
            {{ if is_authenticated }}
                <a href="/profile">Profile</a>
                <a href="/logout">Logout</a>
            {{ else }}
                <a href="/login">Login</a>
            {{ endif }}
        </nav>
    </header>
```

### partials/footer.html

```html
    <footer>
        <p>&copy; 2024 Example App</p>
    </footer>
</body>
</html>
```

## Nested Data

Access nested objects using dot notation:

```html
<p>{{ user.profile.avatar_url }}</p>
<p>{{ company.address.city }}</p>
```

Access array elements by index:

```html
<p>First: {{ items[0].name }}</p>
<p>Second: {{ items[1].name }}</p>
```

## Escaping

By default, all values are escaped to prevent XSS attacks. HTML tags in variables will be converted to safe entities.
