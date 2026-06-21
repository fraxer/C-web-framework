---
outline: deep
description: Шаблонизатор в C Web Framework. Рендеринг HTML с переменными, условиями, циклами и включениями на основе JSON-документа.
---

# Шаблонизатор

Фреймворк включает встроенный шаблонизатор для рендеринга текста (HTML, письма, любые текстовые форматы) с подстановкой динамических данных из JSON-документа.

## Принцип работы

Шаблон — это текстовый файл со специальными тегами. Источником данных выступает `json_doc_t*`, а сам файл читается из [хранилища](./storage.md), объявленного в `config.json`.

Парсинг и компиляция шаблона в дерево тегов выполняются один раз; результат кэшируется в `viewstore` по пути к файлу. При последующих вызовах `render()` с тем же путём используется уже построенное дерево, поэтому повторный рендеринг отрабатывает быстро.

## Настройка хранилища

Шаблоны хранятся в обычном хранилище (`filesystem` или `s3`). Имя хранилища далее передаётся в функции рендеринга.

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

Рендерит шаблон и возвращает готовую строку.

**Параметры**

- `document` — JSON-документ с данными для шаблона.
- `storage_name` — имя хранилища, где расположен шаблон.
- `path_format` — путь к файлу шаблона внутри хранилища. Поддерживает `printf`-форматирование через вариативные аргументы.

**Возвращаемое значение**

Указатель на строку с отрендеренным результатом или `NULL` при ошибке (файл не найден, ошибка синтаксиса). Память необходимо освободить функцией `free()`.

### `send_view()`

Метод объекта ответа — рендерит шаблон и сразу отправляет результат клиенту.

```c
void (*send_view)(struct httpresponse* response, json_doc_t* document,
                  const char* storage_name, const char* path_format, ...);
```

```c
ctx->response->send_view(ctx->response, document, "views", "/index.tpl");
```

## Синтаксис

Шаблонизатор различает три вида тегов:

| Тег | Назначение | Пример |
| --- | --- | --- |
| <code v-pre>{{ ... }}</code> | Вывод значения переменной | <code v-pre>{{ user.name }}</code> |
| `{% ... %}` | Управляющие конструкции (`if`, `for`) | `{% if active %}` |
| `{* include ... *}` | Включение другого шаблона | `{* include /header.tpl *}` |

### Переменные

Значение выводится через <code v-pre>{{ }}</code>. Поддерживается доступ к вложенным полям через точку и к элементам массива по индексу в квадратных скобках.

```html
<p>Привет, {{ name }}!</p>
<p>Email: {{ user.email }}</p>
<p>Первый элемент: {{ items[0] }}</p>
<p>Имя первого пользователя: {{ users[0].name }}</p>
```

::: tip Только строки и числа
Выводятся значения строковых и числовых JSON-полей. Если значение отсутствует или имеет тип объект/массив, ничего не выводится.
:::

### Условия

```html
{% if is_authenticated %}
    <p>Добро пожаловать, {{ user.name }}!</p>
{% elseif is_guest %}
    <p>Вы вошли как гость</p>
{% else %}
    <p>Пожалуйста, войдите в систему</p>
{% endif %}
```

Инверсия условия через `!`:

```html
{% if !is_authenticated %}
    <a href="/login">Войти</a>
{% endif %}
```

Условие истинно, когда переменная равна логическому `true`. В условном выражении можно использовать доступ к вложенным полям и индексам, как и в обычных переменных:

```html
{% if users[0].authorized %}
    {{ users[0].name }} авторизован
{% endif %}
```

### Циклы

Итерация по массиву:

```html
<ul>
{% for product in products %}
    <li>{{ product.name }} — {{ product.price }}</li>
{% endfor %}
</ul>
```

С доступом к индексу (второе имя после запятой):

```html
<ol>
{% for product, i in products %}
    <li>{{ i }}: {{ product.name }}</li>
{% endfor %}
</ol>
```

Если имя индекса не задано, оно по умолчанию равно `index`:

```html
{% for product in products %}
    <p>№ {{ index }}: {{ product.name }}</p>
{% endfor %}
```

Циклы также обходят **объекты** — в этом случае значением индекса служит имя ключа (строка), а элементом — значение поля:

