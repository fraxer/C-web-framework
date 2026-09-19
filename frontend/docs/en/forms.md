---
outline: deep
description: Input validation and cleaning in C Web Framework. The form module — text, numbers, booleans, dates, UUIDs, JSON, files, choices and custom parsers.
---

# Input validation

Data received from the client — `get_payloadf` values, query parameters, JSON fields — arrives as strings, and without a common approach every handler manually checks them for emptiness, length, format and range. The form module (`form.h`, `framework/form`) moves these checks into a declarative description: you describe the field schema once, and on every request the form cleans the input, converts it to the proper type, applies the rules and collects ready-made error messages.

The flow. Ready-made handlers for every field type are collected in the [Input validation examples](/en/examples-form) section:

```c
form_input_t inputs[] = { /* raw strings and objects from the request */ };
form_t* form = form_create(&schema, inputs);  /* the form takes ownership of the input */

if (form_is_valid(form)) {
    const form_value_t* value = form_cleaned_data(form, 0);  /* validated value */
} else {
    const char* message = form_error_message(form, 0);       /* error message */
}

form_free(form);
```

## Key types

| Type | Purpose |
|------|---------|
| `form_field_spec_t` | Description of a single field: `kind` selects the type, the identically named union member holds its settings |
| `form_schema_t` | The set of fields, `clean`/`length` functions and the cross-field `validate` check |
| `form_input_t` | Raw input of a single field: text, object or a list of strings |
| `form_t` | The created form; owns the input until `form_free()` |
| `form_value_t` | The validated value: `kind` plus typed data in `data` |

## Full example: a registration form

A registration form with a login, email, password with confirmation and consent. Fields are addressed by index — convenient to declare the indices as an `enum`.

```c
#include "http.h"
#include "form.h"
#include "json.h"
#include "utf8.h"
#include <string.h>

enum { F_LOGIN, F_EMAIL, F_PASSWORD, F_PASSWORD_CONFIRM, F_CONSENT };

/* schema.clean runs before every check and must accept NULL. */
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

    /* Simplified check: local part, @, domain with a dot. */
    const char* at = strchr(value->data.text, '@');
    if (at == NULL || at == value->data.text || at[1] == '\0') return 0;
    return strchr(at + 1, '.') != NULL;
}

static const form_validator_t email_checks[] = {
    {
        .check = email_valid,
        .message = "Invalid email"
    }
};

static const form_field_spec_t register_fields[] = {
    [F_LOGIN] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "login",
            .required = 1,
            .required_message = "Enter a login"
        },
        .min_length = 3,
        .max_length = 16,
        .min_length_message = "Login is shorter than 3 characters",
        .max_length_message = "Login is longer than 16 characters",
        .regex = "\\A[A-Za-z][A-Za-z0-9_]*\\z",
        .regex_message = "A latin letter, then letters, digits or _"
    } },
    [F_EMAIL] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "email",
            .required = 1,
            .required_message = "Enter an email",
            .validators = email_checks,
            .validators_count = 1
        }
    } },
    [F_PASSWORD] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "password",
            .required = 1,
            .required_message = "Enter a password"
        },
        .min_length = 8,
        .min_length_message = "Password is shorter than 8 characters"
    } },
    [F_PASSWORD_CONFIRM] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "password_confirm",
            .required = 1,
            .required_message = "Repeat the password"
        }
    } },
    [F_CONSENT] = { .kind = FORM_FIELD_BOOLEAN, .boolean = {
        .common = {
            .name = "consent",
            .required = 1,
            .required_message = "Consent is required"
        }
    } }
};

/* Called after all fields have been validated. */
static void check_passwords(form_t* form, void* context) {
    (void)context;
    const form_value_t* password = form_field_value(form, F_PASSWORD);
    const form_value_t* confirm  = form_field_value(form, F_PASSWORD_CONFIRM);
    if (password == NULL || confirm == NULL) return;  /* the field already failed */

    if (strcmp(password->data.text, confirm->data.text) != 0)
        form_add_error(form, F_PASSWORD_CONFIRM, FORM_INVALID,
                       "Passwords do not match");
}

static const form_schema_t register_schema = {
    .fields = register_fields,
    .fields_count = sizeof register_fields / sizeof *register_fields,
    .clean = trim,
    .length = utf8_strlen,   /* length in characters, not bytes */
    .validate = check_passwords
};
```

