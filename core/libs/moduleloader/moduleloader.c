#define _GNU_SOURCE
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "log.h"
#include "file.h"
#include "database.h"
#include "json.h"
#include "redirect.h"
#include "route.h"
#include "routeloader.h"
#include "domain.h"
#include "server.h"
#include "websockets.h"
#include "openssl.h"
#include "mimetype.h"
#include "storagefs.h"
#include "storages3.h"
#include "viewstore.h"
#include "threadhandler.h"
#include "threadworker.h"
#include "broadcast.h"
#include "connection_queue.h"
#include "middlewarelist.h"
#ifdef MySQL_FOUND
    #include "mysql.h"
#endif
#ifdef PostgreSQL_FOUND
    #include "postgresql.h"
#endif
#ifdef Redis_FOUND
    #include "redis.h"
#endif

#include "moduleloader.h"

static atomic_bool __module_loader_wait_signal = ATOMIC_VAR_INIT(0);

static int __module_loader_init_modules(appconfig_t* config, jsondoc_t* document);
static int __module_loader_servers_load(appconfig_t* config, const jsontok_t* servers);
static domain_t* __module_loader_domains_load(const jsontok_t* token_array);
static int __module_loader_databases_load(appconfig_t* config, const jsontok_t* databases);
static int __module_loader_storages_load(appconfig_t* config, const jsontok_t* storages);
static int __module_loader_mimetype_load(appconfig_t* config, const jsontok_t* mimetypes);
static int __module_loader_viewstore_load(appconfig_t* config);

static int __module_loader_http_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object, route_t** route);
static int __module_loader_set_http_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object);
static void __module_loader_pass_memory_sharedlib(routeloader_lib_t*, const char*);
static int __module_loader_http_redirects_load(const jsontok_t* token_object, redirect_t** redirect);
static int __module_loader_middlewares_load(const jsontok_t* token_object, middleware_item_t** middleware_item);
static void __module_loader_websockets_default_load(void(**fn)(void*), routeloader_lib_t** first_lib, const jsontok_t* token_array);
static int __module_loader_websockets_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object, route_t** route);
static int __module_loader_set_websockets_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object);
static openssl_t* __module_loader_tls_load(const jsontok_t* token_object);
static int __module_loader_check_unique_domainport(server_t* first_server);
static void* __module_loader_storage_fs_load(const jsontok_t* token_object, const char* storage_name);
static void* __module_loader_storage_s3_load(const jsontok_t* token_object, const char* storage_name);
static const char* __module_loader_storage_field(const char* storage_name, const jsontok_t* token_object, const char* key);
static int __module_loader_thread_workers_load(void);
static int __module_loader_thread_handlers_load(void);
static void __module_loader_on_shutdown_cb(void);

int module_loader_init(appconfig_t* config) {
    int result = 0;

    appconfig_set_after_run_threads_cb(module_loader_signal_unlock);

    jsondoc_t* document = NULL;
    if (!module_loader_load_json_config(config->path, &document))
        goto failed;
    if (!__module_loader_init_modules(config, document))
        goto failed;

    result = 1;

    failed:

    json_free(document);

    return result;
}

int module_loader_config_correct(const char* path) {
    int result = 0;
    appconfig_t* config = NULL;
    jsondoc_t* document = NULL;
    if (!module_loader_load_json_config(path, &document))
        goto failed;

    config = appconfig_create(path);
    if (!module_loader_config_load(config, document))
        goto failed;

    result = 1;

    failed:

    appconfig_free(config);

    json_free(document);

    return result;
}

int module_loader_load_json_config(const char* path, jsondoc_t** document) {
    if (path == NULL) return 0;

    file_t file = file_open(path, O_RDONLY);
    if (!file.ok) {
        log_error("module_loader_load_json_config: file_open error\n");
        return 0;
    }

    char* data = file.content(&file);

    file.close(&file);
   
    if (data == NULL) {
        log_error("module_loader_load_json_config: file_read error\n");
        return 0;
    }

    int result = 0;

    *document = json_init();
    if (*document == NULL) {
        log_error("module_loader_load_json_config: json_init error\n");
        goto failed;
    }
    if (!json_parse(*document, data)) {
        log_error("module_loader_load_json_config: json_parse error\n");
        goto failed;
    }
    if (!(*document)->ok) {
        log_error("module_loader_load_json_config: json document invalid\n");
        goto failed;
    }
    if (!json_is_object(json_root(*document))) {
        log_error("module_loader_load_json_config: json document must be object\n");
        goto failed;
    }

    result = 1;

    failed:

    free(data);

    return result;
}

int __module_loader_init_modules(appconfig_t* config, jsondoc_t* document) {
    int result = 0;

    if (!connection_queue_init()) {
        log_error("__module_loader_init_modules: connection_queue_init error\n");
        goto failed;
    }
    if (!module_loader_config_load(config, document))
        goto failed;
    if (!__module_loader_thread_workers_load())
        goto failed;
    if (!__module_loader_thread_handlers_load())
        goto failed;

    result = 1;

    failed:

    if (!result)
        appconfig_free(appconfig());

    return result;
}

