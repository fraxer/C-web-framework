---
outline: deep
description: Шаблонизатор в C Web Framework. Рендеринг HTML-шаблонов с переменными, условиями, циклами и включениями.
---

# Шаблонизатор

Фреймворк включает встроенный шаблонизатор для рендеринга HTML-страниц с динамическими данными.

## Основы

Шаблоны — это HTML-файлы со специальными тегами для вставки переменных, условий и циклов. Шаблоны хранятся в хранилище и рендерятся с помощью функции `render()`.

## API

```c
#include "view.h"

char* render(json_doc_t* document, const char* storage_name, const char* path_format, ...);
```

Рендерит шаблон с указанными данными.

**Параметры**\
`document` — JSON-документ с данными для шаблона.\
`storage_name` — имя хранилища, где расположен шаблон.\
`path_format` — путь к файлу шаблона (поддерживает форматирование).

**Возвращаемое значение**\
Указатель на строку с отрендеренным HTML. Необходимо освободить память функцией `free()`.

## Синтаксис шаблонов

### Переменные

Вывод значения переменной:

```html
<p>Привет, {{ name }}!</p>
<p>Email: {{ user.email }}</p>
<p>Первый элемент: {{ items[0] }}</p>
```

### Условия

```html
{{ if is_authenticated }}
    <p>Добро пожаловать, {{ user.name }}!</p>
{{ elseif is_guest }}
    <p>Вы вошли как гость</p>
{{ else }}
    <p>Пожалуйста, войдите в систему</p>
{{ endif }}
```

Инверсия условия:

```html
{{ if !is_authenticated }}
    <a href="/login">Войти</a>
{{ endif }}
```

### Циклы

Итерация по массиву:

```html
<ul>
{{ for item in items }}
    <li>{{ item.name }} - {{ item.price }}</li>
{{ endfor }}
</ul>
```

С доступом к индексу:

```html
{{ for item, index in items }}
    <li>{{ index }}: {{ item.name }}</li>
{{ endfor }}
```

### Включение других шаблонов

```html
{{ include "partials/header.html" }}

<main>
    <h1>{{ title }}</h1>
</main>

{{ include "partials/footer.html" }}
```

## Примеры использования

### Простой рендеринг

```c
#include "http.h"
#include "view.h"

void home(httpctx_t* ctx) {
    // Создаем данные для шаблона
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Главная"));
    json_object_set(root, "username", json_create_string("Иван"));

    // Рендерим шаблон
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

### Шаблон со списком

```c
void products_list(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Товары"));

    // Создаем массив товаров
    json_token_t* products = json_create_array(doc);

    // Добавляем товары
    json_token_t* product1 = json_create_object(doc);
    json_object_set(product1, "name", json_create_string("Ноутбук"));
    json_object_set(product1, "price", json_create_number(50000));
    json_array_append(products, product1);

    json_token_t* product2 = json_create_object(doc);
    json_object_set(product2, "name", json_create_string("Телефон"));
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

### Шаблон products.html

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
                <th>Название</th>
                <th>Цена</th>
            </tr>
            {{ for product, i in products }}
            <tr>
                <td>{{ i }}</td>
                <td>{{ product.name }}</td>
                <td>{{ product.price }} руб.</td>
            </tr>
            {{ endfor }}
        </table>
    {{ else }}
        <p>Товары не найдены</p>
    {{ endif }}
</body>
</html>
```

### Рендеринг с данными модели

```c
void user_profile(httpctx_t* ctx) {
    // Получаем пользователя
    array_t* params = array_create();
    mparams_fill_array(params, mparam_int(id, 1));
    user_t* user = user_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->send_default(ctx->response, 404);
        return;
    }

    // Преобразуем модель в JSON
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_token_t* user_json = model_to_json(user, NULL);
    json_object_set(root, "user", user_json);
    json_object_set(root, "title", json_create_string("Профиль пользователя"));

    user_free(user);

    char* html = render(doc, "templates", "pages/profile.html");
    json_free(doc);

    ctx->response->header_add(ctx->response, "Content-Type", "text/html; charset=utf-8");
    ctx->response->send_data(ctx->response, html);
    free(html);
}
```

### Шаблон profile.html

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
                <p><strong>Имя:</strong> {{ user.name }}</p>
            </div>
        {{ else }}
            <p>Пользователь не найден</p>
        {{ endif }}
    </main>

    {{ include "partials/footer.html" }}
</body>
</html>
```

## Организация шаблонов

Рекомендуемая структура:

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
            <a href="/">Главная</a>
            {{ if is_authenticated }}
                <a href="/profile">Профиль</a>
                <a href="/logout">Выход</a>
            {{ else }}
                <a href="/login">Вход</a>
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

## Вложенные данные

Доступ к вложенным объектам через точку:

```html
<p>{{ user.profile.avatar_url }}</p>
<p>{{ company.address.city }}</p>
```

Доступ к элементам массива по индексу:

```html
<p>Первый: {{ items[0].name }}</p>
<p>Второй: {{ items[1].name }}</p>
```

## Экранирование

По умолчанию все значения экранируются для предотвращения XSS-атак. HTML-теги в переменных будут преобразованы в безопасные сущности.