The handler collects the raw values from the request body and runs them through the form:

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
    /* form_create() takes ownership of the strings in inputs even when
       creation fails — after this call only form_free() releases them. */
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

    /* The form is valid — the values are converted to their types. */
    const char* login = form_cleaned_data(form, F_LOGIN)->data.text;
    const char* email = form_cleaned_data(form, F_EMAIL)->data.text;
    int consent       = form_cleaned_data(form, F_CONSENT)->data.boolean;

    /* ... registration: password hashing, database insert, session ... */

    form_free(form);
    ctx->response->send_data(ctx->response, "Registered");
}
```

An example response for an empty form:

```json
{
  "login": "Enter a login",
  "email": "Enter an email",
  "password": "Enter a password",
  "password_confirm": "Repeat the password",
  "consent": "Consent is required"
}
```

::: tip Email validation
The example application ships a complete `validate_email()` — `app/auth/email_validator.h`; in your own application the `email_valid` validator can simply call it.
:::

## Validation order and lifecycle

Each field is validated in this order:

1. **Cleaning** — `schema.clean` edits the text buffer in place (for a list, every item). The function must accept `NULL` (missing input) and may return a pointer into the buffer. The field's `clean_flags` member is passed to it as the second argument — this way a single cleaning rule can distinguish modes for different fields.
2. **Required** — empty input on a field with `required` yields `FORM_REQUIRED`.
3. **Parsing** — the text is converted to the field type (`int64_t`, `long double`, date, UUID, JSON…). Failure yields `FORM_INVALID`.
4. **Value bounds** — text length, regular expression, numeric minimum/maximum, allowed choices.
5. **Validators** — additional checks from `common.validators`.
6. **Cross-field check** — `schema.validate` after all fields.

The first error message per field is kept — later checks of that field do not overwrite it. An empty optional field without a default yields a value with `kind == FORM_VALUE_EMPTY`. `form_is_valid()` validates once and caches the result; `form_cleaned_data()` is available only after the entire form validates, while `form_field_value()` already works inside `schema.validate` (returning `NULL` for a failed field).

Memory ownership:

- Text inputs (`FORM_INPUT_TEXT`) and lists (`FORM_INPUT_TEXT_LIST`) — the array and every string — move into the form's ownership; `form_create()` consumes them even on failure.
- Objects (`FORM_INPUT_OBJECT`) are released by the form through the `destroy` callback, when set.
- The schema, validator arrays, contexts and default values are borrowed and must outlive `form_free()` — declare them `static`.
- The `inputs` array itself may live on the stack.

## Common field settings

Every field type has a `common` member:

| Field | Type | Description |
|-------|------|-------------|
| `name` | `const char*` | Field name — handy for error messages and JSON keys |
| `required` | `int` | `1` — empty input is rejected with `FORM_REQUIRED` |
| `validators` | `const form_validator_t*` | Additional value checks |
| `validators_count` | `size_t` | Number of checks |
| `required_message` | `const char*` | Message for `FORM_REQUIRED` |
| `invalid_message` | `const char*` | Message for `FORM_INVALID` (parsing, validator) |
| `context` | `void*` | Context passed to a `FORM_FIELD_CUSTOM` parser |

## Text field

`FORM_FIELD_TEXT` accepts a string and returns `FORM_VALUE_TEXT`. Length bounds, a regular expression and a default value are supported:

```c
{ .kind = FORM_FIELD_TEXT, .text = {
    .common = {
        .name = "title",
        .required = 1,
        .required_message = "Enter a title"
    },
    .min_length = 2,
    .max_length = 100,
    .min_length_message = "Too short",
    .max_length_message = "Too long"
} }