int module_loader_config_load(appconfig_t* config, jsondoc_t* document) {
    env_t* env = &config->env;
    const jsontok_t* root = json_root(document);

    const jsontok_t* token_migrations = json_object_get(root, "migrations");
    if (token_migrations == NULL) {
        env->migrations.source_directory = malloc(sizeof(char) * 1);
        if (env->migrations.source_directory == NULL) {
            log_error("module_loader_config_load: memory alloc error\n");
            return 0;
        }

        strcpy(env->migrations.source_directory, "");
    }
    else {
        if (!json_is_object(token_migrations)) {
            log_error("module_loader_config_load: migrations must be object\n");
            return 0;
        }

        const jsontok_t* token_source_directory = json_object_get(token_migrations, "source_directory");
        if (token_source_directory == NULL) {
            env->migrations.source_directory = malloc(sizeof(char) * 1);
            if (env->migrations.source_directory == NULL) {
                log_error("module_loader_config_load: memory alloc error\n");
                return 0;
            }

            strcpy(env->migrations.source_directory, "");
        }
        else {
            if (!json_is_string(token_source_directory)) {
                log_error("module_loader_config_load: source_directory must be string\n");
                return 0;
            }

            env->migrations.source_directory = malloc(sizeof(char) * (token_source_directory->size + 1));
            if (env->migrations.source_directory == NULL) {
                log_error("module_loader_config_load: memory alloc error\n");
                return 0;
            }

            strcpy(env->migrations.source_directory, json_string(token_source_directory));
        }
    }


    const jsontok_t* token_main = json_object_get(root, "main");
    if (token_main == NULL) {
        log_error("module_loader_config_load: main not found\n");
        return 0;
    }
    if (!json_is_object(token_main)) {
        log_error("module_loader_config_load: main must be object\n");
        return 0;
    }


    const jsontok_t* token_reload = json_object_get(token_main, "reload");
    if (token_reload == NULL) {
        log_error("module_loader_config_load: reload not found\n");
        return 0;
    }
    if (!json_is_string(token_reload)) {
        log_error("module_loader_config_load: reload must be string\n");
        return 0;
    }
    if (strcmp(json_string(token_reload), "hard") == 0) {
        env->main.reload = APPCONFIG_RELOAD_HARD;
    }
    else if (strcmp(json_string(token_reload), "soft") == 0) {
        env->main.reload = APPCONFIG_RELOAD_SOFT;
    }
    else {
        log_error("module_loader_config_load: reload must be contain soft or hard\n");
        return 0;
    }


    const jsontok_t* token_workers = json_object_get(token_main, "workers");
    if (token_workers == NULL) {
        log_error("module_loader_config_load: workers not found\n");
        return 0;
    }
    if (!json_is_int(token_workers)) {
        log_error("module_loader_config_load: workers must be int\n");
        return 0;
    }
    if (json_int(token_workers) < 1) {
        log_error("module_loader_config_load: workers must be >= 1\n");
        return 0;
    }
    env->main.workers = json_int(token_workers);


    const jsontok_t* token_threads = json_object_get(token_main, "threads");
    if (token_threads == NULL) {
        log_error("module_loader_config_load: threads not found\n");
        return 0;
    }
    if (!json_is_int(token_threads)) {
        log_error("module_loader_config_load: threads must be int\n");
        return 0;
    }
    if (json_int(token_threads) < 1) {
        log_error("module_loader_config_load: threads must be >= 1\n");
        return 0;
    }
    env->main.threads = json_int(token_threads);


    const jsontok_t* token_buffer_size = json_object_get(token_main, "buffer_size");
    if (token_buffer_size == NULL) {
        log_error("module_loader_config_load: buffer_size not found\n");
        return 0;
    }
    if (!json_is_int(token_buffer_size)) {
        log_error("module_loader_config_load: buffer_size must be int\n");
        return 0;
    }
    if (json_int(token_buffer_size) < 1) {
        log_error("module_loader_config_load: buffer_size must be >= 1\n");
        return 0;
    }
    env->main.buffer_size = json_int(token_buffer_size);


    const jsontok_t* token_client_max_body_size = json_object_get(token_main, "client_max_body_size");
    if (token_client_max_body_size == NULL) {
        log_error("module_loader_config_load: client_max_body_size not found\n");
        return 0;
    }
    if (!json_is_int(token_client_max_body_size)) {
        log_error("module_loader_config_load: client_max_body_size must be int\n");
        return 0;
    }
    if (json_int(token_client_max_body_size) < 1) {
        log_error("module_loader_config_load: client_max_body_size must be >= 1\n");
        return 0;
    }
    env->main.client_max_body_size = json_int(token_client_max_body_size);


    const jsontok_t* token_tmp = json_object_get(token_main, "tmp");
    if (token_tmp == NULL) {
        log_error("module_loader_config_load: tmp not found\n");
        return 0;
    }
    if (!json_is_string(token_tmp)) {
        log_error("module_loader_config_load: tmp must be string\n");
        return 0;
    }
    env->main.tmp = malloc(sizeof(char) * (token_tmp->size + 1));
    if (env->main.tmp == NULL) {
        log_error("module_loader_config_load: memory alloc error for tmp\n");
        return 0;
    }
    strcpy(env->main.tmp, json_string(token_tmp));
    const size_t tmp_length = token_tmp->size;
    if (env->main.tmp[tmp_length - 1] == '/') {
        log_error("module_loader_config_load: remove last slash from main.tmp\n");
        return 0;
    }


    const jsontok_t* token_gzip = json_object_get(token_main, "gzip");
    if (token_gzip == NULL) {
        log_error("module_loader_config_load: gzip not found\n");
        return 0;
    }
    if (!json_is_array(token_gzip)) {
        log_error("module_loader_config_load: gzip must be array\n");
        return 0;
    }

    env_gzip_str_t* last_gzip_item = NULL;
    for (jsonit_t it = json_init_it(token_gzip); !json_end_it(&it); it = json_next_it(&it)) {
        const jsontok_t* token_mimetype = json_it_value(&it);
        if (!json_is_string(token_mimetype)) {
            log_error("module_loader_config_load: gzip must be array of strings\n");
            return 0;
        }
        if (token_mimetype->size == 0) {
            log_error("module_loader_config_load: gzip item must be not empty\n");
            return 0;
        }

        env_gzip_str_t* str = malloc(sizeof * str);
        if (str == NULL) {
            log_error("module_loader_config_load: memory alloc error for gzip item\n");
            return 0;
        }
        str->next = NULL;
        str->mimetype = malloc(sizeof(char) * (token_mimetype->size + 1));
        if (str->mimetype == NULL) {
            log_error("module_loader_config_load: memory alloc error for gzip item value\n");
            free(str);
            return 0;
        }
        strcpy(str->mimetype, json_string(token_mimetype));

        if (env->main.gzip == NULL)
            env->main.gzip = str;

        if (last_gzip_item != NULL)
            last_gzip_item->next = str;

        last_gzip_item = str;
    }


    if (!__module_loader_servers_load(config, json_object_get(root, "servers")))
        return 0;
    if (!__module_loader_databases_load(config, json_object_get(root, "databases")))
        return 0;
    if (!__module_loader_storages_load(config, json_object_get(root, "storages")))
        return 0;
    if (!__module_loader_mimetype_load(config, json_object_get(root, "mimetypes")))
        return 0;
    if (!__module_loader_viewstore_load(config))
        return 0;


    const jsontok_t* token_mail = json_object_get(root, "mail");
    if (token_mail == NULL) {
        env->mail.dkim_private = malloc(sizeof(char) * 1);
        if (env->mail.dkim_private == NULL) {
            log_error("module_loader_config_load: memory alloc error mail.dkim_private\n");
            return 0;
        }
        strcpy(env->mail.dkim_private, "");


        env->mail.dkim_selector = malloc(sizeof(char) * 1);
        if (env->mail.dkim_selector == NULL) {
            log_error("module_loader_config_load: memory alloc error mail.dkim_selector\n");
            return 0;
        }
        strcpy(env->mail.dkim_selector, "");


        env->mail.host = malloc(sizeof(char) * 1);
        if (env->mail.host == NULL) {
            log_error("module_loader_config_load: memory alloc error mail.host\n");
            return 0;
        }
        strcpy(env->mail.host, "");
    }
    else {
        const jsontok_t* token_dkim_private = json_object_get(token_mail, "dkim_private");
        if (token_dkim_private == NULL) {
            env->mail.dkim_private = malloc(sizeof(char) * 1);
            if (env->mail.dkim_private == NULL) {
                log_error("module_loader_config_load: memory alloc error mail.dkim_private\n");
                return 0;
            }
            strcpy(env->mail.dkim_private, "");
        }
        else {
            if (!json_is_string(token_dkim_private)) {
                log_error("module_loader_config_load: mail.dkim_private must be string\n");
                return 0;
            }
            if (token_dkim_private->size == 0) {
                log_error("module_loader_config_load: mail.dkim_private must be not empty\n");
                return 0;
            }

            file_t file = file_open(json_string(token_dkim_private), O_RDONLY);
            if (!file.ok) {
                log_error("module_loader_config_load: open mail.dkim_private error\n");
                return 0;
            }

            env->mail.dkim_private = file.content(&file);
            file.close(&file);

            if (env->mail.dkim_private == NULL) {
                log_error("module_loader_config_load: read mail.dkim_private error\n");
                return 0;
            }
        }

        const jsontok_t* token_dkim_selector = json_object_get(token_mail, "dkim_selector");
        if (token_dkim_selector == NULL) {
            env->mail.dkim_selector = malloc(sizeof(char) * 1);
            if (env->mail.dkim_selector == NULL) {
                log_error("module_loader_config_load: memory alloc error mail.dkim_selector\n");
                return 0;
            }
            strcpy(env->mail.dkim_selector, "");
        }
        else {
            if (!json_is_string(token_dkim_selector)) {
                log_error("module_loader_config_load: mail.dkim_selector must be string\n");
                return 0;
            }
            if (token_dkim_selector->size == 0) {
                log_error("module_loader_config_load: mail.dkim_selector must be not empty\n");
                return 0;
            }

            env->mail.dkim_selector = malloc(sizeof(char) * (token_dkim_selector->size + 1));
            if (env->mail.dkim_selector == NULL) {
                log_error("module_loader_config_load: memory alloc error mail.dkim_selector\n");
                return 0;
            }
            strcpy(env->mail.dkim_selector, json_string(token_dkim_selector));
        }

        const jsontok_t* token_host = json_object_get(token_mail, "host");
        if (token_host == NULL) {
            env->mail.host = malloc(sizeof(char) * 1);
            if (env->mail.host == NULL) {
                log_error("module_loader_config_load: memory alloc error mail.host\n");
                return 0;
            }
            strcpy(env->mail.host, "");
        }
        else {
            if (!json_is_string(token_host)) {
                log_error("module_loader_config_load: mail.host must be string\n");
                return 0;
            }
            if (token_host->size == 0) {
                log_error("module_loader_config_load: mail.host must be not empty\n");
                return 0;
            }

            env->mail.host = malloc(sizeof(char) * (token_host->size + 1));
            if (env->mail.host == NULL) {
                log_error("module_loader_config_load: memory alloc error mail.host\n");
                return 0;
            }
            strcpy(env->mail.host, json_string(token_host));
        }
    }

    return 1;
}

