---
outline: deep
description: JSON-библиотека C Web Framework. Парсинг, генерация, работа с объектами и массивами. Пул токенов, потокобезопасность и поддержка Unicode.
---

# Библиотека JSON

Высокопроизводительная библиотека для работы с форматом JSON: парсинг, генерация, модификация и сериализация документов.

Токены выделяются из **пула памяти** (цепочка блоков по `TOKENS_PER_BLOCK` слотов) и переиспользуются между операциями, что исключает частые обращения к `malloc`/`free`. Менеджер пула инициализируется автоматически при первом вызове любой функции библиотеки.

::: tip Жизненный цикл
- Токены создаются **независимо от документа** — функции `json_create_*` не принимают документ.
- Документ хранит корневой токен; при уничтожении документа (`json_free`) освобождаются все его токены и строковый буфер.
- Строка из `json_stringify` принадлежит документу и живёт до следующего вызова или `json_free`. Чтобы строка пережила документ — используйте `json_stringify_detach` (тогда освобождать её нужно вручную через `free`).
:::

## Основные типы

- `json_doc_t` — документ: корневой токен, буфер сериализации и флаг `ascii_mode`.
- `json_token_t` — узел дерева значений: объект, массив, строка, число, логическое значение или `null`.
- `json_it_t` — итератор для обхода объекта или массива.

Поле `json_doc_t::ascii_mode` управляет сериализацией: `0` (по умолчанию) выводит UTF-8 как есть, `1` кодирует все символы вне ASCII как `\uXXXX`.

## Жизненный цикл документа

```c
json_doc_t* json_parse(const char* string);
```

Разбор строки и построение документа.

**Параметры**\
`string` — исходная JSON-строка (с нулевым байтом).

**Возвращаемое значение**\
Указатель на документ либо `NULL` при ошибке выделения памяти или некорректном JSON.

<br>

```c
json_doc_t* json_create_empty(void);
```

Создание пустого документа без корневого токена. Корень назначается отдельно через `json_set_root`.

**Возвращаемое значение**\
Указатель на документ либо `NULL` при ошибке выделения памяти.

<br>

```c
json_doc_t* json_root_create_object(void);
json_doc_t* json_root_create_array(void);
```

Создание документа с готовым корневым токеном — объектом или массивом соответственно. Эквивалентно `json_create_empty()` + `json_set_root()`.

**Возвращаемое значение**\
Указатель на документ либо `NULL` при ошибке выделения памяти.

<br>

```c
void json_set_root(json_doc_t* document, json_token_t* token);
```

Назначение `token` корневым токеном документа. Ничего не делает, если `document` или `token` равен `NULL`.

**Параметры**\
`document` — документ.\
`token` — токен, назначаемый корнем.

<br>

```c
void json_clear(json_doc_t* document);
void json_free(json_doc_t* document);
```

`json_clear` освобождает дерево токенов и строковый буфер, но оставляет саму структуру документа — в неё можно назначить новый корень.

`json_free` — это `json_clear` плюс освобождение самой структуры документа. Вызывайте по завершении работы с документом.

**Параметры**\
`document` — документ (допускается `NULL`).

<br>

```c
int json_copy(json_doc_t* from, json_doc_t* to);
```

Глубокое копирование документа `from` в существующий документ `to` (сериализация + повторный разбор). Старое содержимое `to` при этом не освобождается — при необходимости сначала вызовите `json_clear(to)`.

**Параметры**\
`from` — источник.\
`to` — приёмник.

**Возвращаемое значение**\
Ненулевое значение при успехе, `0` при ошибке.

<br>

## Корень и значения

```c
json_token_t* json_root(const json_doc_t* document);
```

Корневой токен документа.

**Возвращаемое значение**\
Указатель на токен либо `NULL`, если у документа нет корня.

<br>

```c
int         json_bool(const json_token_t* token);
const char* json_string(const json_token_t* token);
size_t      json_string_size(const json_token_t* token);
```

Прямое чтение логического и строкового значений — без выходного параметра `ok`.

- `json_bool` — логическое значение (`0`/`1`); `0` также при `NULL` или не-`bool`.
- `json_string` — указатель на строку с нулевым байтом; `NULL` при `NULL` или не-строке. Указатель принадлежит токену.
- `json_string_size` — длина строки в байтах **без** нулевого байта; `0` при `NULL` или не-строке.

<br>

```c
int          json_int(const json_token_t* token, int* ok);
unsigned int json_uint(const json_token_t* token, int* ok);
long long    json_llong(const json_token_t* token, int* ok);
double       json_double(const json_token_t* token, int* ok);
```

Чтение числа в нужном типе. Выходной параметр `ok` принимает `NULL` или указатель на `int`: в него записывается ненулевое значение, только если токен является числом и значение не вышло за пределы типа; иначе записывается `0`. При переполнении возвращается соответствующая граница (`INT_MAX`/`INT_MIN`, `UINT_MAX`, `LLONG_MAX`/`LLONG_MIN`, `DBL_MAX`).

::: tip Сначала проверьте тип
Перед чтением убедитесь, что токен — число, через `json_is_number(token)`, либо анализируйте `ok`.
:::

<br>

```c
long double json_ldouble(const json_token_t* token);
```

Число в исходной точности `long double` (внутренний формат хранения). Не имеет параметра `ok`: возвращает `0.0L` для не-числа или `NULL`, знак бесконечности сохраняется.

<br>

## Проверка типа

```c
int json_is_bool(const json_token_t* token);
int json_is_null(const json_token_t* token);
int json_is_string(const json_token_t* token);
int json_is_number(const json_token_t* token);
int json_is_object(const json_token_t* token);
int json_is_array(const json_token_t* token);
```

Проверка типа значения в токене.