/* An optional field with a default value and a pattern. */
{ .kind = FORM_FIELD_TEXT, .text = {
    .common = { .name = "code" },
    .default_value = "ABC-001",               /* borrowed; passes the checks too */
    .regex = "\\A[A-Z]{3}-[0-9]{3}\\z",
    .regex_message = "Expected a code like ABC-001"
} }
```

- Zero in `min_length` or `max_length` disables the bound.
- If any text field has a length rule, the schema must provide `length`; `utf8_strlen` from `utf8.h` counts UTF-8 characters, `strlen` counts bytes.
- `regex` is compiled (PCRE2, UTF-8 mode) at `form_create()`; a broken pattern returns `NULL`. The pattern **searches** for a match, so anchor it to the whole string with `\\A`…`\\z` (doubled backslashes in a C string literal).

## Numeric fields

`FORM_FIELD_INTEGER` parses the text with `strtoll()` and returns `FORM_VALUE_INTEGER` holding `int64_t`. Invalid text and overflow yield `FORM_INVALID`. The `has_default`, `has_min_value`, `has_max_value` flags exist because zero is a valid bound value:

```c
/* Required age from 18 to 120 inclusive. */
{ .kind = FORM_FIELD_INTEGER, .integer = {
    .common = {
        .name = "age",
        .required = 1,
        .required_message = "Enter an age",
        .invalid_message = "Enter an integer"
    },
    .has_min_value = 1,
    .min_value = 18,
    .has_max_value = 1,
    .max_value = 120,
    .min_value_message = "Age is under 18",
    .max_value_message = "Age is over 120"
} }

/* An optional counter: empty input becomes 0. */
{ .kind = FORM_FIELD_INTEGER, .integer = {
    .common = { .name = "count" },
    .has_default = 1,
    .default_value = 0,
    .has_min_value = 1,
    .min_value = 0,
    .min_value_message = "The number must be non-negative"
} }
```

`FORM_FIELD_DECIMAL` parses the text with `strtold()` and returns a `long double` in `FORM_VALUE_DECIMAL`; `NaN`, infinity and out-of-range values are rejected:

```c
{ .kind = FORM_FIELD_DECIMAL, .decimal = {
    .common = {
        .name = "rate",
        .required = 1,
        .invalid_message = "Enter a number"
    },
    .has_min_value = 1,
    .min_value = 0.5L,
    .has_max_value = 1,
    .max_value = 1.5L,
    .min_value_message = "Too small",
    .max_value_message = "Too large"
} }
```

::: warning Money
`long double` is a binary floating-point number: it does not provide exact decimal arithmetic. For monetary amounts store integer cents or a string.
:::

## Boolean field

`FORM_FIELD_BOOLEAN` accepts the text `true`, `1`, `false`, `0`; any other text yields `FORM_INVALID`:

```c
/* Required consent: false and 0 yield FORM_REQUIRED. */
{ .kind = FORM_FIELD_BOOLEAN, .boolean = {
    .common = {
        .name = "consent",
        .required = 1,
        .required_message = "Consent is required"
    }
} }