int __module_loader_servers_load(appconfig_t* config, const jsontok_t* token_servers) {
    if (token_servers == NULL) {
        log_error("__module_loader_servers_load: servers not found\n");
        return 0;
    }
    if (!json_is_object(token_servers)) {
        log_error("__module_loader_servers_load: servers must be object\n");
        return 0;
    }

    int result = 0;
    server_t* first_server = NULL;
    server_t* last_server = NULL;
    routeloader_lib_t* first_lib = NULL;
    for (jsonit_t it_servers = json_init_it(token_servers); !json_end_it(&it_servers); json_next_it(&it_servers)) {
        enum required_fields { R_DOMAINS = 0, R_IP, R_PORT, R_ROOT, R_FIELDS_COUNT };
        char* str_required_fields[R_FIELDS_COUNT] = {"domains", "ip", "port", "root"};
        enum fields { DOMAINS = 0, IP, PORT, ROOT, INDEX, HTTP, WEBSOCKETS, DATABASE, OPENSSL, FIELDS_COUNT };

        int finded_fields[FIELDS_COUNT] = {0};

        server_t* server = server_create();
        if (server == NULL) {
            log_error("__module_loader_servers_load: can't create server\n");
            goto failed;
        }

        if (first_server == NULL)
            first_server = server;

        if (last_server != NULL)
            last_server->next = server;

        last_server = server;

        server->broadcast = broadcast_init();
        if (server->broadcast == NULL) {
            log_error("__module_loader_servers_load: can't create broadcast\n");
            goto failed;
        }

        const jsontok_t* token_server = json_it_value(&it_servers);
        if (!json_is_object(token_server)) {
            log_error("__module_loader_servers_load: server must be object\n");
            goto failed;
        }

        for (jsonit_t it_server = json_init_it(token_server); !json_end_it(&it_server); json_next_it(&it_server)) {
            const char* key = json_it_key(&it_server);
            const jsontok_t* token_value = json_it_value(&it_server);

            if (strcmp(key, "domains") == 0) {
                finded_fields[DOMAINS] = 1;

                if (!json_is_array(token_value)) {
                    log_error("__module_loader_servers_load: domains must be array\n");
                    goto failed;
                }

                server->domain = __module_loader_domains_load(token_value);
                if (server->domain == NULL) {
                    log_error("__module_loader_servers_load: can't load domains\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "ip") == 0) {
                finded_fields[IP] = 1;

                if (!json_is_string(token_value)) {
                    log_error("__module_loader_servers_load: ip must be string\n");
                    goto failed;
                }

                server->ip = inet_addr(json_string(token_value));
            }
            else if (strcmp(key, "port") == 0) {
                finded_fields[PORT] = 1;

                if (!json_is_int(token_value)) {
                    log_error("__module_loader_servers_load: port must be integer\n");
                    goto failed;
                }

                server->port = json_int(token_value);
            }
            else if (strcmp(key, "root") == 0) {
                finded_fields[ROOT] = 1;

                if (!json_is_string(token_value)) {
                    log_error("__module_loader_servers_load: root path must be string\n");
                    goto failed;
                }

                const char* value = json_string(token_value);
                size_t value_length = token_value->size;

                if (value[value_length - 1] == '/')
                    value_length--;

                server->root = malloc(value_length + 1);
                if (server->root == NULL) {
                    log_error("__module_loader_servers_load: can't alloc memory for root path\n");
                    goto failed;
                }

                strncpy(server->root, value, value_length);

                server->root[value_length] = 0;
                server->root_length = value_length;

                struct stat stat_obj;
                stat(server->root, &stat_obj);
                if (!S_ISDIR(stat_obj.st_mode)) {
                    log_error("__module_loader_servers_load: root directory not found\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "index") == 0) {
                finded_fields[INDEX] = 1;

                if (!json_is_string(token_value)) {
                    log_error("__module_loader_servers_load: index file must be string\n");
                    goto failed;
                }

                server->index = server_index_create(json_string(token_value));
                if (server->index == NULL) {
                    log_error("__module_loader_servers_load: can't alloc memory for index file\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "http") == 0) {
                finded_fields[HTTP] = 1;

                if (!json_is_object(token_value)) {
                    log_error("__module_loader_servers_load: http must be object\n");
                    goto failed;
                }

                if (!__module_loader_http_routes_load(&first_lib, json_object_get(token_value, "routes"), &server->http.route)) {
                    log_error("__module_loader_servers_load: can't load routes\n");
                    goto failed;
                }
                if (!__module_loader_http_redirects_load(json_object_get(token_value, "redirects"), &server->http.redirect)) {
                    log_error("__module_loader_servers_load: can't load redirects\n");
                    goto failed;
                }
                if (!__module_loader_middlewares_load(json_object_get(token_value, "middlewares"), &server->http.middleware)) {
                    log_error("__module_loader_servers_load: can't load middlewares\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "websockets") == 0) {
                finded_fields[WEBSOCKETS] = 1;

                if (!json_is_object(token_value)) {
                    log_error("__module_loader_servers_load: websockets must be object\n");
                    goto failed;
                }

                __module_loader_websockets_default_load(&server->websockets.default_handler, &first_lib, json_object_get(token_value, "default"));

                if (!__module_loader_websockets_routes_load(&first_lib, json_object_get(token_value, "routes"), &server->websockets.route)) {
                    log_error("__module_loader_servers_load: can't load routes\n");
                    goto failed;
                }
                if (!__module_loader_middlewares_load(json_object_get(token_value, "middlewares"), &server->websockets.middleware)) {
                    log_error("__module_loader_servers_load: can't load middlewares\n");
                    goto failed;
                }
            }
            else if (strcmp(key, "tls") == 0) {
                finded_fields[OPENSSL] = 1;

                if (!json_is_object(token_value)) {
                    log_error("__module_loader_servers_load: tls must be object\n");
                    goto failed;
                }

                server->openssl = __module_loader_tls_load(token_value);
                if (server->openssl == NULL) {
                    log_error("__module_loader_servers_load: can't load tls\n");
                    goto failed;
                }
            }
        }

        for (int i = 0; i < R_FIELDS_COUNT; i++) {
            if (finded_fields[i] == 0) {
                log_error("__module_loader_servers_load: Section %s not found in config\n", str_required_fields[i]);
                goto failed;
            }
        }

        if (finded_fields[INDEX] == 0) {
            server->index = server_index_create("index.html");
            if (server->index == NULL) {
                log_error("__module_loader_servers_load: can't create index file\n");
                goto failed;
            }
        }

        if (finded_fields[WEBSOCKETS] == 0)
            server->websockets.default_handler = (void(*)(void*))websockets_default_handler;

        if (!__module_loader_check_unique_domainport(first_server)) {
            log_error("__module_loader_servers_load: domains with ports must be unique\n");
            goto failed;
        }
    }

    if (first_server == NULL) {
        log_error("__module_loader_servers_load: section server is empty\n");
        goto failed;
    }

    config->server_chain = server_chain_create(first_server, first_lib);
    if (config->server_chain == NULL) {
        log_error("__module_loader_servers_load: can't create server chain\n");
        goto failed;
    }

    result = 1;

    failed:

    if (result == 0) {
        servers_free(first_server);
        routeloader_free(first_lib);
    }

    return result;
}

domain_t* __module_loader_domains_load(const jsontok_t* token_array) {
    domain_t* result = NULL;
    domain_t* first_domain = NULL;
    domain_t* last_domain = NULL;

    for (jsonit_t it = json_init_it(token_array); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_domain = json_it_value(&it);
        if (!json_is_string(token_domain)) {
            log_error("__module_loader_domains_load: domain must be string\n");
            goto failed;
        }

        domain_t* domain = domain_create(json_string(token_domain));
        if (domain == NULL) {
            log_error("__module_loader_domains_load: can't create domain\n");
            goto failed;
        }

        if (first_domain == NULL)
            first_domain = domain;

        if (last_domain != NULL)
            last_domain->next = domain;

        last_domain = domain;
    }

    result = first_domain;

    failed:

    if (result == NULL)
        domains_free(first_domain);

    return result;
}

int __module_loader_databases_load(appconfig_t* config, const jsontok_t* token_databases) {
    if (token_databases == NULL) return 1;
    if (!json_is_object(token_databases)) {
        log_error("__module_loader_databases_load: databases must be object\n");
        return 0;
    }

    config->databases = array_create();
    if (config->databases == NULL) {
        log_error("__module_loader_databases_load: can't create databases array\n");
        return 0;
    }

    int result = 0;
    for (jsonit_t it = json_init_it(token_databases); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) {
            log_error("__module_loader_databases_load: database driver must be array\n");
            goto failed;
        }
        if (json_array_size(token_array) == 0) {
            log_error("__module_loader_databases_load: database driver must be not empty\n");
            goto failed;
        }

        const char* driver = json_it_key(&it);
        db_t* database = NULL;

        #ifdef PostgreSQL_FOUND
        if (strcmp(driver, "postgresql") == 0)
            database = postgresql_load(driver, token_array);
        #endif

        #ifdef MySQL_FOUND
        if (strcmp(driver, "mysql") == 0)
            database = my_load(driver, token_array);
        #endif

        #ifdef Redis_FOUND
        if (strcmp(driver, "redis") == 0)
            database = redis_load(driver, token_array);
        #endif

        if (database == NULL) {
            log_error("__module_loader_databases_load: database driver <%s> not found\n", driver);
            continue;
        }

        array_push_back(config->databases, array_create_pointer(database, array_nocopy, db_free));
    }

    result = 1;

    failed:

    return result;
}

int __module_loader_storages_load(appconfig_t* config, const jsontok_t* token_storages) {
    if (token_storages == NULL) return 1;
    if (!json_is_object(token_storages)) {
        log_error("__module_loader_storages_load: storages must be object\n");
        return 0;
    }

    int result = 0;
    storage_t* last_storage = NULL;
    for (jsonit_t it = json_init_it(token_storages); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_object = json_it_value(&it);
        if (!json_is_object(token_object)) {
            log_error("__module_loader_storages_load: storage must be object\n");
            goto failed;
        }

        const char* storage_name = json_it_key(&it);
        if (strlen(storage_name) == 0) {
            log_error("__module_loader_storages_load: storage name must be not empty\n");
            goto failed;
        }

        jsontok_t* token_storage_type = json_object_get(token_object, "type");
        if (!json_is_string(token_storage_type)) {
            log_error("Field type must be string in storage %s\n", storage_name);
            goto failed;
        }

        storage_t* storage = NULL;
        const char* storage_type = json_string(token_storage_type);
        if (strcmp(storage_type, "filesystem") == 0)
            storage = __module_loader_storage_fs_load(token_object, storage_name);
        else if (strcmp(storage_type, "s3") == 0)
            storage = __module_loader_storage_s3_load(token_object, storage_name);
        else
            goto failed;

        if (config->storages == NULL)
            config->storages = storage;

        if (last_storage != NULL)
            last_storage->next = storage;

        last_storage = storage;
    }

    result = 1;

    failed:

    if (result == 0) {
        storages_free(config->storages);
        config->storages = NULL;
    }

    return result;
}

int __module_loader_mimetype_load(appconfig_t* config, const jsontok_t* token_mimetypes) {
    if (token_mimetypes == NULL) {
        log_error("__module_loader_mimetype_load: mimetypes not found\n");
        return 0;
    }
    if (!json_is_object(token_mimetypes)) {
        log_error("__module_loader_mimetype_load: mimetypes must be object\n");
        return 0;
    }
    if (json_object_size(token_mimetypes) == 0) {
        log_error("__module_loader_mimetype_load: mimetypes must be not empty\n");
        return 0;
    }

    int result = 0;
    size_t table_type_size = json_object_size(token_mimetypes);
    size_t table_ext_size = 0;

    for (jsonit_t it = json_init_it(token_mimetypes); !json_end_it(&it); json_next_it(&it)) {
        const jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) {
            log_error("__module_loader_mimetype_load: mimetype item must be array\n");
            return 0;
        }

        table_ext_size += json_array_size(token_array);
    }

    config->mimetype = mimetype_create(table_type_size, table_ext_size);
    if (config->mimetype == NULL) {
        log_error("__module_loader_mimetype_load: can't alloc mimetype memory, %d\n", errno);
        goto failed;
    }

    for (jsonit_t it_object = json_init_it(token_mimetypes); !json_end_it(&it_object); json_next_it(&it_object)) {
        const char* mimetype = json_it_key(&it_object);
        const jsontok_t* token_array = json_it_value(&it_object);
        if (!json_is_array(token_array)) {
            log_error("__module_loader_mimetype_load: mimetype item must be array\n");
            goto failed;
        }
        if (json_array_size(token_array) == 0) {
            log_error("__module_loader_mimetype_load: mimetype item must be not empty\n");
            goto failed;
        }

        for (jsonit_t it_array = json_init_it(token_array); !json_end_it(&it_array); json_next_it(&it_array)) {
            const int* index = json_it_key(&it_array);
            const jsontok_t* token_value = json_it_value(&it_array);
            if (!json_is_string(token_value)) {
                log_error("__module_loader_mimetype_load: mimetype item.value must be string\n");
                goto failed;
            }
            if (token_value->size == 0) {
                log_error("__module_loader_mimetype_load: mimetype item.value must be not empty\n");
                goto failed;
            }

            const char* extension = json_string(token_value);
            if (*index == 0)
                if (!mimetype_add(mimetype_get_table_type(config->mimetype), mimetype, extension))
                    goto failed;

            if (!mimetype_add(mimetype_get_table_ext(config->mimetype), extension, mimetype))
                goto failed;
        }
    }

    result = 1;

    failed:

    if (result == 0) {
        mimetype_destroy(config->mimetype);
        config->mimetype = NULL;
    }

    return result;
}

int __module_loader_viewstore_load(appconfig_t* config) {
    config->viewstore = viewstore_create();
    if (config->viewstore == NULL) {
        log_error("__module_loader_viewstore_load: can't create viewstore\n");
        return 0;
    }

    return 1;
}

int __module_loader_http_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object, route_t** route) {
    int result = 0;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return 1;
    if (!json_is_object(token_object)) {
        log_error("__module_loader_http_routes_load: http.route must be object\n");
        goto failed;
    }
    if (json_object_size(token_object) == 0) return 1;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* route_path = json_it_key(&it);
        if (strlen(route_path) == 0) {
            log_error("__module_loader_http_routes_load: route path is empty\n");
            goto failed;
        }

        route_t* rt = route_create(route_path);
        if (rt == NULL) {
            log_error("__module_loader_http_routes_load: failed to create route\n");
            goto failed;
        }

        if (first_route == NULL)
            first_route = rt;

        if (last_route != NULL)
            last_route->next = rt;

        last_route = rt;

        if (!__module_loader_set_http_route(first_lib, &last_lib, rt, json_it_value(&it))) {
            log_error("__module_loader_http_routes_load: failed to set http route\n");
            goto failed;
        }
    }

    result = 1;

    *route = first_route;

    failed:

    if (result == 0)
        routes_free(first_route);

    return result;
}

int __module_loader_set_http_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object) {
    if (token_object == NULL) {
        log_error("__module_loader_set_http_route: http.route item is empty\n");
        return 0;
    }
    if (!json_is_object(token_object)) {
        log_error("__module_loader_set_http_route: http.route item must be object\n");
        return 0;
    }
    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* method = json_it_key(&it);
        jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) {
            log_error("__module_loader_set_http_route: http.route item.value must be array\n");
            return 0;
        }
        if (json_array_size(token_array) != 2) {
            log_error("__module_loader_set_http_route: http.route item.value must be array with 2 elements\n");
            return 0;
        }

        const jsontok_t* token_string0 = json_array_get(token_array, 0);
        if (!json_is_string(token_string0)) {
            log_error("__module_loader_set_http_route: http.route item.value[0] must be string\n");
            return 0;
        }
        if (token_string0->size == 0) {
            log_error("__module_loader_set_http_route: http.route item.value[0] must be not empty string\n");
            return 0;
        }
        const jsontok_t* token_string1 = json_array_get(token_array, 1);
        if (!json_is_string(token_string1)) {
            log_error("__module_loader_set_http_route: http.route item.value[1] must be string\n");
            return 0;
        }
        if (token_string1->size == 0) {
            log_error("__module_loader_set_http_route: http.route item.value[1] must be not empty string\n");
            return 0;
        }

        const char* lib_file = json_string(token_string0);
        const char* lib_handler = json_string(token_string1);
        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib_t* routeloader_lib = routeloader_load_lib(lib_file);
            if (routeloader_lib == NULL) {
                log_error("__module_loader_set_http_route: failed to load lib %s\n", lib_file);
                return 0;
            }

            if (*first_lib == NULL)
                *first_lib = routeloader_lib;

            if (*last_lib != NULL)
                (*last_lib)->next = routeloader_lib;

            *last_lib = routeloader_lib;
        }

        void(*function)(void*);
        *(void**)(&function) = routeloader_get_handler(*first_lib, lib_file, lib_handler);
        if (function == NULL) {
            log_error("__module_loader_set_http_route: failed to get handler %s.%s\n", lib_file, lib_handler);
            return 0;
        }

        if (!route_set_http_handler(route, method, function)) {
            log_error("__module_loader_set_http_route: failed to set http handler %s.%s\n", lib_file, lib_handler);
            return 0;
        }

        __module_loader_pass_memory_sharedlib(*first_lib, lib_file);
    }

    return 1;
}

