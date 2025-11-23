---
outline: deep
description: JSON-библиотека C Web Framework. Парсинг, генерация, работа с объектами и массивами. Потокобезопасные операции и поддержка Unicode.
---

# Библиотека JSON

Высокопроизводительная библиотека для работы с форматом JSON. Поддерживает парсинг, генерацию, модификацию и сериализацию JSON-документов.

Документ хранит в себе список токенов и добавляет новые токены по мере необходимости.

При уничтожении документа освобождается вся связанная с ним память, все токены.

Вызов метода `json_stringify_detach` открепляет указатель на строку внутри документа, поэтому возвращаемый указатель на строку необходимо освободить вручную функцией `free`.

<br>

```C
jsondoc_t* json_init();
```

Создание json-документа.

**Параметры**\
Нет

**Возвращаемое значение**\
Указатель на выделенную память типа `jsondoc_t`.

<br>

```C
int json_parse(jsondoc_t* document, const char* string);
```

Анализ строки и формирование json-документа.

**Параметры**\
`document` - указатель на документ для записи.\
`string` - исходная строка для анализа.

**Возвращаемое значение**\
Ненулевое значение, если успешный анализ, и ноль в противном случае.

<br>

```C
void json_free(jsondoc_t* document);
```

Освобождение памяти json-документа, включая все токены.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
Нет

<br>

```C
void json_token_reset(jsontok_t* token);
```

Сброс внутренних данных токена.

**Параметры**\
`token` - указатель на токен.

**Возвращаемое значение**\
Нет

<br>

```C
jsontok_t* json_root(jsondoc_t* document);
```

Извлечение корневого токена документа.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
Указатель на токен.

<br>

```C
int json_ok(jsondoc_t* document);
```

Проверка корректности json-документа.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
Ненулевое значение, если документ корректный, и ноль в противном случае.

<br>

```C
const char* json_error(jsondoc_t* document);
```

Вывод строки с сообщением об ошибке.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
В случае успеха возвращает константный указатель на начало строки.\
В случае ошибки возвращает нулевой указатель.

<br>

```C
int json_bool(const jsontok_t* token);
int json_int(const jsontok_t* token);
double json_double(const jsontok_t* token);
long long json_llong(const jsontok_t* token);
const char *json_string(const jsontok_t* token);
unsigned int json_uint(const jsontok_t* token);
```

Извлечение значения из токена.

**Параметры**\
`token` - указатель на токен.

**Возвращаемое значение**\
В случае успеха возвращает значение в зависимости от типа.

<br>

```C
int json_is_bool(const jsontok_t* token);
int json_is_null(const jsontok_t* token);
int json_is_string(const jsontok_t* token);
int json_is_llong(const jsontok_t* token);
int json_is_int(const jsontok_t* token);
int json_is_uint(const jsontok_t* token);
int json_is_double(const jsontok_t* token);
int json_is_object(const jsontok_t* token);
int json_is_array(const jsontok_t* token);
```

Проверка типа значения в токене.

**Параметры**\
`token` - указатель на токен.

**Возвращаемое значение**\
Ненулевое значение, если ожидаемый тип значения, и ноль в противном случае.

<br>

```C
jsontok_t* json_create_bool(jsondoc_t* document, int bool);
jsontok_t* json_create_null(jsondoc_t* document);
jsontok_t* json_create_string(jsondoc_t* document, const char* string);
jsontok_t* json_create_llong(jsondoc_t* document, long long number);
jsontok_t* json_create_int(jsondoc_t* document, int number);
jsontok_t* json_create_uint(jsondoc_t* document, unsigned int number);
jsontok_t* json_create_double(jsondoc_t* document, double number);
jsontok_t* json_create_object(jsondoc_t* document);
jsontok_t* json_create_array(jsondoc_t* document);
```

Создание нового токена с типом для документа.

**Параметры**\
`document` - указатель на документ.


**Возвращаемое значение**\
Указатель на токен.

<br>

```C
int json_array_prepend(jsontok_t* array, jsontok_t* token);
int json_array_append(jsontok_t* array, jsontok_t* token);
int json_array_append_to(jsontok_t* array, int index, jsontok_t* token);
int json_array_erase(jsontok_t* array, int index, int count);
int json_array_clear(jsontok_t* array);
int json_array_size(const jsontok_t* array);
jsontok_t* json_array_get(const jsontok_t* array, int index);
```

Методы управления массивом.

**Параметры**\
`array` - указатель на массив для записи.\
`token` - указатель на токен для вставки.\
`index` - позиция элемента.\
`count` - количество элементов.

**Возвращаемое значение**\
В случае успеха возвращает значение в зависимости от типа.

<br>

```C
int json_object_set(jsontok_t* object, const char* key, jsontok_t* token);
jsontok_t* json_object_get(const jsontok_t* object, const char* key);
int json_object_remove(jsontok_t* object, const char* key);
int json_object_size(const jsontok_t* object);
int json_object_clear(jsontok_t* object);
```

Методы управления объектом.

**Параметры**\
`object` - указатель на объект для записи.\
`key` - указатель на строку с названием свойства.\
`token` - указатель на токен для вставки.

**Возвращаемое значение**\
В случае успеха возвращает значение в зависимости от типа.

<br>

```C
void json_token_set_bool(jsontok_t* token, int bool);
void json_token_set_null(jsontok_t* token);
void json_token_set_string(jsontok_t* token, const char* string);
void json_token_set_llong(jsontok_t* token, long long number);
void json_token_set_int(jsontok_t* token, int number);
void json_token_set_uint(jsontok_t* token, unsigned int number);
void json_token_set_double(jsontok_t* token, double number);
void json_token_set_object(jsontok_t* token, jsontok_t* object);
void json_token_set_array(jsontok_t* token, jsontok_t* array);
```

Установка нового значения для токена.

**Параметры**\
`token` - указатель на токен для вставки.

**Возвращаемое значение**\
Нет

<br>

```C
jsonit_t json_init_it(const jsontok_t* token);
int json_end_it(const jsonit_t* iterator);
const void* json_it_key(const jsonit_t* iterator);
jsontok_t* json_it_value(const jsonit_t* iterator);
jsonit_t json_next_it(jsonit_t* iterator);
void json_it_erase(jsonit_t* iterator);
```

Управление итератором при обходе объекта или массива.

**Параметры**\
`token` - указатель на токен для инициализации итератора.
`iterator` - указатель на итератор.

**Возвращаемое значение**\
В случае успеха возвращает значение в зависимости от типа.

<br>

```C
const char* json_stringify(jsondoc_t* document);
```

Формирование строки из json-документа.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
В случае успеха возвращает константный указатель на начало строки.\
В случае ошибки возвращает нулевой указатель.

<br>

```C
char* json_stringify_detach(jsondoc_t* document);
```

Формирование строки из json-документа.\
Необходимо вручную освободить память по окончанию работы со строкой.

**Параметры**\
`document` - указатель на документ.

**Возвращаемое значение**\
В случае успеха возвращает указатель на начало строки.\
В случае ошибки возвращает нулевой указатель.

<br>

