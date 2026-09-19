---
outline: deep
description: Handler examples with input validation through the form module for every field type of C Web Framework.
---

# Input validation examples

Ready-made handlers with validation for every field type of the form module: text, numbers, booleans, date and time, UUID, JSON, choices, files, storage paths, arbitrary objects and custom parsers. The full API description is in the [Input validation](/en/forms) section; extracting raw data from a request is covered in [Receiving data from the client](/en/payload).

## Setup

The handlers below are attached to routes as usual. Example for the profile form:

```json
// config.json
"routes": {
    "/profile": {
        "PUT": { "file": "handlers/forms/libprofile.so", "function": "profile_put" }
    }
}
```

Every example answers validation errors with a single JSON object `{"field": "message"}`. For that a shared function is used — place it in a common forms header or in each handler file:

```c
// handlers/forms/errors.h
#include "http.h"
#include "form.h"
#include "json.h"

static void send_form_errors(httpctx_t* ctx, form_t* form) {
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
}
```

## Text fields: profile

Three text fields: a required name with length bounds, an optional nickname with a regular expression, and an optional bio with a default value.

**Request**

```bash
curl http://example.com/profile \
    -X PUT \
    -d "name=Alexander&nickname=alex_7&bio=Backend developer"
```

**Handler**

```c
// handlers/forms/profile.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "utf8.h"
#include "errors.h"
#include <string.h>

static char* trim(char* text, int flags) {
    (void)flags;
    if (text == NULL) return NULL;

    while (*text == ' ') text++;
    size_t length = strlen(text);
    while (length > 0 && text[length - 1] == ' ')
        text[--length] = '\0';
    return text;
}

enum { F_NAME, F_NICKNAME, F_BIO };

static const form_field_spec_t profile_fields[] = {
    [F_NAME] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = {
            .name = "name",
            .required = 1,
            .required_message = "Enter a name"
        },
        .min_length = 2,
        .max_length = 100,
        .min_length_message = "Name is shorter than 2 characters",
        .max_length_message = "Name is longer than 100 characters"
    } },
    [F_NICKNAME] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = { .name = "nickname" },
        .min_length = 3,
        .max_length = 16,
        .regex = "\\A[a-z0-9_]+\\z",
        .regex_message = "Lowercase latin letters, digits and _"
    } },
    [F_BIO] = { .kind = FORM_FIELD_TEXT, .text = {
        .common = { .name = "bio" },
        .default_value = "",
        .max_length = 500,
        .max_length_message = "Bio is longer than 500 characters"
    } }
};

static const form_schema_t profile_schema = {
    .fields = profile_fields,
    .fields_count = sizeof profile_fields / sizeof *profile_fields,
    .clean = trim,
    .length = utf8_strlen
};

void profile_put(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_NAME] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "name")
        },
        [F_NICKNAME] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "nickname")
        },
        [F_BIO] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "bio")
        }
    };

    form_t* form = form_create(&profile_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    const char* name     = form_cleaned_data(form, F_NAME)->data.text;
    const char* nickname = form_cleaned_data(form, F_NICKNAME)->kind == FORM_VALUE_EMPTY
                           ? NULL
                           : form_cleaned_data(form, F_NICKNAME)->data.text;
    const char* bio      = form_cleaned_data(form, F_BIO)->data.text;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "name", json_create_string(name));
    if (nickname != NULL)
        json_object_set(object, "nickname", json_create_string(nickname));
    json_object_set(object, "bio", json_create_string(bio));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## Integer: transfer amount

A required amount of at least 100 and divisible by 100 — the bound belongs to the field, divisibility to a validator with a context. An optional note id defaults to zero.

**Request**

```bash
curl http://example.com/transfer \
    -X POST \
    -d "amount=1500&note=42"
```

**Handler**

```c
// handlers/forms/transfer.c
#include "http.h"
#include "form.h"
#include "errors.h"

static int64_t step = 100;

static int divisible_by(const form_value_t* value, void* context) {
    int64_t n = *(int64_t*)context;
    return value->kind == FORM_VALUE_INTEGER && n != 0 &&
           value->data.integer % n == 0;
}