void __module_loader_pass_memory_sharedlib(routeloader_lib_t* first_lib, const char* lib_file) {
    void(*function)(void*);
    *(void**)(&function) = routeloader_get_handler_silent(first_lib, lib_file, "appconfig_set");

    if (function != NULL)
        function(appconfig());
}

int __module_loader_http_redirects_load(const jsontok_t* token_object, redirect_t** redirect) {
    int result = 0;
    redirect_t* first_redirect = NULL;
    redirect_t* last_redirect = NULL;

    if (token_object == NULL) return 1;
    if (!json_is_object(token_object)) {
        log_error("__module_loader_http_redirects_load: http.redirects must be object\n");
        goto failed;
    }
    if (json_object_size(token_object) == 0) return 1;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_value = json_it_value(&it);
        if (token_value == NULL) {
            log_error("__module_loader_http_redirects_load: http.redirects item.value is empty\n");
            goto failed;
        }
        if (!json_is_string(token_value)) {
            log_error("__module_loader_http_redirects_load: http.redirects item.value must be string\n");
            goto failed;
        }
        if (token_value->size == 0) {
            log_error("__module_loader_http_redirects_load: http.redirects item.value is empty\n");
            goto failed;
        }

        const char* redirect_path = json_it_key(&it);
        if (strlen(redirect_path) == 0) {
            log_error("__module_loader_http_redirects_load: http.redirects item.key is empty\n");
            goto failed;
        }

        const char* redirect_target = json_string(token_value);
        redirect_t* redirect = redirect_create(redirect_path, redirect_target);
        if (redirect == NULL) {
            log_error("__module_loader_http_redirects_load: failed to create redirect\n");
            goto failed;
        }

        if (first_redirect == NULL)
            first_redirect = redirect;

        if (last_redirect != NULL)
            last_redirect->next = redirect;

        last_redirect = redirect;
    }

    result = 1;

    *redirect = first_redirect;

    failed:

    if (result == 0)
        redirect_free(first_redirect);

    return result;
}