```html
{% for value, key in settings %}
    <p>{{ key }} = {{ value }}</p>
{% endfor %}
```

Внутри цикла становятся доступны переменные с именами элемента и индекса; к их полям можно обращаться как к обычным переменным.

### Включение шаблонов

Включение другого файла выполняется тегом `{* include ... *}`. Путь указывается как есть, без кавычек — это путь внутри хранилища шаблонов.

```html
{* include /partials/header.tpl *}

<main>
    <h1>{{ title }}</h1>
</main>

{* include /partials/footer.tpl *}
```

::: warning Лимит вложенности
Глубина включений ограничена 100 уровнями. Циклические включения (шаблон включает сам себя без условий) приведут к ошибке парсинга.
:::

## Примеры использования

### Простой рендеринг

```c
#include "http.h"
#include "view.h"

void home(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Главная"));
    json_object_set(root, "username", json_create_string("Иван"));

    ctx->response->send_view(ctx->response, doc, "views", "/pages/home.tpl");

    json_free(doc);
}
```

### Рендеринг с форматированием пути

Путь поддерживает `printf`-форматирование — удобно, когда имя шаблона зависит от входных данных:

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

### Список товаров

```c
void products_list(httpctx_t* ctx) {
    json_doc_t* doc = json_root_create_object();
    json_token_t* root = json_root(doc);

    json_object_set(root, "title", json_create_string("Товары"));

    json_token_t* products = json_create_array(doc);

    json_token_t* p1 = json_create_object(doc);
    json_object_set(p1, "name", json_create_string("Ноутбук"));
    json_object_set(p1, "price", json_create_number(50000));
    json_array_append(products, p1);

    json_token_t* p2 = json_create_object(doc);
    json_object_set(p2, "name", json_create_string("Телефон"));
    json_object_set(p2, "price", json_create_number(30000));
    json_array_append(products, p2);

    json_object_set(root, "products", products);

    ctx->response->send_view(ctx->response, doc, "views", "/pages/products.tpl");

    json_free(doc);
}
```

Шаблон `/pages/products.tpl`:

```html
<!DOCTYPE html>
<html>
<head><title>{{ title }}</title></head>
<body>
    <h1>{{ title }}</h1>

    {% if products %}
        <table>
            <tr><th>#</th><th>Название</th><th>Цена</th></tr>
            {% for product, i in products %}
            <tr>
                <td>{{ i }}</td>
                <td>{{ product.name }}</td>
                <td>{{ product.price }} руб.</td>
            </tr>
            {% endfor %}
        </table>
    {% else %}
        <p>Товары не найдены</p>
    {% endif %}
</body>
</html>
```

### Рендеринг с данными модели

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
    json_object_set(root, "title", json_create_string("Профиль пользователя"));

    user_free(user);

    ctx->response->send_view(ctx->response, doc, "views", "/pages/profile.tpl");

    json_free(doc);
}
```

### Вложенные циклы

Допустим, есть документ с пользователями и их задачами:

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

Шаблон, обходящий пользователей и их задачи:

```html
{* include /header.tpl *}

Пользователи компании {{ company }}:
{% for user, i in users %}
    {{ i }}. {{ user.name }}
    Авторизован: {% if user.authorized %}да{% else %}нет{% endif %}
    Задачи:
    {% for task, j in user.tasks %}
        - {{ j }}: {{ task }}
    {% endfor %}
{% endfor %}
```

Внешний цикл объявляет элемент `user` и индекс `i`, внутренний — элемент `task` и индекс `j`.

## Организация шаблонов

Рекомендуемая структура хранилища:

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
            <a href="/">Главная</a>
            {% if is_authenticated %}
                <a href="/profile">Профиль</a>
                <a href="/logout">Выход</a>
            {% else %}
                <a href="/login">Вход</a>
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

## Экранирование и безопасность

::: warning Значения не экранируются автоматически
Шаблонизатор выводит строковые значения как есть, без HTML-экранирования. Если данные приходят от пользователя и вставляются в HTML, их необходимо экранировать заранее, иначе возможно выполнение произвольного HTML/JavaScript (XSS).
:::

Вывод значений, которые могут содержать спецсимволы (`<`, `>`, `&`, `"`), безопасно только после предварительной очистки на стороне обработчика.
