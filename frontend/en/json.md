---
outline: deep
description: Description of json library methods
---

# Json library

The library provides methods for working with the `json` format.

The document keeps a list of tokens and adds new tokens as needed.

When a document is destroyed, all memory associated with it, all tokens, are released.

Calling the `json_stringify_detach` method detaches the pointer to a string within the document, so the returned pointer to the string must be manually freed with the `free` function.

<br>

```C
jsondoc_t* json_init();
```

Creating a json document.

**Parameters**\
No

**Return value**\
Pointer to allocated memory of type `jsondoc_t`.

<br>

```C
int json_parse(jsondoc_t* document, const char* string);
```

Parsing a string and generating a json document.

**Parameters**\
`document` - pointer to the document to write.\
`string` - source string to parse.

**Return value**\
Nonzero if the parse succeeded, zero otherwise.

<br>

```C
void json_free(jsondoc_t* document);
```

Freeing the memory of the json document, including all tokens.

**Parameters**\
`document` - document pointer.

**Return value**\
No

<br>

```C
void json_token_reset(jsontok_t* token);
```

Reset the internal data of the token.

**Parameters**\
`token` - pointer to token.

**Return value**\
No

<br>

```C
jsontok_t* json_root(jsondoc_t* document);
```

Retrieve the document's root token.

**Parameters**\
`document` - document pointer.

**Return value**\
A pointer to a token.

<br>

```C
int json_ok(jsondoc_t* document);
```

Checking the correctness of the json document.

**Parameters**\
`document` - document pointer.

**Return value**\
Nonzero if the document is valid, zero otherwise.

<br>

```C
const char* json_error(jsondoc_t* document);
```

Output a string with an error message.

**Parameters**\
`document` - document pointer.

**Return value**\
If successful, returns a constant pointer to the beginning of the string.\
Returns a null pointer on error.

<br>

```C
int json_bool(const jsontok_t* token);
int json_int(const jsontok_t* token);
double json_double(const jsontok_t* token);
long long json_llong(const jsontok_t* token);
const char *json_string(const jsontok_t* token);
unsigned int json_uint(const jsontok_t* token);
```

Extracting a value from a token.

**Parameters**\
`token` - pointer to token.

**Return value**\
If successful, returns a value depending on the type.

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

Checking the type of the value in the token.

**Parameters**\
`token` - pointer to token.

**Return value**\
Nonzero if the expected value type, zero otherwise.

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

Create a new token with a type for the document.

**Parameters**\
`document` - document pointer.


**Return value**\
pointer to token.

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

Array management methods.

**Parameters**\
`array` - pointer to the array to write.\
`token` - pointer to token to insert.\
`index` - element position.\
`count` - the number of elements.

**Return value**\
If successful, returns a value depending on the type.

<br>

```C
int json_object_set(jsontok_t* object, const char* key, jsontok_t* token);
jsontok_t* json_object_get(const jsontok_t* object, const char* key);
int json_object_remove(jsontok_t* object, const char* key);
int json_object_size(const jsontok_t* object);
int json_object_clear(jsontok_t* object);
```

Object management methods.

**Parameters**\
`object` - pointer to the object to write.\
`key` - pointer to a string with the name of the property.\
`token` - pointer to the token to insert.

**Return value**\
If successful, returns a value depending on the type.

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

Setting a new value for the token.

**Parameters**\
`token` - a pointer to the token to insert.

**Return value**\
No

<br>

```C
jsonit_t json_init_it(const jsontok_t* token);
int json_end_it(const jsonit_t* iterator);
const void* json_it_key(const jsonit_t* iterator);
jsontok_t* json_it_value(const jsonit_t* iterator);
jsonit_t json_next_it(jsonit_t* iterator);
void json_it_erase(jsonit_t* iterator);
```

Iterator control when traversing an object or array.

**Parameters**\
`token` - pointer to a token to initialize the iterator.
`iterator` - pointer to an iterator.

**Return value**\
If successful, returns a value depending on the type.

<br>

```C
const char* json_stringify(jsondoc_t* document);
```

Formation of a string from a json document.

**Parameters**\
`document` - document pointer.

**Return value**\
If successful, returns a constant pointer to the beginning of the string.\
Returns a null pointer on error.

<br>

```C
char* json_stringify_detach(jsondoc_t* document);
```

Forming a string from a json document.\
You must manually free the memory when you are done with the string.

**Parameters**\
`document` - document pointer.

**Return value**\
If successful, returns a pointer to the beginning of the string.\
Returns a null pointer on error.

<br>