int __module_loader_middlewares_load(const jsontok_t* token_array, middleware_item_t** middleware_item) {
    int result = 0;
    middleware_item_t* first_middleware = NULL;
    middleware_item_t* last_middleware = NULL;

    if (token_array == NULL) return 1;
    if (!json_is_array(token_array)) {
        log_error("__module_loader_middlewares_load: http.middlewares must be array\n");
        goto failed;
    }
    if (json_array_size(token_array) == 0) return 1;

    for (jsonit_t it = json_init_it(token_array); !json_end_it(&it); json_next_it(&it)) {
        jsontok_t* token_value = json_it_value(&it);
        if (!json_is_string(token_value)) {
            log_error("__module_loader_middlewares_load: http.middlewares item.value must be string\n");
            goto failed;
        }
        if (token_value->size == 0) {
            log_error("__module_loader_middlewares_load: http.middlewares item.value is empty\n");
            goto failed;
        }

        const char* middleware_name = json_string(token_value);
        middleware_fn_p fn = middleware_by_name(middleware_name);
        if (fn == NULL) {
            log_error("__module_loader_middlewares_load: failed to find middleware %s\n", middleware_name);
            goto failed;
        }

        middleware_item_t* middleware_item = middleware_create(fn);
        if (middleware_item == NULL) {
            log_error("__module_loader_middlewares_load: failed to create middleware\n");
            goto failed;
        }

        if (first_middleware == NULL)
            first_middleware = middleware_item;

        if (last_middleware != NULL)
            last_middleware->next = middleware_item;

        last_middleware = middleware_item;
    }

    result = 1;

    *middleware_item = first_middleware;

    failed:

    if (result == 0)
        middlewares_free(first_middleware);

    return result;
}