**Параметры**\
`token` — указатель на токен.

**Возвращаемое значение**\
Ненулевое значение, если тип совпадает, иначе `0` (в том числе при `NULL`).

<br>

## Создание токенов

```c
json_token_t* json_create_bool(int value);
json_token_t* json_create_null(void);
json_token_t* json_create_string(const char* value);
json_token_t* json_create_number(long double value);
json_token_t* json_create_object(void);
json_token_t* json_create_array(void);
```

Создание нового токена заданного типа. Все числовые значения создаются одной функцией `json_create_number` — тип хранения числа единый (`long double`), а приведение к нужному типу выполняется при чтении (`json_int`, `json_double`, …).

**Параметры**\
`value` — значение для токена.

**Возвращаемое значение**\
Указатель на токен либо `NULL` при ошибке выделения памяти.

<br>

## Массивы

```c
int           json_array_prepend(json_token_t* array, json_token_t* token);
int           json_array_append(json_token_t* array, json_token_t* token);
int           json_array_append_to(json_token_t* array, int index, json_token_t* token);
int           json_array_erase(json_token_t* array, int index, int count);
int           json_array_clear(json_token_t* array);
int           json_array_size(const json_token_t* array);
json_token_t* json_array_get(const json_token_t* array, int index);
```

Управление массивом.

- `json_array_prepend` — вставка в начало (индекс 0).
- `json_array_append` — добавление в конец.
- `json_array_append_to` — вставка по индексу `index`; если `index` выходит за размер, элемент добавляется в конец.
- `json_array_erase` — удаление `count` элементов начиная с `index`.
- `json_array_clear` — удаление всех элементов (без удаления самого массива).
- `json_array_size` — количество элементов.
- `json_array_get` — элемент по индексу либо `NULL`.

**Параметры**\
`array` — массив. `token` — вставляемый токен. `index` — позиция элемента. `count` — количество элементов.

**Возвращаемое значение**\
Для модифицирующих операций и `json_array_size` — ненулевое/значение при успехе, `0` при ошибке аргументов. `json_array_get` — указатель на токен или `NULL`.

<br>

## Объекты

```c
int           json_object_set(json_token_t* object, const char* key, json_token_t* token);
json_token_t* json_object_get(const json_token_t* object, const char* key);
int           json_object_remove(json_token_t* object, const char* key);
int           json_object_size(const json_token_t* object);
int           json_object_clear(json_token_t* object);
```

Управление объектом.

- `json_object_set` — добавление пары `key` → `token`. Если ключ уже существует, старое значение заменяется и освобождается рекурсивно (вместе с вложенными токенами).
- `json_object_get` — значение по ключу либо `NULL`.
- `json_object_remove` — удаление пары по ключу.
- `json_object_clear` — удаление всех пар.
- `json_object_size` — количество пар.

**Параметры**\
`object` — объект. `key` — название свойства. `token` — значение.

**Возвращаемое значение**\
Для модифицирующих операций и `json_object_size` — ненулевое/значение при успехе, `0` при ошибке аргументов. `json_object_get` — указатель на токен или `NULL`.

<br>

## Изменение значения токена

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

Замена значения существующего токена: меняется тип и содержимое. Для `json_token_set_object` / `json_token_set_array` токен начинает ссылаться на переданный контейнер.

**Параметры**\
`token` — изменяемый токен. `value` — новое значение. `object` / `array` — контейнер.

**Возвращаемое значение**\
Нет

<br>

## Итератор

```c
json_it_t     json_init_it(const json_token_t* token);
int           json_end_it(const json_it_t* iterator);
const void*   json_it_key(const json_it_t* iterator);
json_token_t* json_it_value(const json_it_t* iterator);
json_it_t     json_next_it(json_it_t* iterator);
void          json_it_erase(json_it_t* iterator);
```

Единый итератор для обхода объекта или массива.

- `json_init_it` — создание итератора по токену-контейнеру.
- `json_end_it` — возвращает `1` (истина), если обход завершён.
- `json_it_key` — для объекта возвращает ключ (`const char*`), для массива — указатель на текущий индекс (`int*`), иначе `NULL`.
- `json_it_value` — значение текущего элемента.
- `json_next_it` — переход к следующему элементу.
- `json_it_erase` — удаление текущего элемента прямо во время обхода.

**Параметры**\
`token` — контейнер для инициализации итератора. `iterator` — итератор.

Типичный цикл обхода:

```c
for (json_it_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
    const char* key   = json_it_key(&it);        // ключ объекта
    json_token_t* val = json_it_value(&it);
    /* ... */
}
```

<br>

## Сериализация

```c
const char* json_stringify(json_doc_t* document);
size_t      json_stringify_size(json_doc_t* document);
char*       json_stringify_detach(json_doc_t* document);
```

- `json_stringify` — формирует строку из документа. Возвращает указатель, принадлежащий документу: он действителен до следующего вызова `json_stringify` или до `json_free`. `NULL`, если у документа нет корня или при ошибке.
- `json_stringify_size` — длина последней сформированной строки в байтах без нулевого байта.
- `json_stringify_detach` — формирует строку и возвращает её отдельную копию, владение которой переходит к вызывающему; при этом буфер документа очищается. Освобождайте возвращённый указатель через `free`.

Установите `document->ascii_mode = 1` перед сериализацией, чтобы все символы вне ASCII кодировались как `\uXXXX`.

**Параметры**\
`document` — документ.

**Возвращаемое значение**\
`json_stringify` — константный указатель на строку или `NULL`. `json_stringify_size` — размер. `json_stringify_detach` — указатель на копию (освобождает вызывающий) или `NULL`.

<br>

Готовые примеры парсинга, построения и сериализации — в разделе [Примеры JSON](/examples-json).
