---
outline: deep
description: Работа с Cookie в C Web Framework. Структура cookie_t, чтение и установка cookie, флаги Secure, HttpOnly, SameSite и удаление cookie.
---

# Cookie

Cookie позволяют сохранять данные между HTTP-запросами и чаще всего используются для сессий, токенов доступа и пользовательских настроек. Фреймворк поддерживает все современные атрибуты безопасности: `Secure`, `HttpOnly` и `SameSite`.

## Структура cookie

Cookie описывается структурой `cookie_t` (определена в `httpresponse.h`):

```c
typedef struct {
    const char* name;      // имя cookie (обязательно)
    const char* value;     // значение (обязательно)
    int seconds;           // время жизни в секундах
    const char* path;      // путь (NULL — пропустить)
    const char* domain;    // домен (NULL — пропустить)
    int secure;            // 1 — только по HTTPS
    int http_only;         // 1 — недоступно из JavaScript
    const char* same_site; // "Strict" | "Lax" | "None" (NULL — пропустить)
} cookie_t;
```

| Поле | Тип | Обязательное | Поведение |
|------|-----|--------------|----------|
| `name` | `const char*` | да | Если `NULL` или пустая строка — cookie игнорируется. |
| `value` | `const char*` | да | Если `NULL` — cookie игнорируется. |
| `seconds` | `int` | нет | `0` — удаляет cookie (`Max-Age=0`); `> 0` — задаёт время жизни через `Expires`. |
| `path` | `const char*` | нет | `NULL` или пусто — атрибут `Path` не добавляется. |
| `domain` | `const char*` | нет | `NULL` или пусто — атрибут `Domain` не добавляется. |
| `secure` | `int` | нет | `1` — добавляет флаг `Secure`, `0` — пропускает. |
| `http_only` | `int` | нет | `1` — добавляет флаг `HttpOnly`, `0` — пропускает. |
| `same_site` | `const char*` | нет | `NULL` или пусто — атрибут `SameSite` не добавляется. |

::: tip Как формируется заголовок Set-Cookie
При `seconds > 0` фреймворк рассчитывает дату истечения (`текущее время + seconds`) и добавляет атрибут `Expires` в формате RFC 1123 (`Wed, 21 Jun 2026 12:34:56 GMT`). Атрибут `Max-Age` при этом **не** добавляется. Чтобы немедленно удалить cookie, установите `seconds = 0` — будет отправлен `Max-Age=0`.
:::

## Чтение cookie

Метод `get_cookie` возвращает значение cookie по имени из текущего запроса. Вызывается через объект запроса.

```c
void get(httpctx_t* ctx) {
    const char* token = ctx->request->get_cookie(ctx->request, "token");

    if (token == NULL) {
        ctx->response->send_data(ctx->response, "Token not found");
        return;
    }

    ctx->response->send_data(ctx->response, token);
}
```

**Параметры**\
`name` — имя cookie. Поиск регистронезависимый.

**Возвращаемое значение**\
Указатель на значение cookie либо `NULL`, если cookie с таким именем нет в запросе. Указатель принадлежит запросу — освобождать его не нужно.

## Установка cookie

Метод `add_cookie` добавляет cookie в ответ — формирует заголовок `Set-Cookie` и присоединяет его к ответу. Вызывается через объект ответа.

```c
void get(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "token_value",
        .seconds = 3600,
        .path = "/",
        .domain = ".example.com",
        .secure = 1,
        .http_only = 1,
        .same_site = "Lax"
    });

    ctx->response->send_data(ctx->response, "Token successfully added");
}
```

**Параметры**\
`cookie` — структура `cookie_t`. Поля `name` и `value` обязательны; при их отсутствии cookie игнорируется.

## Удаление cookie

Отдельной функции удаления нет — cookie удаляется установкой `seconds = 0`. При этом отправляется `Max-Age=0`, и браузер немедленно отбрасывает cookie. Имя и `path` должны совпадать с теми, с которыми cookie был установлен.

```c
void logout(httpctx_t* ctx) {
    ctx->response->add_cookie(ctx->response, (cookie_t){
        .name = "token",
        .value = "",
        .seconds = 0,
        .path = "/"
    });

    ctx->response->send_data(ctx->response, "Logged out");
}
```

## SameSite

Атрибут `SameSite` управляет тем, отправляется ли cookie при кросс-сайтовых запросах:

| Значение | Поведение |
|----------|-----------|
| `Strict` | Cookie отправляется только при переходах внутри того же сайта. |
| `Lax` | Отправляется при顶层-навигации на сайт (значение по умолчанию в большинстве браузеров). |
| `None` | Отправляется во всех запросах, включая кросс-сайтовые. Требует флаг `Secure`. |

## Безопасность

- **`Secure`** — cookie передаётся только по HTTPS. Всегда включайте для аутентификационных cookie.
- **`HttpOnly`** — скрывает cookie от JavaScript (`document.cookie`), защищая от кражи через XSS. Включайте для сессионных cookie.
- **`SameSite`** — защищает от CSRF. `Lax` подходит большинству приложений; для cookie, которые нужны в iframe или кросс-сайтовых запросах, используйте `None` вместе с `Secure`.

Полный пример использования cookie совместно с сессиями — в разделе [Аутентификация](./auth.md).