void __module_loader_websockets_default_load(void(**fn)(void*), routeloader_lib_t** first_lib, const jsontok_t* token_array) {
    *fn = (void(*)(void*))websockets_default_handler;

    if (token_array == NULL) return;
    if (!json_is_array(token_array)) {
        log_error("__module_loader_websockets_default_load: websockets.default must be array\n");
        return;
    }
    if (json_array_size(token_array) != 2) {
        log_error("__module_loader_websockets_default_load: websockets.default must be array with 2 items\n");
        return;
    }

    const jsontok_t* token_string0 = json_array_get(token_array, 0);
    if (!json_is_string(token_string0)) {
        log_error("__module_loader_websockets_default_load: websockets.default item.value[0] must be string\n");
        return;
    }
    if (token_string0->size == 0) {
        log_error("__module_loader_websockets_default_load: websockets.default item.value[0] must be not empty string\n");
        return;
    }
    const jsontok_t* token_string1 = json_array_get(token_array, 1);
    if (!json_is_string(token_string1)) {
        log_error("__module_loader_websockets_default_load: websockets.default item.value[1] must be string\n");
        return;
    }
    if (token_string1->size == 0) {
        log_error("__module_loader_websockets_default_load: websockets.default item.value[1] must be not empty string\n");
        return;
    }

    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);
    const char* lib_file = json_string(token_string0);
    const char* lib_handler = json_string(token_string1);
    if (!routeloader_has_lib(*first_lib, lib_file)) {
        routeloader_lib_t* routeloader_lib = routeloader_load_lib(lib_file);
        if (routeloader_lib == NULL) {
            log_error("__module_loader_websockets_default_load: failed to load lib %s\n", lib_file);
            return;
        }

        if (*first_lib == NULL)
            *first_lib = routeloader_lib;

        if (last_lib != NULL)
            last_lib->next = routeloader_lib;

        last_lib = routeloader_lib;
    }

    *(void**)(&(*fn)) = routeloader_get_handler(*first_lib, lib_file, lib_handler);
}

