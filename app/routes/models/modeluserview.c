#include "http.h"
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

    int param_ok = 0;
    const int user_id = query_param_int(ctx->request, "id", &param_ok);
    if (!param_ok) {
        ctx->response->send_default(ctx->response, 400);
        return;
    }

    array_t* params = array_create();
    if (params == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        return;
    }

    mparams_fill_array(params,
        mparam_int(id, user_id)
    )

    userview_t* user = userview_get(params);
    array_free(params);

    if (user == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "user not found");
        return;
    }

    char* data = model_stringify(user, NULL);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        model_free(user);
        return;
    }
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, data);
    // ctx->response->send_model(ctx->response, user, display_fields("id", "email", "name"));

    free(data);
    model_free(user);
}

void userviewlist(httpctx_t* ctx) {
    array_t* users = userview_list();
    if (users == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "users not found");
        return;
    }

    char* data = userview_list_stringify(users);
    if (data == NULL) {
        ctx->response->status_code = 500;
        ctx->response->send_data(ctx->response, "error");
        array_free(users);
        return;
    }
    ctx->response->add_header(ctx->response, "Content-Type", "application/json");
    ctx->response->send_data(ctx->response, data);

    free(data);
    array_free(users);
}

char* userview_list_stringify(array_t* users) {
    if (users == NULL) return NULL;

    json_doc_t* doc = json_root_create_array();
    if (!doc) return NULL;

    char* data = NULL;

    json_token_t* json_array_users = json_root(doc);
    for (size_t i = 0; i < array_size(users); i++) {
        userview_t* user = (&users->elements[i])->_pointer;
        if (user == NULL) return NULL;

        array_t* user_roles_params = array_create();
        if (user_roles_params == NULL)
            return NULL;

        mparams_fill_array(user_roles_params,
            mparam_int(user_id, model_int(&user->field.id))
        )
        array_t* user_roles = roleview_list(user_roles_params);
        array_free(user_roles_params);

        json_token_t* json_object_user = model_to_json(user, NULL);

        if (user_roles != NULL) {
            json_token_t* json_array_roles = json_create_array();
            for (size_t j = 0; j < array_size(user_roles); j++) {
                roleview_t* role = (&user_roles->elements[j])->_pointer;
                if (role == NULL) return NULL;

                array_t* role_permissions_params = array_create();
                if (role_permissions_params == NULL)
                    return NULL;

                mparams_fill_array(role_permissions_params,
                    mparam_int(role_id, model_int(&role->field.id))
                )
                array_t* role_permissions = permissionview_list(role_permissions_params);
                array_free(role_permissions_params);

                json_token_t* json_object_role = model_to_json(role, NULL);

                if (role_permissions != NULL) {
                    json_token_t* json_array_permissions = json_create_array();
                    for (size_t k = 0; k < array_size(role_permissions); k++) {
                        permissionview_t* permission = (&role_permissions->elements[k])->_pointer;
                        if (permission == NULL) return NULL;

                        json_token_t* json_object_permission = model_to_json(permission, NULL);
                        json_array_append(json_array_permissions, json_object_permission);
                    }

                    array_free(role_permissions);

                    json_object_set(json_object_role, "permissions", json_array_permissions);
                }

                json_array_append(json_array_roles, json_object_role);
            }

            array_free(user_roles);

            json_object_set(json_object_user, "roles", json_array_roles);
        }
        
        json_array_append(json_array_users, json_object_user);
    }

    data = json_stringify_detach(doc);

    json_free(doc);

    return data;
}
