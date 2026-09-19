---
outline: deep
description: Обработка и валидация входных данных в C Web Framework. Модуль форм — текст, числа, булевы значения, даты, UUID, JSON, файлы, выбор вариантов и собственные парсеры.
---

# Обработка входных данных

Данные, полученные от клиента — значения `get_payloadf`, query-параметры, поля JSON — приходят строками, и без единого подхода каждый обработчик вручную проверяет их на пустоту, длину, формат и диапазон. Модуль форм (`form.h`, `framework/form`) переносит эти проверки в декларативное описание: вы один раз описываете схему полей, а на каждом запросе форма сама очищает вход, приводит его к нужному типу, применяет правила и собирает готовые сообщения об ошибках.

Схема работы. Готовые обработчики для всех типов полей собраны в разделе
[Примеры обработки входных данных](/examples-form):

```c
form_input_t inputs[] = { /* сырые строки и объекты из запроса */ };
form_t* form = form_create(&schema, inputs);  /* форма забирает вход во владение */

if (form_is_valid(form)) {
    const form_value_t* value = form_cleaned_data(form, 0);  /* проверенное значение */
} else {
    const char* message = form_error_message(form, 0);       /* сообщение об ошибке */
}

form_free(form);
```

## Ключевые типы

| Тип | Назначение |
|-----|-----------|
| `form_field_spec_t` | Описание одного поля: `kind` выбирает тип, а одноимённый член объединения хранит настройки |
| `form_schema_t` | Набор полей, функции `clean`/`length` и межполевая проверка `validate` |
| `form_input_t` | Сырой вход одного поля: текст, объект или список строк |
| `form_t` | Созданная форма; владеет входом до `form_free()` |
| `form_value_t` | Проверенное значение: вид `kind` и типизированные данные в `data` |

## Полный пример: форма регистрации

Форма регистрации с логином, email, паролем с подтверждением и согласием. Поля адресуются по индексу — удобно объявить его константами через `enum`.

```c
#include "http.h"
#include "form.h"
#include "json.h"
#include "utf8.h"
#include <string.h>

enum { F_LOGIN, F_EMAIL, F_PASSWORD, F_PASSWORD_CONFIRM, F_CONSENT };

/* schema.clean вызывается до всех проверок и должна принимать NULL. */
static char* trim(char* text, int flags) {
    (void)flags;
    if (text == NULL) return NULL;

    while (*text == ' ') text++;
    size_t length = strlen(text);
    while (length > 0 && text[length - 1] == ' ')
        text[--length] = '\0';
    return text;
}

static int email_valid(const form_value_t* value, void* context) {
    (void)context;
    if (value->kind != FORM_VALUE_TEXT) return 0;

    /* Упрощённая проверка: локальная часть, @, домен с точкой. */
    const char* at = strchr(value->data.text, '@');
    if (at == NULL || at == value->data.text || at[1] == '\0') return 0;
    return strchr(at + 1, '.') != NULL;
}

static const form_validator_t email_checks[] = {
    {
        .check = email_valid,
        .message = "Некорректный email"
    }
};

static const form_field_spec_t register_fields[] = {
    [F_LOGIN] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "login",
            .required = 1,
            .required_message = "Укажите логин"
        },
        .min_length = 3,
        .max_length = 16,
        .min_length_message = "Логин короче 3 символов",
        .max_length_message = "Логин длиннее 16 символов",
        .regex = "\\A[A-Za-z][A-Za-z0-9_]*\\z",
        .regex_message = "Латинская буква, затем буквы, цифры или _"
    } },
    [F_EMAIL] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "email",
            .required = 1,
            .required_message = "Укажите email",
            .validators = email_checks,
            .validators_count = 1
        }
    } },
    [F_PASSWORD] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "password",
            .required = 1,
            .required_message = "Укажите пароль"
        },
        .min_length = 8,
        .min_length_message = "Пароль короче 8 символов"
    } },
    [F_PASSWORD_CONFIRM] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "password_confirm",
            .required = 1,
            .required_message = "Повторите пароль"
        }
    } },
    [F_CONSENT] = { .kind = FORM_FIELD_BOOLEAN, .boolean = {
        .common = {
            .name = "consent",
            .required = 1,
            .required_message = "Необходимо согласие"
        }
    } }
};

/* Вызывается после проверки всех полей. */
static void check_passwords(form_t* form, void* context) {
    (void)context;
    const form_value_t* password = form_field_value(form, F_PASSWORD);
    const form_value_t* confirm  = form_field_value(form, F_PASSWORD_CONFIRM);
    if (password == NULL || confirm == NULL) return;  /* у поля уже есть ошибка */

    if (strcmp(password->data.text, confirm->data.text) != 0)
        form_add_error(form, F_PASSWORD_CONFIRM, FORM_INVALID,
                       "Пароли не совпадают");
}

static const form_schema_t register_schema = {
    .fields = register_fields,
    .fields_count = sizeof register_fields / sizeof *register_fields,
    .clean = trim,
    .length = utf8_strlen,   /* длина в символах, а не в байтах */
    .validate = check_passwords
};
```