/* An optional flag: empty input becomes false. */
{ .kind = FORM_FIELD_BOOLEAN, .boolean = {
    .common = { .name = "notify" },
    .has_default = 1,
    .default_value = 0
} }
```

The input is supplied as `FORM_INPUT_TEXT` — the form parses the string itself.

## Date, time and UUID

Built-in formats: date `YYYY-MM-DD`, time `HH:MM[:SS[.ffffff]]`, date and time `YYYY-MM-DDTHH:MM[:SS[.ffffff]]` with an optional `Z` or `+HH:MM`/`-HH:MM` offset. `DateTimeField` keeps the offset in `offset_minutes` but does not convert the time to a local timezone. A UUID accepts the dashed form and 32 hexadecimal digits without dashes:

```c
static const form_field_spec_t temporal_fields[] = {
    { .kind = FORM_FIELD_DATE, .date = {
        .common = {
            .name = "birthday",
            .required = 1,
            .invalid_message = "Enter a date as YYYY-MM-DD"
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

/* After form_is_valid(form): */
form_date_t     date     = form_cleaned_data(form, 0)->data.date;      /* year, month, day */
form_time_t     time     = form_cleaned_data(form, 1)->data.time;      /* hour, minute, second, microsecond */
form_datetime_t datetime = form_cleaned_data(form, 2)->data.datetime;  /* date + time + offset_minutes */
form_uuid_t     uuid     = form_cleaned_data(form, 3)->data.uuid;      /* bytes[16] */
```

For other date formats use `FORM_FIELD_CUSTOM` with your own parser.

## JSON

`FORM_FIELD_JSON` accepts text, checks the syntax and returns a `json_doc_t*` in `FORM_VALUE_OBJECT`. The document belongs to the form and lives until `form_free()`:

```c
static const form_field_spec_t settings_field = {
    .kind = FORM_FIELD_JSON,
    .json = {
        .common = {
            .name = "settings",
            .required = 1,
            .invalid_message = "Invalid JSON"
        }
    }
};

/* After successful validation: */
json_doc_t* doc = form_cleaned_data(form, 0)->data.object;
json_token_t* root = json_root(doc);
```

For an optional JSON you can set `default_value` — a borrowed `json_doc_t*`; the caller keeps it until `form_free()`.

## Choosing one or several values

`FORM_FIELD_CHOICE` checks the value against a list of allowed options. `FORM_FIELD_MULTIPLE_CHOICE` does the same for a list of strings — order and duplicates are preserved, values are checked individually:

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
        .invalid_choice_message = "Unknown color"
    } },
    { .kind = FORM_FIELD_MULTIPLE_CHOICE, .multiple_choice = {
        .common = { .name = "extra_colors" },
        .choices = colors,
        .choices_count = 3
    } }
};

/* The list array and every string in it move into the form's ownership. */
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
    /* result->count == 2; items borrow the strings from inputs. */
}
form_free(form);
```

An optional empty list is valid and yields `FORM_VALUE_TEXT_LIST` with `count == 0`. For a default value set the borrowed `default_values` and `default_values_count`.

## Uploaded file

`FORM_FIELD_FILE` accepts an uploaded file (a `file_content_t*` from `get_payload_filef`) as `FORM_INPUT_OBJECT` and checks the name, size and empty file:

```c
static const form_field_spec_t document_field = {
    .kind = FORM_FIELD_FILE,
    .file = {
        .common = {
            .name = "document",
            .required = 1,
            .required_message = "Attach a file"
        },
        .max_size = 5 * 1024 * 1024,
        .max_filename_length = 100,
        .max_size_message = "File is too large",
        .empty_file_message = "File is empty"
    }
};

void upload_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    file_content_t* uploaded = malloc(sizeof *uploaded);  /* the form frees the copy */
    if (uploaded == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }
    *uploaded = request->get_payload_filef(request, "document");
    /* uploaded->ok == 0 when there is no file: a required field yields FORM_REQUIRED. */

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

    /* The file is valid — save it to storage before the request ends. */
    file_content_t* file = form_cleaned_data(form, 0)->data.object;
    if (!storage_file_content_put("remote", file, "documents/%s", file->filename))
        ctx->response->send_default(ctx->response, 500);
    else
        ctx->response->send_data(ctx->response, "Uploaded");

    form_free(form);
}
```

- By default a zero-size file is rejected (`allow_empty_file = 1` changes that).
- The `file->fd` descriptor belongs to the HTTP request: `free(uploaded)` does not close it. Use the file before the request ends, or save a copy.
- The limit on the whole HTTP body (`client_max_body_size`) applies earlier — a validator sees the file only after it has been received.

For custom content checks — a signature, magic bytes — use `FORM_FIELD_OBJECT` with validators:

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

A signature check is more reliable than checking the file extension.

## Storage path

`FORM_FIELD_FILEPATH` validates a **name inside a storage directory** and returns that name in `FORM_VALUE_TEXT`. The storage is a name from the `storages` section of `config.json`, the directory is a path inside it (`NULL` or `""` means the root). Nested paths, `.`/`..`, symlinks and names containing `*` are rejected:

```c
static const form_field_spec_t template_field = {
    .kind = FORM_FIELD_FILEPATH,
    .filepath = {
        .common = { .name = "template" },
        .storage = "templates",
        .directory = "public",
        .allow_files = 1,          /* set allow_directories = 1 to pick directories */
        .invalid_choice_message = "File not found"
    }
};

/* After validation, pass the name to storage functions as an argument, not a format: */
const char* name = form_cleaned_data(form, 0)->data.text;
storage_file_get("templates", "public/%s", name);
```

The check reflects the storage state at validation time — the file may be gone by the time it is opened. On S3 a directory is a prefix holding at least one object, and every check is a request to the storage.

## Custom parsing

`FORM_FIELD_CUSTOM` is used when the built-in types cannot parse the input. The parser receives the input, fills a `form_value_t` and returns `1` on success. A hexadecimal number instead of a decimal one:

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
            .invalid_message = "Invalid number"
        },
        .default_value = &default_hex,   /* borrowed */
        .parse = parse_hex
    }
};
```

Parser ownership rules:

- If the parser creates an object with `malloc`, it returns `FORM_VALUE_OBJECT` with `destroy = free` — the form calls it on release.
- If the parser returns the input object as is, the result's `destroy` must be `NULL`, so the object is not freed twice.
- The built-in parsers `form_parse_integer`, `form_parse_decimal` and `form_parse_boolean` are available to `CUSTOM` fields too — for example, to parse a number and box it into an object.

## Validators

A validator is a function `int (*check)(const form_value_t* value, void* context)`: `1` — the value is acceptable, `0` — an error with the message from `message`. The `form_validator_t.context` is passed to the validator and can hold check parameters:

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
        .message = "The number must be divisible by five"
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

Validators run in order after the built-in checks; the first message per field is kept.

## Validating several fields together

`schema.validate` is called after all fields have been validated. Inside it `form_field_value()` returns the field value (`NULL` if the field failed; `FORM_VALUE_EMPTY` for an empty optional one). An error is attached to a field with `form_add_error()` or to the whole form with `form_add_non_field_error()`:

```c
enum { F_START, F_END };

static void check_range(form_t* form, void* context) {
    (void)context;
    const form_value_t* start = form_field_value(form, F_START);
    const form_value_t* end   = form_field_value(form, F_END);
    if (start == NULL || end == NULL ||
        start->kind != FORM_VALUE_INTEGER ||
        end->kind != FORM_VALUE_INTEGER)
        return;   /* the fields already have their own errors */

    if (start->data.integer > end->data.integer)
        form_add_error(form, F_END, FORM_INVALID,
                       "Range end precedes its start");
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

## Error codes

`form_error_code()` returns the code of the field's first error:

| Code | Reason |
|------|--------|
| `FORM_VALID` | No error |
| `FORM_REQUIRED` | A required field is empty |
| `FORM_MIN_LENGTH` / `FORM_MAX_LENGTH` | Text length out of bounds |
| `FORM_MIN_VALUE` / `FORM_MAX_VALUE` | Number out of bounds |
| `FORM_INVALID` | Parsing or a validator failed |
| `FORM_REGEX` | Text does not match the pattern |
| `FORM_MAX_SIZE` | File larger than `max_size` |
| `FORM_EMPTY_FILE` | File is empty |
| `FORM_INVALID_CHOICE` | Value outside the options list, or path not found |

`form_error_message()` picks the first non-empty of: the message added by a validator or `schema.validate`, and the message of the specific rule (`min_length_message`, `regex_message`…) for the error code. If nothing is set it returns `NULL` — set the rule texts in the field description.
