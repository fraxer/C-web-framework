#include <openssl/evp.h>
#include <openssl/rand.h>

#include "log.h"
#include "appconfig.h"
#include "helpers.h"
#include "session.h"

#define SESSION_ID_SIZE 40
#define SESSION_ID_HEX_SIZE 81

/**
 * Generates a random session id and returns it as a hexadecimal string.
 *
 * The session id is a random 40 byte long string, represented as a 80 byte
 * long hexadecimal string. The string is null-terminated and allocated with
 * malloc. The caller is responsible for freeing the memory with free() when
 * it is no longer needed.
 *
 * @return A randomly generated session id as a hexadecimal string, or NULL
 * if an error occurred.
 */
char* session_create_id() {
    unsigned char id[SESSION_ID_SIZE];
    if (RAND_bytes(id, SESSION_ID_SIZE - 1) != 1) {
        log_error("session_create_id: create session id failed\n");
        return 0;
    }

    char* id_hex = malloc(SESSION_ID_HEX_SIZE);
    if (id_hex == NULL) return NULL;

    bytes_to_hex(id, SESSION_ID_SIZE, id_hex);

    return id_hex;
}

/**
 * Create a new session and return the session id.
 *
 * Before creating a new session, all expired sessions are removed.
 *
 * @param data The data to be stored in the session
 * @return The session id of the newly created session
 */
char* session_create(const char* data) {
    session_remove_expired();

    return appconfig()->sessionconfig.session->create(data);
}

/**
 * Destroys a session identified by the given session id.
 *
 * This function removes the session associated with the provided session id,
 * effectively invalidating it. After destruction, the session id can no longer
 * be used to retrieve or update session data.
 *
 * @param session_id The identifier of the session to be destroyed.
 * @return 1 if the session was successfully destroyed, 0 if the session
 * could not be found or an error occurred.
 */
int session_destroy(const char* session_id) {
    return appconfig()->sessionconfig.session->destroy(session_id);
}

/**
 * Updates the data associated with a session.
 *
 * This function updates the session data for the specified session id
 * with the provided data. If the session id is valid, the session data
 * is modified accordingly.
 *
 * @param session_id The identifier of the session to be updated.
 * @param data The new data to be stored in the session.
 * @return 1 if the session was successfully updated, 0 if the session
 * could not be found or an error occurred.
 */
int session_update(const char* session_id, const char* data) {
    return appconfig()->sessionconfig.session->update(session_id, data);
}

/**
 * Retrieves the data associated with a session.
 *
 * This function returns the session data associated with the specified session
 * id. If the session id is valid, the session data is returned as a string.
 * Otherwise, NULL is returned.
 *
 * @param session_id The identifier of the session to be retrieved.
 * @return The session data associated with the given session id, or NULL
 * if the session could not be found or an error occurred.
 */
char* session_get(const char* session_id) {
    return appconfig()->sessionconfig.session->get(session_id);
}

void session_remove_expired(void) {
    appconfig()->sessionconfig.session->remove_expired();
}

void sessionconfig_clear(sessionconfig_t* sessionconfig) {
    if (sessionconfig->session != NULL)
        free(sessionconfig->session);
    
    memset(sessionconfig, 0, sizeof(sessionconfig_t));
}