Обработчик собирает сырые значения из тела запроса и прогоняет их через форму:

```c
void register_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_LOGIN] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "login")
        },
        [F_EMAIL] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "email")
        },
        [F_PASSWORD] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "password")
        },
        [F_PASSWORD_CONFIRM] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "password_confirm")
        },
        [F_CONSENT] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "consent")
        }
    };

    form_t* form = form_create(&register_schema, inputs);
    /* form_create() забирает строки из inputs во владение даже при ошибке
       создания — после этого вызова их освобождает только form_free(). */
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        json_doc_t* errors = json_root_create_object();
        json_token_t* object = json_root(errors);

        for (size_t i = 0; i < form_fields_count(form); i++) {
            if (form_error_code(form, i) == FORM_VALID) continue;
            json_object_set(object, form_field_name(form, i),
                            json_create_string(form_error_message(form, i)));
        }

        const char* non_field = form_non_field_error(form);
        if (non_field != NULL)
            json_object_set(object, "form", json_create_string(non_field));

        ctx->response->send_json(ctx->response, errors);
        json_free(errors);
        form_free(form);
        return;
    }

    /* Форма валидна — значения приведены к типам. */
    const char* login = form_cleaned_data(form, F_LOGIN)->data.text;
    const char* email = form_cleaned_data(form, F_EMAIL)->data.text;
    int consent       = form_cleaned_data(form, F_CONSENT)->data.boolean;

    /* ... регистрация: хеширование пароля, запись в БД, сессия ... */

    form_free(form);
    ctx->response->send_data(ctx->response, "Registered");
}
```

Пример ответа при незаполненной форме:

```json
{
  "login": "Укажите логин",
  "email": "Укажите email",
  "password": "Укажите пароль",
  "password_confirm": "Повторите пароль",
  "consent": "Необходимо согласие"
}
```

::: tip Проверка email
В примере приложения есть полноценный `validate_email()` — `app/auth/email_validator.h`; в собственном приложении валидатор `email_valid` может просто вызывать его.
:::

## Порядок проверки и жизненный цикл

Каждое поле проверяется в таком порядке:

1. **Очистка** — `schema.clean` правит текстовый буфер на месте (для списка — каждый элемент). Функция обязательна должна принимать `NULL` (отсутствующий вход) и может возвращать указатель внутрь буфера. Член `clean_flags` у поля передаётся в неё вторым аргументом — так одно и то же правило очистки различает режимы для разных полей.
2. **Обязательность** — пустой ввод у поля с `required` даёт `FORM_REQUIRED`.
3. **Разбор** — текст приводится к типу поля (`int64_t`, `long double`, дата, UUID, JSON…). Неудача даёт `FORM_INVALID`.
4. **Границы значения** — длина текста, регулярное выражение, минимум/максимум числа, допустимые варианты.
5. **Валидаторы** — дополнительные проверки из `common.validators`.
6. **Межполевая проверка** — `schema.validate` после всех полей.