int __module_loader_websockets_routes_load(routeloader_lib_t** first_lib, const jsontok_t* token_object, route_t** route) {
    int result = 0;
    route_t* first_route = NULL;
    route_t* last_route = NULL;
    routeloader_lib_t* last_lib = routeloader_get_last(*first_lib);

    if (token_object == NULL) return 1;
    if (!json_is_object(token_object)) {
        log_error("__module_loader_websockets_routes_load: websockets.routes must be object\n");
        goto failed;
    }
    if (token_object->size == 0) return 1;

    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* route_path = json_it_key(&it);
        if (strlen(route_path) == 0) {
            log_error("__module_loader_websockets_routes_load: websockets.route path is empty\n");
            goto failed;
        }

        route_t* rt = route_create(route_path);
        if (rt == NULL) {
            log_error("__module_loader_websockets_routes_load: failed to create route\n");
            goto failed;
        }

        if (first_route == NULL)
            first_route = rt;

        if (last_route != NULL)
            last_route->next = rt;

        last_route = rt;

        if (!__module_loader_set_websockets_route(first_lib, &last_lib, rt, json_it_value(&it))) {
            log_error("__module_loader_websockets_routes_load: failed to set websockets route\n");
            goto failed;
        }
    }

    result = 1;
    
    *route = first_route;

    failed:

    if (result == 0)
        routes_free(first_route);

    return result;
}

int __module_loader_set_websockets_route(routeloader_lib_t** first_lib, routeloader_lib_t** last_lib, route_t* route, const jsontok_t* token_object) {
    if (token_object == NULL) {
        log_error("__module_loader_set_websockets_route: websockets.route item is empty\n");
        return 0;
    }
    if (!json_is_object(token_object)) {
        log_error("__module_loader_set_websockets_route: websockets.route item must be object\n");
        return 0;
    }
    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* method = json_it_key(&it);
        if (strlen(method) == 0) {
            log_error("__module_loader_set_websockets_route: websockets.route item.key is empty\n");
            return 0;
        }

        jsontok_t* token_array = json_it_value(&it);
        if (!json_is_array(token_array)) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value must be array\n");
            return 0;
        }
        if (json_array_size(token_array) != 2) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value must be array with 2 elements\n");
            return 0;
        }

        const jsontok_t* token_string0 = json_array_get(token_array, 0);
        if (!json_is_string(token_string0)) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value[0] must be string\n");
            return 0;
        }
        if (token_string0->size == 0) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value[0] must be not empty string\n");
            return 0;
        }
        const jsontok_t* token_string1 = json_array_get(token_array, 1);
        if (!json_is_string(token_string1)) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value[1] must be string\n");
            return 0;
        }
        if (token_string1->size == 0) {
            log_error("__module_loader_set_websockets_route: websockets.route item.value[1] must be not empty string\n");
            return 0;
        }

        const char* lib_file = json_string(token_string0);
        const char* lib_handler = json_string(token_string1);
        routeloader_lib_t* routeloader_lib = NULL;
        if (!routeloader_has_lib(*first_lib, lib_file)) {
            routeloader_lib = routeloader_load_lib(lib_file);
            if (routeloader_lib == NULL) {
                log_error("__module_loader_set_websockets_route: failed to load lib %s\n", lib_file);
                return 0;
            }

            if (*first_lib == NULL)
                *first_lib = routeloader_lib;

            if (*last_lib != NULL)
                (*last_lib)->next = routeloader_lib;

            *last_lib = routeloader_lib;
        }

        void(*function)(void*);
        *(void**)(&function) = routeloader_get_handler(*first_lib, lib_file, lib_handler);

        if (function == NULL) {
            log_error("__module_loader_set_websockets_route: failed to get handler %s.%s\n", lib_file, lib_handler);
            return 0;
        }

        if (!route_set_websockets_handler(route, method, function)) {
            log_error("__module_loader_set_websockets_route: failed to set websockets handler\n");
            return 0;
        }

        __module_loader_pass_memory_sharedlib(*first_lib, lib_file);
    }

    return 1;
}

void module_loader_threads_pause(appconfig_t* config) {
    if (config == NULL) return;

    if (atomic_load(&config->threads_pause) == 0)
        return;

    appconfig_lock(config);

    atomic_fetch_add(&config->threads_stop_count, 1);

    const int threads_count = config->env.main.threads + config->env.main.workers;
    if (atomic_load(&config->threads_stop_count) == threads_count) {
        atomic_store(&config->shutdown, 1);
        module_loader_create_config_and_init();
        atomic_store(&config->threads_pause, 0);
        appconfig_unlock(config);
        return;
    }

    appconfig_unlock(config);

    while (atomic_load(&config->threads_pause))
        usleep(1000);
}

void module_loader_create_config_and_init(void) {
    appconfig_t* newconfig = appconfig_create(appconfig_path());
    if (newconfig == NULL) {
        log_error("appconfig_threads_pause: can't create new config\n");
        return;
    }

    appconfig_set(newconfig);
    module_loader_init(newconfig);
}

void __module_loader_on_shutdown_cb(void) {
    atomic_store(&appconfig()->shutdown, 1);
    module_loader_wakeup_all_threads();
    module_loader_signal_unlock();
}

void module_loader_signal_lock(void) {
    atomic_store(&__module_loader_wait_signal, 1);
}

int module_loader_signal_locked(void) {
    return atomic_load(&__module_loader_wait_signal);
}

void module_loader_signal_unlock(void) {
    atomic_store(&__module_loader_wait_signal, 0);
}

void module_loader_wakeup_all_threads(void) {
    thread_handlers_wakeup();
}

void* __module_loader_storage_fs_load(const jsontok_t* token_object, const char* storage_name) {
    void* result = NULL;

    const char* root = __module_loader_storage_field(storage_name, token_object, "root");
    if (root == NULL) goto failed;

    char root_path[PATH_MAX];
    strcpy(root_path, root);
    size_t root_path_length = strlen(root_path);
    while (root_path_length > 0) {
        root_path_length--;
        if (root_path[root_path_length] == '/')
            root_path[root_path_length] = 0;
        else
            break;
    }

    if (root_path_length == 0) {
        log_error("__module_loader_storage_fs_load: storage %s has empty path\n", storage_name);
        goto failed;
    }

    storagefs_t* storage = storage_create_fs(storage_name, root_path);
    if (storage == NULL) {
        log_error("__module_loader_storage_fs_load: failed to create storage %s\n", storage_name);
        goto failed;
    }

    result = storage;

    failed:

    return result;
}

