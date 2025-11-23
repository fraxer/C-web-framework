#include "statement_registry.h"
#include "log.h"

/**
 * Application-specific prepared statement: user data retrieval
 */
static prepare_stmt_t* __pstmt_user_get(void);

prepare_stmt_t* __pstmt_user_get(void) {
    prepare_stmt_t* stmt = pstmt_create();
    if (stmt == NULL) return NULL;

    array_t* params = array_create();
    if (params == NULL) {
        pstmt_free(stmt);
        return NULL;
    }

    mparams_fill_array(params,
        mfield_def_int(id),
        mfield_def_text(email),
        mparam_text(table, "user"),
        mfield_def_array(fields, array_create_strings("id", "name", "email"))
    )

    stmt->name = str_create("user_get");
    stmt->query = str_create("SELECT @list__fields FROM @table WHERE email = :email AND id = :id LIMIT 1");
    stmt->params = params;

    return stmt;
}

/* ============= INITIALIZATION FUNCTION ============= */

int prepare_statements_init(void) {
    /* Register all application prepared statements */

    /* Register user_get prepared statement */
    if (!pstmt_registry_register(__pstmt_user_get)) {
        log_error("prepare_statements_init: failed to register __pstmt_user_get\n");
        return 0;
    }

    /* Add more prepared statements here as needed:
     * if (!pstmt_registry_register(pstmt_user_create)) {
     *     log_error("prepare_statements_init: failed to register pstmt_user_create\n");
     *     return 0;
     * }
     */

    return 1;
}