Первое сообщение об ошибке для каждого поля сохраняется — последующие проверки поля не перезаписывают его. Пустое необязательное поле без значения по умолчанию даёт значение с `kind == FORM_VALUE_EMPTY`. `form_is_valid()` выполняет проверку один раз и кеширует результат; `form_cleaned_data()` доступен только после успешной проверки всей формы, а `form_field_value()` — уже внутри `schema.validate` (для поля с ошибкой возвращает `NULL`).

Владение памятью:

- Текстовые входы (`FORM_INPUT_TEXT`) и списки (`FORM_INPUT_TEXT_LIST`) — массив и каждая строка — переходят во владение формы; `form_create()` поглощает их даже при неудаче.
- Объекты (`FORM_INPUT_OBJECT`) форма освобождает через колбэк `destroy`, если он задан.
- Схема, массивы валидаторов, контексты и значения по умолчанию заимствованы и должны жить до `form_free()` — объявляйте их `static`.
- Сам массив `inputs` может лежать на стеке.

## Общие настройки поля

Член `common` есть у каждого типа поля:

| Поле | Тип | Описание |
|------|-----|----------|
| `name` | `const char*` | Имя поля — удобно для сообщений об ошибках и ключей JSON |
| `required` | `int` | `1` — пустой ввод отвергается с `FORM_REQUIRED` |
| `validators` | `const form_validator_t*` | Дополнительные проверки значения |
| `validators_count` | `size_t` | Количество проверок |
| `required_message` | `const char*` | Сообщение для `FORM_REQUIRED` |
| `invalid_message` | `const char*` | Сообщение для `FORM_INVALID` (разбор, валидатор) |
| `context` | `void*` | Контекст, передаваемый пользовательскому парсеру `FORM_FIELD_CUSTOM` |

## Текстовое поле

`FORM_FIELD_TEXT` принимает строку и возвращает `FORM_VALUE_TEXT`. Поддерживаются границы длины, регулярное выражение и значение по умолчанию:

```c
{ .kind = FORM_FIELD_TEXT, .text = {
    .common = {
        .name = "title",
        .required = 1,
        .required_message = "Введите название"
    },
    .min_length = 2,
    .max_length = 100,
    .min_length_message = "Слишком коротко",
    .max_length_message = "Слишком длинно"
} }

/* Необязательное поле со значением по умолчанию и шаблоном. */
{ .kind = FORM_FIELD_TEXT, .text = {
    .common = { .name = "code" },
    .default_value = "ABC-001",               /* заимствовано, тоже проходит проверки */
    .regex = "\\A[A-Z]{3}-[0-9]{3}\\z",
    .regex_message = "Ожидается код вида ABC-001"
} }
```

- Ноль в `min_length` или `max_length` отключает границу.
- Если у любого текстового поля есть правило длины, схема обязана предоставлять `length`; `utf8_strlen` из `utf8.h` считает символы UTF-8, `strlen` — байты.
- `regex` компилируется (PCRE2, режим UTF-8) при `form_create()`; ошибка в шаблоне возвращает `NULL`. Шаблон **ищет совпадение**, поэтому для привязки ко всей строке используйте `\\A`…`\\z` (в строковом литерале C — двойной обратный слэш).

## Числовые поля

`FORM_FIELD_INTEGER` разбирает текст через `strtoll()` и возвращает `FORM_VALUE_INTEGER` с `int64_t`. Неверный текст и переполнение дают `FORM_INVALID`. Флаги `has_default`, `has_min_value`, `has_max_value` нужны потому, что ноль — допустимое значение границы:

```c
/* Обязательный возраст от 18 до 120 включительно. */
{ .kind = FORM_FIELD_INTEGER, .integer = {
    .common = {
        .name = "age",
        .required = 1,
        .required_message = "Укажите возраст",
        .invalid_message = "Введите целое число"
    },
    .has_min_value = 1,
    .min_value = 18,
    .has_max_value = 1,
    .max_value = 120,
    .min_value_message = "Возраст меньше 18",
    .max_value_message = "Возраст больше 120"
} }

/* Необязательный счётчик: пустой ввод превращается в 0. */
{ .kind = FORM_FIELD_INTEGER, .integer = {
    .common = { .name = "count" },
    .has_default = 1,
    .default_value = 0,
    .has_min_value = 1,
    .min_value = 0,
    .min_value_message = "Число должно быть неотрицательным"
} }
```

`FORM_FIELD_DECIMAL` разбирает текст через `strtold()` и возвращает `long double` в `FORM_VALUE_DECIMAL`; `NaN`, бесконечность и выход за диапазон отклоняются:

```c
{ .kind = FORM_FIELD_DECIMAL, .decimal = {
    .common = {
        .name = "rate",
        .required = 1,
        .invalid_message = "Введите число"
    },
    .has_min_value = 1,
    .min_value = 0.5L,
    .has_max_value = 1,
    .max_value = 1.5L,
    .min_value_message = "Слишком мало",
    .max_value_message = "Слишком много"
} }
```

::: warning Деньги
`long double` — двоичное число с плавающей точкой: он не обеспечивает точную десятичную арифметику. Для денежных сумм храните целые копейки или строку.
:::

## Булево поле

`FORM_FIELD_BOOLEAN` принимает текст `true`, `1`, `false`, `0`; другой текст даёт `FORM_INVALID`:

```c
/* Обязательное согласие: false и 0 дают FORM_REQUIRED. */
{ .kind = FORM_FIELD_BOOLEAN, .boolean = {
    .common = {
        .name = "consent",
        .required = 1,
        .required_message = "Необходимо согласие"
    }
} }

/* Необязательный флаг: пустой ввод превращается в false. */
{ .kind = FORM_FIELD_BOOLEAN, .boolean = {
    .common = { .name = "notify" },
    .has_default = 1,
    .default_value = 0
} }
```

Вход подаётся как `FORM_INPUT_TEXT` — форма сама разбирает строку.

## Дата, время и UUID

Встроенные форматы: дата `YYYY-MM-DD`, время `HH:MM[:SS[.ffffff]]`, дата и время `YYYY-MM-DDTHH:MM[:SS[.ffffff]]` с необязательным `Z` или смещением `+HH:MM`/`-HH:MM`. `DateTimeField` сохраняет смещение в `offset_minutes`, но не приводит время к локальному часовому поясу. UUID допускает запись с дефисами и 32 шестнадцатеричных цифры без них:

```c
static const form_field_spec_t temporal_fields[] = {
    { .kind = FORM_FIELD_DATE, .date = {
        .common = {
            .name = "birthday",
            .required = 1,
            .invalid_message = "Введите дату в формате YYYY-MM-DD"
        }
    } },
    { .kind = FORM_FIELD_TIME, .time = {
        .common = { .name = "starts_at" },
        .has_default = 1,
        .default_value = { .hour = 9 }
    } },
    { .kind = FORM_FIELD_DATETIME, .datetime = {
        .common = { .name = "published_at" }
    } },
    { .kind = FORM_FIELD_UUID, .uuid = {
        .common = {
            .name = "token",
            .required = 1
        }
    } }
};

/* После form_is_valid(form): */
form_date_t     date     = form_cleaned_data(form, 0)->data.date;      /* year, month, day */
form_time_t     time     = form_cleaned_data(form, 1)->data.time;      /* hour, minute, second, microsecond */
form_datetime_t datetime = form_cleaned_data(form, 2)->data.datetime;  /* date + time + offset_minutes */
form_uuid_t     uuid     = form_cleaned_data(form, 3)->data.uuid;      /* bytes[16] */
```