static const form_validator_t amount_checks[] = {
    {
        .check = divisible_by,
        .context = &step,
        .message = "The amount must be divisible by 100"
    }
};

enum { F_AMOUNT, F_NOTE };

static const form_field_spec_t transfer_fields[] = {
    [F_AMOUNT] = { .kind = FORM_FIELD_INTEGER, .integer = {
        .common = {
            .name = "amount",
            .required = 1,
            .required_message = "Enter an amount",
            .invalid_message = "Enter an integer",
            .validators = amount_checks,
            .validators_count = 1
        },
        .has_min_value = 1,
        .min_value = 100,
        .min_value_message = "The minimum amount is 100"
    } },
    [F_NOTE] = { .kind = FORM_FIELD_INTEGER, .integer = {
        .common = { .name = "note" },
        .has_default = 1,
        .default_value = 0,
        .has_min_value = 1,
        .min_value = 0,
        .min_value_message = "The note id must be non-negative"
    } }
};

static const form_schema_t transfer_schema = {
    .fields = transfer_fields,
    .fields_count = sizeof transfer_fields / sizeof *transfer_fields
};

void transfer_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_AMOUNT] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "amount")
        },
        [F_NOTE] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "note")
        }
    };

    form_t* form = form_create(&transfer_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    int64_t amount = form_cleaned_data(form, F_AMOUNT)->data.integer;
    int64_t note   = form_cleaned_data(form, F_NOTE)->data.integer;

    /* ... transfer: transaction, database insert ... */

    form_free(form);
    ctx->response->send_data(ctx->response, "Transfer accepted");
}
```

## Decimal: parcel weight

A required weight from 0.05 to 30 kilograms and an optional insurance rate in percent (zero by default):

```c
// handlers/forms/parcel.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"

enum { F_WEIGHT, F_INSURANCE_RATE };

static const form_field_spec_t parcel_fields[] = {
    [F_WEIGHT] = { .kind = FORM_FIELD_DECIMAL, .decimal = {
        .common = {
            .name = "weight",
            .required = 1,
            .required_message = "Enter a weight",
            .invalid_message = "Enter a number"
        },
        .has_min_value = 1,
        .min_value = 0.05L,
        .has_max_value = 1,
        .max_value = 30.0L,
        .min_value_message = "Weight is under 0.05 kg",
        .max_value_message = "Weight is over 30 kg"
    } },
    [F_INSURANCE_RATE] = { .kind = FORM_FIELD_DECIMAL, .decimal = {
        .common = {
            .name = "insurance_rate",
            .invalid_message = "Enter a number"
        },
        .has_default = 1,
        .default_value = 0.0L
    } }
};

static const form_schema_t parcel_schema = {
    .fields = parcel_fields,
    .fields_count = sizeof parcel_fields / sizeof *parcel_fields
};