void* __module_loader_storage_s3_load(const jsontok_t* token_object, const char* storage_name) {
    void* result = NULL;

    const char* access_id = __module_loader_storage_field(storage_name, token_object, "access_id");
    if (access_id == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty access_id\n");
        goto failed;
    }

    const char* access_secret = __module_loader_storage_field(storage_name, token_object, "access_secret");
    if (access_secret == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty access_secret\n");
        goto failed;
    }

    const char* protocol = __module_loader_storage_field(storage_name, token_object, "protocol");
    if (protocol == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty protocol\n");
        goto failed;
    }

    const char* host = __module_loader_storage_field(storage_name, token_object, "host");
    if (host == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty host\n");
        goto failed;
    }

    const char* port = __module_loader_storage_field(storage_name, token_object, "port");
    if (port == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty port\n");
        goto failed;
    }

    const char* bucket = __module_loader_storage_field(storage_name, token_object, "bucket");
    if (bucket == NULL) {
        log_error("__module_loader_storage_s3_load: storage %s has empty bucket\n");
        goto failed;
    }

    storages3_t* storage = storage_create_s3(storage_name, access_id, access_secret, protocol, host, port, bucket);
    if (storage == NULL) {
        log_error("__module_loader_storage_s3_load: failed to create storage %s\n", storage_name);
        goto failed;
    }

    result = storage;

    failed:

    return result;
}

const char* __module_loader_storage_field(const char* storage_name, const jsontok_t* token_object, const char* key) {
    jsontok_t* token_value = json_object_get(token_object, key);
    if (!json_is_string(token_value)) {
        log_error("__module_loader_storage_field: field %s must be string in storage %s\n", key, storage_name);
        return NULL;
    }

    const char* value = json_string(token_value);
    if (token_value->size == 0) {
        log_error("__module_loader_storage_field: field %s is empty in storage %s\n", key, storage_name);
        return NULL;
    }

    return value;
}

openssl_t* __module_loader_tls_load(const jsontok_t* token_object) {
    if (token_object == NULL) {
        log_error("__module_loader_tls_load: openssl not found\n");
        return NULL;
    }
    if (!json_is_object(token_object)) {
        log_error("__module_loader_tls_load: openssl must be object\n");
        return NULL;
    }

    openssl_t* result = NULL;
    openssl_t* openssl = openssl_create();
    if (openssl == NULL) {
        log_error("__module_loader_tls_load: failed to create openssl\n");
        return NULL;
    }

    enum fields { FULLCHAIN = 0, PRIVATE, CIPHERS, FIELDS_COUNT };
    char* finded_fields_str[FIELDS_COUNT] = {"fullchain", "private", "ciphers"};
    int finded_fields[FIELDS_COUNT] = {0};
    for (jsonit_t it = json_init_it(token_object); !json_end_it(&it); json_next_it(&it)) {
        const char* key = json_it_key(&it);
        if (strlen(key) == 0) {
            log_error("__module_loader_tls_load: tls key is empty\n");
            goto failed;
        }

        const jsontok_t* token_value = json_it_value(&it);
        if (token_value == NULL) {
            log_error("__module_loader_tls_load: tls value is empty\n");
            goto failed;
        }

        if (strcmp(key, "fullchain") == 0) {
            if (!json_is_string(token_value)) {
                log_error("__module_loader_tls_load: field fullchain must be string type\n");
                goto failed;
            }
            if (token_value->size == 0) {
                log_error("__module_loader_tls_load: field fullchain is empty\n");
                goto failed;
            }

            finded_fields[FULLCHAIN] = 1;

            openssl->fullchain = malloc(token_value->size + 1);
            if (openssl->fullchain == NULL) {
                log_error("__module_loader_tls_load: failed to allocate memory for fullchain\n");
                goto failed;
            }

            strcpy(openssl->fullchain, json_string(token_value));
        }
        else if (strcmp(key, "private") == 0) {
            if (!json_is_string(token_value)) {
                log_error("Openssl field private must be string type\n");
                goto failed;
            }
            if (token_value->size == 0) {
                log_error("__module_loader_tls_load: field private is empty\n");
                goto failed;
            }

            finded_fields[PRIVATE] = 1;

            openssl->private = malloc(token_value->size + 1);
            if (openssl->private == NULL) {
                log_error("__module_loader_tls_load: failed to allocate memory for private\n");
                goto failed;
            }

            strcpy(openssl->private, json_string(token_value));
        }
        else if (strcmp(key, "ciphers") == 0) {
            if (!json_is_string(token_value)) {
                log_error("Openssl field ciphers must be string type\n");
                goto failed;
            }
            if (token_value->size == 0) {
                log_error("__module_loader_tls_load: field ciphers is empty\n");
                goto failed;
            }

            finded_fields[CIPHERS] = 1;

            openssl->ciphers = malloc(token_value->size + 1);
            if (openssl->ciphers == NULL) {
                log_error("__module_loader_tls_load: failed to allocate memory for ciphers\n");
                goto failed;
            }

            strcpy(openssl->ciphers, json_string(token_value));
        }
    }

    for (int i = 0; i < FIELDS_COUNT; i++) {
        if (finded_fields[i] == 0) {
            log_error("__module_loader_tls_load: field %s not found in tls\n", finded_fields_str[i]);
            goto failed;
        }
    }

    if (!openssl_init(openssl))
        goto failed;

    result = openssl;

    failed:

    if (result == NULL)
        openssl_free(openssl);

    return result;
}

int __module_loader_check_unique_domainport(server_t* first_server) {
    for (server_t* current_server = first_server; current_server; current_server = current_server->next) {
        unsigned short int current_port = current_server->port;
        for (domain_t* current_domain = current_server->domain; current_domain; current_domain = current_domain->next) {

            for (server_t* server = first_server; server; server = server->next) {
                unsigned short int port = server->port;
                for (domain_t* domain = server->domain; domain; domain = domain->next) {
                    if (current_domain == domain) continue;

                    if (strcmp(current_domain->template, domain->template) == 0 && current_port == port) {
                        log_error("__module_loader_check_unique_domainport: domains with ports must be unique. %s %d\n", domain->template, port);
                        return 0;
                    }
                }
            }
        }
    }

    return 1;
}

int __module_loader_thread_workers_load(void) {
    const int count = env()->main.workers;
    if (count <= 0) {
        log_error("__module_loader_thread_workers_load: set the number of workers\n");
        return 0;
    }

    thread_worker_set_threads_pause_cb(module_loader_threads_pause);
    thread_worker_set_threads_shutdown_cb(__module_loader_on_shutdown_cb);

    return thread_worker_run(appconfig(), count);
}

int __module_loader_thread_handlers_load(void) {
    const int count = env()->main.threads;
    if (count <= 0) {
        log_error("__module_loader_thread_handlers_load: set the number of threads\n");
        return 0;
    }

    thread_handler_set_threads_pause_cb(module_loader_threads_pause);

    return thread_handler_run(appconfig(), count);
}