Для других форматов даты оставьте `FORM_FIELD_CUSTOM` с собственным парсером.

## JSON

`FORM_FIELD_JSON` принимает текст, проверяет синтаксис и возвращает `json_doc_t*` в `FORM_VALUE_OBJECT`. Документ принадлежит форме и живёт до `form_free()`:

```c
static const form_field_spec_t settings_field = {
    .kind = FORM_FIELD_JSON,
    .json = {
        .common = {
            .name = "settings",
            .required = 1,
            .invalid_message = "Некорректный JSON"
        }
    }
};

/* После успешной проверки: */
json_doc_t* doc = form_cleaned_data(form, 0)->data.object;
json_token_t* root = json_root(doc);
```

Для необязательного JSON можно указать `default_value` — заимствованный `json_doc_t*`; вызывающий код хранит его до `form_free()`.

## Выбор одного или нескольких значений

`FORM_FIELD_CHOICE` проверяет значение по списку допустимых вариантов. `FORM_FIELD_MULTIPLE_CHOICE` делает то же для списка строк — порядок и повторы сохраняются, значения проверяются по отдельности:

```c
static const char* const colors[] = { "red", "green", "blue" };

static const form_field_spec_t color_fields[] = {
    { .kind = FORM_FIELD_CHOICE, .choice = {
        .common = {
            .name = "color",
            .required = 1
        },
        .choices = colors,
        .choices_count = 3,
        .invalid_choice_message = "Цвет не найден"
    } },
    { .kind = FORM_FIELD_MULTIPLE_CHOICE, .multiple_choice = {
        .common = { .name = "extra_colors" },
        .choices = colors,
        .choices_count = 3
    } }
};

/* Массив списка и каждая его строка переходят во владение формы. */
char** selected = calloc(2, sizeof *selected);
selected[0] = strdup("red");
selected[1] = strdup("blue");

form_input_t inputs[] = {
    {
        .kind = FORM_INPUT_TEXT,
        .data.text = strdup("green")
    },
    {
        .kind = FORM_INPUT_TEXT_LIST,
        .data.text_list = { selected, 2 }
    }
};

form_schema_t schema = {
    .fields = color_fields,
    .fields_count = 2
};
form_t* form = form_create(&schema, inputs);
if (form != NULL && form_is_valid(form)) {
    const form_value_text_list_t* result =
        &form_cleaned_data(form, 1)->data.text_list;
    /* result->count == 2; элементы заимствуют строки из inputs. */
}
form_free(form);
```

Необязательный пустой список валиден и даёт `FORM_VALUE_TEXT_LIST` с `count == 0`. Для значения по умолчанию укажите заимствованные `default_values` и `default_values_count`.

## Загруженный файл

`FORM_FIELD_FILE` принимает загруженный файл (`file_content_t*` из `get_payload_filef`) как `FORM_INPUT_OBJECT` и проверяет имя, размер и пустой файл:

```c
static const form_field_spec_t document_field = {
    .kind = FORM_FIELD_FILE,
    .file = {
        .common = {
            .name = "document",
            .required = 1,
            .required_message = "Приложите файл"
        },
        .max_size = 5 * 1024 * 1024,
        .max_filename_length = 100,
        .max_size_message = "Слишком большой файл",
        .empty_file_message = "Файл пуст"
    }
};

void upload_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    file_content_t* uploaded = malloc(sizeof *uploaded);  /* форма освободит копию */
    if (uploaded == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }
    *uploaded = request->get_payload_filef(request, "document");
    /* uploaded->ok == 0, если файла нет: поле с required даст FORM_REQUIRED. */

    form_input_t input = {
        .kind = FORM_INPUT_OBJECT,
        .data.object = uploaded,
        .destroy = free
    };
    form_schema_t schema = {
        .fields = &document_field,
        .fields_count = 1
    };

    form_t* form = form_create(&schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        ctx->response->send_data(ctx->response, form_error_message(form, 0));
        form_free(form);
        return;
    }

    /* Файл валиден — сохраняем в хранилище до завершения запроса. */
    file_content_t* file = form_cleaned_data(form, 0)->data.object;
    if (!storage_file_content_put("remote", file, "documents/%s", file->filename))
        ctx->response->send_default(ctx->response, 500);
    else
        ctx->response->send_data(ctx->response, "Uploaded");

    form_free(form);
}
```