void parcel_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_WEIGHT] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "weight")
        },
        [F_INSURANCE_RATE] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "insurance_rate")
        }
    };

    form_t* form = form_create(&parcel_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    long double weight = form_cleaned_data(form, F_WEIGHT)->data.decimal;
    long double rate   = form_cleaned_data(form, F_INSURANCE_RATE)->data.decimal;

    json_doc_t* doc = json_root_create_object();
    json_object_set(json_root(doc), "weight", json_create_number(weight));
    json_object_set(json_root(doc), "insurance_rate", json_create_number(rate));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## Booleans: notification settings

Two optional flags with default values — empty input is not an error:

```c
// handlers/forms/notifications.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"

enum { F_NOTIFY, F_WEEKLY_DIGEST };

static const form_field_spec_t settings_fields[] = {
    [F_NOTIFY] = { .kind = FORM_FIELD_BOOLEAN, .boolean = {
        .common = {
            .name = "notify",
            .invalid_message = "Expected true or false"
        },
        .has_default = 1,
        .default_value = 1
    } },
    [F_WEEKLY_DIGEST] = { .kind = FORM_FIELD_BOOLEAN, .boolean = {
        .common = {
            .name = "weekly_digest",
            .invalid_message = "Expected true or false"
        },
        .has_default = 1,
        .default_value = 0
    } }
};

static const form_schema_t settings_schema = {
    .fields = settings_fields,
    .fields_count = sizeof settings_fields / sizeof *settings_fields
};

void settings_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_NOTIFY] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "notify")
        },
        [F_WEEKLY_DIGEST] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "weekly_digest")
        }
    };

    form_t* form = form_create(&settings_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    int notify  = form_cleaned_data(form, F_NOTIFY)->data.boolean;
    int digest  = form_cleaned_data(form, F_WEEKLY_DIGEST)->data.boolean;

    json_doc_t* doc = json_root_create_object();
    json_object_set(json_root(doc), "notify", json_create_bool(notify));
    json_object_set(json_root(doc), "weekly_digest", json_create_bool(digest));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## Date and time: booking

A required date, an end time, and a start time defaulting to 9:00. The start must precede the end — the relation is checked by `schema.validate`:

**Request**

```bash
curl http://example.com/booking \
    -X POST \
    -d "date=2026-10-01&starts_at=10:30&ends_at=12:00"
```

**Handler**

```c
// handlers/forms/booking.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"

enum { F_DATE, F_STARTS_AT, F_ENDS_AT };

static int seconds(const form_time_t* time) {
    return time->hour * 3600 + time->minute * 60 + time->second;
}

static void check_interval(form_t* form, void* context) {
    (void)context;
    const form_value_t* start = form_field_value(form, F_STARTS_AT);
    const form_value_t* end   = form_field_value(form, F_ENDS_AT);
    if (start == NULL || end == NULL) return;

    if (seconds(&start->data.time) >= seconds(&end->data.time))
        form_add_error(form, F_ENDS_AT, FORM_INVALID,
                       "End time precedes the start");
}

static const form_field_spec_t booking_fields[] = {
    [F_DATE] = { .kind = FORM_FIELD_DATE, .date = {
        .common = {
            .name = "date",
            .required = 1,
            .required_message = "Enter a date",
            .invalid_message = "Date as YYYY-MM-DD"
        }
    } },
    [F_STARTS_AT] = { .kind = FORM_FIELD_TIME, .time = {
        .common = {
            .name = "starts_at",
            .invalid_message = "Time as HH:MM"
        },
        .has_default = 1,
        .default_value = { .hour = 9 }
    } },
    [F_ENDS_AT] = { .kind = FORM_FIELD_TIME, .time = {
        .common = {
            .name = "ends_at",
            .required = 1,
            .required_message = "Enter the end time",
            .invalid_message = "Time as HH:MM"
        }
    } }
};

static const form_schema_t booking_schema = {
    .fields = booking_fields,
    .fields_count = sizeof booking_fields / sizeof *booking_fields,
    .validate = check_interval
};

void booking_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t inputs[] = {
        [F_DATE] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "date")
        },
        [F_STARTS_AT] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "starts_at")
        },
        [F_ENDS_AT] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = request->get_payloadf(request, "ends_at")
        }
    };

    form_t* form = form_create(&booking_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    form_date_t date  = form_cleaned_data(form, F_DATE)->data.date;
    form_time_t start = form_cleaned_data(form, F_STARTS_AT)->data.time;
    form_time_t end   = form_cleaned_data(form, F_ENDS_AT)->data.time;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    char buffer[32];

    snprintf(buffer, sizeof buffer, "%04d-%02d-%02d",
             date.year, date.month, date.day);
    json_object_set(object, "date", json_create_string(buffer));
    snprintf(buffer, sizeof buffer, "%02d:%02d", start.hour, start.minute);
    json_object_set(object, "starts_at", json_create_string(buffer));
    snprintf(buffer, sizeof buffer, "%02d:%02d", end.hour, end.minute);
    json_object_set(object, "ends_at", json_create_string(buffer));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## A moment with an offset: publishing

`FORM_FIELD_DATETIME` accepts `2026-10-01T12:00` with an optional `Z` or `+03:00` offset; the offset lands in `offset_minutes`:

```c
// handlers/forms/publish.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"
#include <stdio.h>
#include <stdlib.h>

static const form_field_spec_t publish_fields[] = {
    { .kind = FORM_FIELD_DATETIME, .datetime = {
        .common = {
            .name = "published_at",
            .required = 1,
            .required_message = "Enter the publication moment",
            .invalid_message =
                "Format YYYY-MM-DDTHH:MM[:SS] with an optional offset"
        }
    } }
};

static const form_schema_t publish_schema = {
    .fields = publish_fields,
    .fields_count = 1
};

void publish_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t input = {
        .kind = FORM_INPUT_TEXT,
        .data.text = request->get_payloadf(request, "published_at")
    };

    form_t* form = form_create(&publish_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    form_datetime_t moment = form_cleaned_data(form, 0)->data.datetime;

    char buffer[64];
    int written = snprintf(buffer, sizeof buffer,
                           "%04d-%02d-%02dT%02d:%02d:%02d",
                           moment.date.year, moment.date.month, moment.date.day,
                           moment.time.hour, moment.time.minute,
                           moment.time.second);

    if (moment.has_offset)
        snprintf(buffer + written, sizeof buffer - (size_t)written,
                 "%+03d:%02d", moment.offset_minutes / 60,
                 abs(moment.offset_minutes) % 60);

    json_doc_t* doc = json_root_create_object();
    json_object_set(json_root(doc), "published_at",
                    json_create_string(buffer));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## UUID: token activation

The field accepts the dashed form and 32 hexadecimal digits without dashes; after validation an array of 16 bytes is available:

```c
// handlers/forms/activation.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"
#include <stdio.h>

static const form_field_spec_t activation_fields[] = {
    { .kind = FORM_FIELD_UUID, .uuid = {
        .common = {
            .name = "token",
            .required = 1,
            .required_message = "Pass a token",
            .invalid_message = "Invalid token"
        }
    } }
};

static const form_schema_t activation_schema = {
    .fields = activation_fields,
    .fields_count = 1
};

void activation_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t input = {
        .kind = FORM_INPUT_TEXT,
        .data.text = request->get_payloadf(request, "token")
    };

    form_t* form = form_create(&activation_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    form_uuid_t uuid = form_cleaned_data(form, 0)->data.uuid;

    char hex[33];
    for (int i = 0; i < 16; i++)
        snprintf(hex + i * 2, 3, "%02x", uuid.bytes[i]);

    /* ... look the record up by uuid, activate it ... */

    json_doc_t* doc = json_root_create_object();
    json_object_set(json_root(doc), "activated", json_create_string(hex));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## JSON: webhook settings

The field checks the syntax and hands over the parsed document; from there the usual JSON API applies:

**Request**

```bash
curl http://example.com/settings/webhook \
    -X POST \
    -H "Content-Type: application/json" \
    -d '{"url":"https://example.com/hook","events":["push","ping"],"retries":3}'
```

**Handler**

```c
// handlers/forms/webhook.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"

static const form_field_spec_t webhook_fields[] = {
    { .kind = FORM_FIELD_JSON, .json = {
        .common = {
            .name = "config",
            .required = 1,
            .required_message = "Pass the settings",
            .invalid_message = "Invalid JSON"
        }
    } }
};

static const form_schema_t webhook_schema = {
    .fields = webhook_fields,
    .fields_count = 1
};

void webhook_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t input = {
        .kind = FORM_INPUT_TEXT,
        .data.text = request->get_payload(request)
    };

    form_t* form = form_create(&webhook_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    /* The document belongs to the form and lives until form_free(). */
    json_doc_t* config = form_cleaned_data(form, 0)->data.object;
    json_token_t* root = json_root(config);

    json_token_t* url    = json_object_get(root, "url");
    json_token_t* events = json_object_get(root, "events");

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);

    if (url != NULL && json_is_string(url))
        json_object_set(object, "url", json_create_string(json_string(url)));
    if (events != NULL && json_is_array(events))
        json_object_set(object, "events_count",
                        json_create_number(json_array_size(events)));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## Choosing values: product filter

A single category choice and a multiple tag choice. The tag list arrives as a JSON array — items are copied with `strdup` into the form's ownership:

**Request**

```bash
curl http://example.com/catalog/filter \
    -X POST \
    -H "Content-Type: application/json" \
    -d '{"category":"shirts","tags":["new","sale"]}'
```

**Handler**

```c
// handlers/forms/filter.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"
#include <stdlib.h>
#include <string.h>

static const char* const categories[] = { "shirts", "pants", "shoes" };
static const char* const tags[] = { "new", "sale", "local" };

enum { F_CATEGORY, F_TAGS };

static const form_field_spec_t filter_fields[] = {
    [F_CATEGORY] = { .kind = FORM_FIELD_CHOICE, .choice = {
        .common = {
            .name = "category",
            .required = 1,
            .required_message = "Choose a category"
        },
        .choices = categories,
        .choices_count = 3,
        .invalid_choice_message = "Category not found"
    } },
    [F_TAGS] = { .kind = FORM_FIELD_MULTIPLE_CHOICE, .multiple_choice = {
        .common = { .name = "tags" },
        .choices = tags,
        .choices_count = 3,
        .invalid_choice_message = "Tag not found"
    } }
};

static const form_schema_t filter_schema = {
    .fields = filter_fields,
    .fields_count = sizeof filter_fields / sizeof *filter_fields
};

void filter_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    json_doc_t* body = request->get_payload_json(request);
    if (body == NULL) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    json_token_t* root = json_root(body);
    json_token_t* category = json_object_get(root, "category");
    json_token_t* tags_json = json_object_get(root, "tags");

    /* The list array and every string in it move into the form's ownership. */
    size_t tags_count = tags_json != NULL && json_is_array(tags_json)
                        ? (size_t)json_array_size(tags_json) : 0;
    char** items = calloc(tags_count + 1, sizeof *items);
    if (items == NULL) {
        json_free(body);
        ctx->response->send_default(ctx->response, 500);
        return;
    }
    for (size_t i = 0; i < tags_count; i++) {
        json_token_t* tag = json_array_get(tags_json, (int)i);
        items[i] = tag != NULL && json_is_string(tag)
                   ? strdup(json_string(tag)) : strdup("");
    }

    form_input_t inputs[] = {
        [F_CATEGORY] = {
            .kind = FORM_INPUT_TEXT,
            .data.text = category != NULL && json_is_string(category)
                         ? strdup(json_string(category)) : NULL
        },
        [F_TAGS] = {
            .kind = FORM_INPUT_TEXT_LIST,
            .data.text_list = { items, tags_count }
        }
    };

    json_free(body);   /* the strings are already copied by strdup */

    form_t* form = form_create(&filter_schema, inputs);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    const char* category_name = form_cleaned_data(form, F_CATEGORY)->data.text;
    const form_value_text_list_t* selected =
        &form_cleaned_data(form, F_TAGS)->data.text_list;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "category", json_create_string(category_name));

    json_token_t* list = json_create_array();
    for (size_t i = 0; i < selected->count; i++)
        json_array_append(list, json_create_string(selected->items[i]));
    json_object_set(object, "tags", list);

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

An optional empty tag list is valid — the form returns `count == 0`.

## Uploaded file: avatar

A multipart form with a file: the form checks the size, the name length and the empty file, then the file goes to storage:

**Request**

```bash
curl http://example.com/avatar \
    -X POST \
    -F "avatar=@photo.png"
```

**Handler**

```c
// handlers/forms/avatar.c
#include "http.h"
#include "form.h"
#include "storage.h"
#include "errors.h"
#include <stdlib.h>

static const form_field_spec_t avatar_fields[] = {
    { .kind = FORM_FIELD_FILE, .file = {
        .common = {
            .name = "avatar",
            .required = 1,
            .required_message = "Choose a file"
        },
        .max_size = 2 * 1024 * 1024,
        .max_filename_length = 100,
        .max_size_message = "File is larger than 2 MB",
        .empty_file_message = "File is empty"
    } }
};

static const form_schema_t avatar_schema = {
    .fields = avatar_fields,
    .fields_count = 1
};

void avatar_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    file_content_t* uploaded = malloc(sizeof *uploaded);
    if (uploaded == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }
    *uploaded = request->get_payload_filef(request, "avatar");

    form_input_t input = {
        .kind = FORM_INPUT_OBJECT,
        .data.object = uploaded,
        .destroy = free
    };

    form_t* form = form_create(&avatar_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    file_content_t* avatar = form_cleaned_data(form, 0)->data.object;

    /* The descriptor belongs to the request — save before it ends. */
    if (!storage_file_content_put("avatars", avatar, "%s", avatar->filename)) {
        form_free(form);
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    form_free(form);
    ctx->response->send_data(ctx->response, "Avatar saved");
}
```

## Storage path: picking a template

A GET request: the template name arrives as a query parameter, is checked by a `FORM_FIELD_FILEPATH` field against a storage directory, and the template is then rendered:

**Request**

```bash
curl "http://example.com/page?template=about"
```

**Handler**

```c
// handlers/forms/page.c
#include "http.h"
#include "form.h"
#include "query.h"
#include "view.h"
#include "errors.h"
#include <stdlib.h>

static const form_field_spec_t page_fields[] = {
    { .kind = FORM_FIELD_FILEPATH, .filepath = {
        .common = {
            .name = "template",
            .required = 1,
            .required_message = "Specify a template"
        },
        .storage = "views",
        .directory = "pages",
        .allow_files = 1,
        .invalid_choice_message = "Template not found"
    } }
};

static const form_schema_t page_schema = {
    .fields = page_fields,
    .fields_count = 1
};

void page_get(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    /* query_param_char returns a borrowed string — copy it for the form. */
    int ok = 0;
    const char* raw = query_param_char(request->query_, "template", &ok);

    form_input_t input = {
        .kind = FORM_INPUT_TEXT,
        .data.text = ok && raw != NULL ? strdup(raw) : NULL
    };

    form_t* form = form_create(&page_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    const char* name = form_cleaned_data(form, 0)->data.text;

    /* The name is an argument, not part of the format: the path is validated. */
    char* html = render(NULL, "views", "pages/%s.tpl", name);
    form_free(form);

    if (html == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    ctx->response->add_header(ctx->response, "Content-Type", "text/html");
    ctx->response->send_data(ctx->response, html);
    free(html);
}
```

## Arbitrary object: a PDF with a signature check

`FORM_FIELD_OBJECT` validates the content with validators — here the size and the `%PDF-` signature, more reliable than the file extension:

```c
// handlers/forms/contract.c
#include "http.h"
#include "form.h"
#include "errors.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int size_ok(const form_value_t* value, void* context) {
    (void)context;
    if (value->kind != FORM_VALUE_OBJECT) return 0;

    const file_content_t* file = value->data.object;
    return file->ok && file->size > 0 && file->size <= 5 * 1024 * 1024;
}

static int pdf_signature_ok(const form_value_t* value, void* context) {
    (void)context;
    if (value->kind != FORM_VALUE_OBJECT) return 0;

    const file_content_t* file = value->data.object;
    if (!file->ok || file->size < 5) return 0;

    char signature[5];
    return pread(file->fd, signature, sizeof signature, file->offset) == 5 &&
           memcmp(signature, "%PDF-", sizeof signature) == 0;
}

static const form_validator_t contract_checks[] = {
    {
        .check = size_ok,
        .message = "File is empty or larger than 5 MB"
    },
    {
        .check = pdf_signature_ok,
        .message = "A PDF is expected"
    }
};

static const form_field_spec_t contract_fields[] = {
    { .kind = FORM_FIELD_OBJECT, .object = {
        .common = {
            .name = "document",
            .required = 1,
            .required_message = "Attach a file",
            .validators = contract_checks,
            .validators_count = 2
        }
    } }
};

static const form_schema_t contract_schema = {
    .fields = contract_fields,
    .fields_count = 1
};

void contract_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    file_content_t* received = malloc(sizeof *received);
    if (received == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }
    *received = request->get_payload_filef(request, "document");

    form_input_t input = {
        .kind = FORM_INPUT_OBJECT,
        .data.object = received,
        .destroy = free
    };

    form_t* form = form_create(&contract_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    file_content_t* document = form_cleaned_data(form, 0)->data.object;

    /* ... store the contract, answer the client ... */
    (void)document;

    form_free(form);
    ctx->response->send_data(ctx->response, "Contract accepted");
}
```

## Custom parser: a HEX color

`FORM_FIELD_CUSTOM` parses `#RRGGBB` into a structure; the parser creates the object with `malloc` and hands it to the form with `destroy = free`:

**Request**

```bash
curl http://example.com/settings/theme \
    -X POST \
    -d "accent=%232563eb"   # #2563eb
```

**Handler**

```c
// handlers/forms/theme.c
#include "http.h"
#include "form.h"
#include "json.h"
#include "errors.h"
#include <stdlib.h>

typedef struct {
    int r;
    int g;
    int b;
} rgb_t;

static int hex_digit(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static int parse_hex_color(const form_input_t* input, form_value_t* value,
                           void* context) {
    (void)context;
    if (input->kind != FORM_INPUT_TEXT || input->data.text == NULL)
        return 0;

    const char* text = input->data.text;
    if (text[0] == '#') text++;
    if (strlen(text) != 6) return 0;

    rgb_t* color = malloc(sizeof *color);
    if (color == NULL) return 0;

    const int components[] = { 0, 2, 4 };
    int parsed[3];
    for (int i = 0; i < 3; i++) {
        int high = hex_digit(text[components[i]]);
        int low  = hex_digit(text[components[i] + 1]);
        if (high < 0 || low < 0) {
            free(color);
            return 0;
        }
        parsed[i] = high * 16 + low;
    }

    color->r = parsed[0];
    color->g = parsed[1];
    color->b = parsed[2];

    *value = (form_value_t){
        .kind = FORM_VALUE_OBJECT,
        .data.object = color,
        .destroy = free
    };
    return 1;
}

static const form_field_spec_t theme_fields[] = {
    { .kind = FORM_FIELD_CUSTOM, .custom = {
        .common = {
            .name = "accent",
            .required = 1,
            .required_message = "Enter the accent color",
            .invalid_message = "Color as #RRGGBB"
        },
        .parse = parse_hex_color
    } }
};

static const form_schema_t theme_schema = {
    .fields = theme_fields,
    .fields_count = 1
};

void theme_post(httpctx_t* ctx) {
    httprequest_t* request = ctx->request;

    form_input_t input = {
        .kind = FORM_INPUT_TEXT,
        .data.text = request->get_payloadf(request, "accent")
    };

    form_t* form = form_create(&theme_schema, &input);
    if (form == NULL) {
        ctx->response->send_default(ctx->response, 500);
        return;
    }

    if (!form_is_valid(form)) {
        send_form_errors(ctx, form);
        form_free(form);
        return;
    }

    /* The object belongs to the form and is released in form_free(). */
    const rgb_t* accent = form_cleaned_data(form, 0)->data.object;

    json_doc_t* doc = json_root_create_object();
    json_token_t* object = json_root(doc);
    json_object_set(object, "r", json_create_number(accent->r));
    json_object_set(object, "g", json_create_number(accent->g));
    json_object_set(object, "b", json_create_number(accent->b));

    ctx->response->send_json(ctx->response, doc);

    json_free(doc);
    form_free(form);
}
```

## What is next

- All field types and rules — in the [Input validation](/en/forms) section.
- Raw `get_payload*` methods — in [Receiving data from the client](/en/payload).
- Sending JSON responses — in [JSON examples](/en/examples-json).
