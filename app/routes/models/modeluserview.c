#include "http1.h"
#include "httpmiddlewares.h"
#include "userview.h"
#include "roleview.h"
#include "permissionview.h"
#include "str.h"

char* userview_list_stringify(array_t* users);

void userviewget(httpctx_t* ctx) {
    middleware(
        middleware_http_query_param_required(ctx, args_str("id"))
    )

    const char* quser_id = ctx->request->query(ctx->request, "id");

    // const char* quser_id = query(ctx->request, "id");
    // if (str_is_int(quser_id)) {
    //     const int _user_id = str_to_int(quser_id);
    // }
    // if (str_is_long(quser_id)) {
    //     const long long _user_id = str_to_long(quser_id);
    // }
    // if (str_is_double(quser_id)) {
    //     const double _user_id = str_to_double(quser_id);
    // }
    // if (str_is_exp(quser_id)) {
    //     const int _user_id = str_to_expint(quser_id);
    //     const int _user_id = str_to_expdouble(quser_id);
    // }

    const int userid = atoi(quser_id);

    userview_t* user = userview_get(&(userview_get_params_t){
        mfield_int(id, userid)
    });
    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "user not found");
        // user_free(user);
        return;
    }

    char* data = model_stringify(user);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        // user_free(user);
        return;
    }
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    free(data);
    // user_free(user);
}

void userviewlist(httpctx_t* ctx) {
    // middleware(
    //     middleware_http_query_param_required(ctx, args_str("id"))
    // )

    array_t* users = userview_list();
    if (users == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "users not found");
        // user_free(user);
        return;
    }

    char* data = userview_list_stringify(users);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->data(ctx->response, "error");
        // user_free(user);
        return;
    }
    ctx->response->header_add(ctx->response, "Content-Type", "application/json");
    ctx->response->data(ctx->response, data);

    free(data);
    array_free(users);
}

char* userview_list_stringify(array_t* users) {
    if (users == NULL) return NULL;

    jsondoc_t* doc = json_init();
    if (!doc) return NULL;

    char* data = NULL;

    jsontok_t* json_array_users = json_create_array(doc);
    for (size_t i = 0; i < array_size(users); i++) {
        userview_t* user = (&users->elements[i])->_pointer;
        if (user == NULL) return NULL;

        array_t* user_roles = roleview_list(&(roleview_list_params_t){
            mfield_int(user_id, model_int(&user->field.id))
        });

        jsontok_t* json_array_roles = json_create_array(doc);
        for (size_t j = 0; j < array_size(user_roles); j++) {
            roleview_t* role = (&user_roles->elements[j])->_pointer;
            if (role == NULL) return NULL;

            array_t* role_permissions = permissionview_list(&(permissionview_list_params_t){
                mfield_int(role_id, model_int(&role->field.id))
            });

            jsontok_t* json_array_permissions = json_create_array(doc);
            for (size_t k = 0; k < array_size(role_permissions); k++) {
                permissionview_t* permission = (&role_permissions->elements[k])->_pointer;
                if (permission == NULL) return NULL;

                jsontok_t* json_object_permission = model_to_json(permission, doc);
                json_array_append(json_array_permissions, json_object_permission);
            }

            jsontok_t* json_object_role = model_to_json(role, doc);
            json_object_set(json_object_role, "permissions", json_array_permissions);
            json_array_append(json_array_roles, json_object_role);
        }

        jsontok_t* json_object_user = model_to_json(user, doc);
        json_object_set(json_object_user, "roles", json_array_roles);
        json_array_append(json_array_users, json_object_user);
    }

    data = json_stringify_detach(doc);

    json_free(doc);

    return data;
}

void usertest(httpctx_t* ctx) {

    // str_t str;
    // str_init(&str);

    // str_prependc(&str, 'a');
    // str_appendc(&str, 'b');
    // str_prependc(&str, 'c');
    // str_appendc(&str, 'd');
    // str_insertc(&str, 'e', 2);
    // str_appendc(&str, 'f');
    // str_appendc(&str, 'g');

    // str_clear(&str);


    const char* s = "";
    str_t* str = str_create(s, strlen(s));

    str_append(str, "a", 1);
    str_prepend(str, "b", 1);
    str_append(str, "cd", 2);
    str_insert(str, "e", 1, 2);

    str_free(str);

    ctx->response->data(ctx->response, "done");
}