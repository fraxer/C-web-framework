---
outline: deep
description: Примеры работы с форматом json
---

# Пример работы с JSON

```c
void payload_json(httpctx_t* ctx) {
    const char* json_string = "{\"long\":4000000123000000,\"name\":\"alex\",\"number\":165.230000,\"bool\":true,\"array\":[\"string\",145,{\"array\":[]}]}";

    json_doc_t* document = json_init();
    if (!document) {
        ctx->response->send_data(ctx->response, "json init error");
        return;
    }

    if (json_parse(document, json_string) < 0) {
        ctx->response->send_data(ctx->response, "json parse error");
        return;
    }

    json_token_t* object = json_root(document);
    if (!json_is_object(object)) {
        ctx->response->send_data(ctx->response, "is not object");
        return;
    }

    json_object_set(object, "mykey", json_create_null(document));

    json_token_t* array = json_create_array(document);
    json_object_set(object, "prop", array);

    json_array_append(array, json_create_bool(document, 1));
    json_array_append(array, json_create_string(document, "Hello"));
    json_array_append_to(array, 0, json_create_llong(document, 123));
    json_array_append_to(array, 2, json_create_llong(document, -4096));
    json_array_append_to(array, 30, json_create_double(document, -409.6));

    json_array_erase(array, 2, 2);
    json_array_erase(array, 1, 1);

    json_array_prepend(array, json_create_string(document, "Prepended string"));
    json_token_t* token_double = json_array_get(array, 2);

    json_token_set_double(token_double, 152834534563.132);

    json_token_t* obj = json_create_object(document);
    json_array_append(array, obj);

    json_object_set(obj, "key1", json_create_double(document, 432.9640545));
    json_object_set(obj, "key2", json_create_llong(document, 432765656659640545));
    json_object_set(obj, "key3", json_create_bool(document, 0));
    json_object_set(obj, "key4", json_create_null(document));
    json_object_set(obj, "key5", json_create_object(document));
    json_object_set(obj, "key6", json_create_array(document));

    json_token_t* array2 = json_object_get(obj, "key6");
    if (array2) {
        json_array_prepend(array2, json_create_string(document, "String in array"));
    }

    json_object_remove(obj, "key5");

    for (jsonit_t it = json_init_it(object); !json_end_it(&it); it = json_next_it(&it)) {
        if (strcmp(json_it_key(&it), "number") == 0) {
            const json_token_t* token = json_it_value(&it);

            if (json_is_double(token)) {
                double bl = json_double(token);
            }
        }
    }

    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, json_stringify(document));

    json_free(document);
}
```