- По умолчанию файл нулевого размера недопустим (`allow_empty_file = 1` меняет это).
- Дескриптор `file->fd` принадлежит HTTP-запросу: `free(uploaded)` не закрывает его. Используйте файл до завершения запроса или сохраните копию.
- Лимит на размер всего HTTP-тела (`client_max_body_size`) применяется раньше — валидатор видит файл уже после приёма.

Для нестандартных проверок содержимого — сигнатура, magic bytes — используйте `FORM_FIELD_OBJECT` с валидаторами:

```c
static int pdf_signature_ok(const form_value_t* value, void* context) {
    (void)context;
    if (value->kind != FORM_VALUE_OBJECT) return 0;

    const file_content_t* file = value->data.object;
    if (!file->ok || file->size < 5) return 0;

    char signature[5];
    return pread(file->fd, signature, sizeof signature, file->offset) == 5 &&
           memcmp(signature, "%PDF-", sizeof signature) == 0;
}
```

Проверка сигнатуры надёжнее проверки расширения имени.

## Путь в хранилище

`FORM_FIELD_FILEPATH` проверяет **имя внутри каталога хранилища** и возвращает это имя в `FORM_VALUE_TEXT`. Хранилище задаётся именем из секции `storages` в `config.json`, каталог — путём внутри него (`NULL` или `""` означает корень). Вложенные пути, `.`/`..`, символьные ссылки и имена с `*` отклоняются:

```c
static const form_field_spec_t template_field = {
    .kind = FORM_FIELD_FILEPATH,
    .filepath = {
        .common = { .name = "template" },
        .storage = "templates",
        .directory = "public",
        .allow_files = 1,          /* укажите allow_directories = 1 для каталогов */
        .invalid_choice_message = "Файл не найден"
    }
};

/* После проверки имя передают в функции storage аргументом, а не форматом: */
const char* name = form_cleaned_data(form, 0)->data.text;
storage_file_get("templates", "public/%s", name);
```

Проверка отражает состояние хранилища на момент валидации — при последующем открытии файл мог исчезнуть. В S3 каталогом считается префикс, под которым есть хотя бы один объект, а каждая проверка — запрос к хранилищу.

## Собственный разбор

`FORM_FIELD_CUSTOM` применяется, когда встроенные типы не дают нужного разбора. Парсер получает вход, заполняет `form_value_t` и возвращает `1` при успехе. Шестнадцатеричное число вместо десятичного:

```c
#include <errno.h>

static int parse_hex(const form_input_t* input, form_value_t* value,
                     void* context) {
    (void)context;
    if (input->kind != FORM_INPUT_TEXT || input->data.text == NULL)
        return 0;

    errno = 0;
    char* end;
    long long number = strtoll(input->data.text, &end, 16);
    if (errno == ERANGE || end == input->data.text || *end != '\0')
        return 0;

    *value = (form_value_t){
        .kind = FORM_VALUE_INTEGER,
        .data.integer = (int64_t)number
    };
    return 1;
}

static const form_value_t default_hex = {
    .kind = FORM_VALUE_INTEGER,
    .data.integer = 42
};

static const form_field_spec_t hex_field = {
    .kind = FORM_FIELD_CUSTOM,
    .custom = {
        .common = {
            .name = "hex",
            .invalid_message = "Неверное число"
        },
        .default_value = &default_hex,   /* заимствовано */
        .parse = parse_hex
    }
};
```

Правила владения для парсера:

- Если парсер создаёт объект через `malloc`, он возвращает `FORM_VALUE_OBJECT` с `destroy = free` — форма вызовет его при освобождении.
- Если парсер возвращает входной объект как есть, `destroy` у результата должен быть `NULL`, чтобы объект не освободился дважды.
- Встроенные парсеры `form_parse_integer`, `form_parse_decimal` и `form_parse_boolean` доступны и для `CUSTOM`-полей — например, чтобы разобрать число и упаковать его в объект.

## Валидаторы

Валидатор — функция `int (*check)(const form_value_t* value, void* context)`: `1` — значение допустимо, `0` — ошибка с сообщением из `message`. Контекст `form_validator_t.context` передаётся валидатору и может хранить параметры проверки:

```c
static int64_t divisor = 5;

static int divisible_by(const form_value_t* value, void* context) {
    int64_t n = *(int64_t*)context;
    return value->kind == FORM_VALUE_INTEGER && n != 0 &&
           value->data.integer % n == 0;
}

static const form_validator_t multiple_of_five[] = {
    {
        .check = divisible_by,
        .context = &divisor,
        .message = "Число должно быть кратно пяти"
    }
};

static const form_field_spec_t quantity_field = {
    .kind = FORM_FIELD_INTEGER,
    .integer = {
        .common = {
            .name = "quantity",
            .validators = multiple_of_five,
            .validators_count = 1
        }
    }
};
```

Валидаторы выполняются по порядку после встроенных проверок; первое сообщение для поля сохраняется.

## Проверка нескольких полей вместе

`schema.validate` вызывается после проверок всех полей. Внутри него `form_field_value()` возвращает значение поля (`NULL`, если поле не прошло проверку; `FORM_VALUE_EMPTY` — пустое необязательное). Ошибка привязывается к полю через `form_add_error()` или к форме целиком через `form_add_non_field_error()`:

```c
enum { F_START, F_END };

static void check_range(form_t* form, void* context) {
    (void)context;
    const form_value_t* start = form_field_value(form, F_START);
    const form_value_t* end   = form_field_value(form, F_END);
    if (start == NULL || end == NULL ||
        start->kind != FORM_VALUE_INTEGER ||
        end->kind != FORM_VALUE_INTEGER)
        return;   /* у полей уже есть собственные ошибки */

    if (start->data.integer > end->data.integer)
        form_add_error(form, F_END, FORM_INVALID,
                       "Конец диапазона раньше начала");
}

static const form_field_spec_t range_fields[] = {
    { .kind = FORM_FIELD_INTEGER, .integer = {
        .common = {
            .name = "start",
            .required = 1
        }
    } },
    { .kind = FORM_FIELD_INTEGER, .integer = {
        .common = {
            .name = "end",
            .required = 1
        }
    } }
};

static const form_schema_t range_schema = {
    .fields = range_fields,
    .fields_count = 2,
    .validate = check_range
};
```

## Коды ошибок

`form_error_code()` возвращает код первой ошибки поля:

| Код | Причина |
|-----|---------|
| `FORM_VALID` | Ошибок нет |
| `FORM_REQUIRED` | Обязательное поле пусто |
| `FORM_MIN_LENGTH` / `FORM_MAX_LENGTH` | Длина текста вне границ |
| `FORM_MIN_VALUE` / `FORM_MAX_VALUE` | Число вне границ |
| `FORM_INVALID` | Не удался разбор или валидатор |
| `FORM_REGEX` | Текст не соответствует шаблону |
| `FORM_MAX_SIZE` | Файл больше `max_size` |
| `FORM_EMPTY_FILE` | Файл пустой |
| `FORM_INVALID_CHOICE` | Значение вне списка вариантов или путь не найден |

`form_error_message()` выбирает первое непустое из: сообщения, добавленного валидатором или `schema.validate`, и сообщения конкретного правила (`min_length_message`, `regex_message`…) для кода ошибки. Если ничего не задано, возвращается `NULL` — задавайте тексты правил в описании поля.
