#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "log.h"
#include "email_validator.h"

#define MAX_EMAIL_LENGTH 254
#define MAX_LOCAL_PART_LENGTH 64
#define MAX_DOMAIN_LENGTH 255

static int __is_local_part_char(char c);
static int __is_domain_char(char c);
static int __is_valid_tld(const char* domain);
static int __validate_local_part(const char* local_part);
static int __validate_domain(const char* domain);

/**
 * @brief Checks if a character is allowed in the local part of an email address
 * as per RFC 5322.
 *
 * @param c The character to check
 * @return 1 if the character is allowed, false otherwise
 */
int __is_local_part_char(char c) {
    return (isalnum(c) ||
            c == '.' || c == '!' || c == '#' ||
            c == '$' || c == '%' || c == '&' ||
            c == '\'' || c == '*' || c == '+' ||
            c == '-' || c == '/' || c == '=' ||
            c == '?' || c == '^' || c == '_' ||
            c == '`' || c == '{' || c == '|' ||
            c == '}' || c == '~');
}

/**
 * @brief Checks if a character is allowed in the domain part of an email address
 * as per RFC 1035.
 *
 * @param c The character to check
 * @return 1 if the character is allowed, false otherwise
 */
int __is_domain_char(char c) {
    return (isalnum(c) || c == '-' || c == '.');
}

/**
 * @brief Checks if the TLD of the given domain is valid.
 *
 * This function will return 0 if the TLD is invalid, and 1 if it is valid.
 *
 * A valid TLD must be at least 2 characters and contain only letters.
 *
 * @param domain The domain to check
 * @return 1 if the TLD is valid, 0 otherwise
 */
int __is_valid_tld(const char* domain) {
    const char* dot = strrchr(domain, '.');
    if (dot == NULL) return 0;

    const char* tld = dot + 1;

    // TLD must be at least 2 characters
    if (strlen(tld) < 2) return 0;

    // Check if TLD contains only letters
    while (*tld) {
        if (!isalpha(*tld)) return 0;
        tld++;
    }

    return 1;
}

/**
 * @brief Checks if the given local part of an email address is valid.
 *
 * This function will return 0 if the local part is invalid, and 1 if it is valid.
 *
 * A valid local part must be between 1 and MAX_LOCAL_PART_LENGTH characters long
 * and contain only valid characters. It must not start or end with a dot, and
 * there must be no consecutive dots.
 *
 * @param local_part The local part to check
 * @return 1 if the local part is valid, 0 otherwise
 */
int __validate_local_part(const char* local_part) {
    const size_t length = strlen(local_part);
    if (length == 0) {
        log_error("Local part cannot be empty\n");
        return 0;
    }

    if (length > MAX_LOCAL_PART_LENGTH) {
        log_error("Local part is too long\n");
        return 0;
    }

    // Cannot start or end with dot
    if (local_part[0] == '.' || local_part[length - 1] == '.') {
        log_error("Local part cannot start or end with dot\n");
        return 0;
    }

    // Check for consecutive dots
    char prev = '\0';
    for (size_t i = 0; i < length; i++) {
        char current = local_part[i];

        // Check for consecutive dots
        if (current == '.' && prev == '.') {
            log_error("Local part cannot contain consecutive dots\n");
            return 0;
        }

        // Check for valid characters
        if (!__is_local_part_char(current)) {
            log_error("Local part contains invalid characters\n");
            return 0;
        }

        prev = current;
    }

    return 1;
}

/**
 * @brief Validates the domain part of an email address.
 *
 * This function checks if the given domain is valid according to several rules,
 * including length constraints, character validity, and structural requirements.
 * 
 * The domain must not be empty, exceed the maximum length, or start/end with a
 * hyphen or dot. It also cannot contain consecutive dots or hyphens, must have 
 * at least one dot, and must have a valid top-level domain (TLD).
 *
 * @param domain The domain to validate
 * @return 1 if the domain is valid, 0 otherwise
 */
int __validate_domain(const char* domain) {
    size_t length = strlen(domain);
    if (length == 0) {
        log_error("Domain cannot be empty\n");
        return 0;
    }
    if (length > MAX_DOMAIN_LENGTH) {
        log_error("Domain is too long\n");
        return 0;
    }

    // Cannot start or end with hyphen or dot
    if (domain[0] == '-' || domain[0] == '.' ||
        domain[length - 1] == '-' || domain[length - 1] == '.') {
        log_error("Domain cannot start or end with hyphen or dot\n");
        return 0;
    }

    // Check for consecutive dots or hyphens
    char prev = '\0';
    int has_dot = 0;
    for (size_t i = 0; i < length; i++) {
        char ch = domain[i];

        // Track if we've seen a dot
        if (ch == '.')
            has_dot = 1;

        // Check for consecutive special characters
        if ((ch == '.' || ch == '-') &&
            (prev == '.' || prev == '-')) {
            log_error("Domain cannot contain consecutive dots or hyphens\n");
            return 0;
        }

        // Check for valid characters
        if (!__is_domain_char(ch)) {
            log_error("Domain contains invalid characters\n");
            return 0;
        }

        prev = ch;
    }
    
    // Must have at least one dot
    if (!has_dot) {
        log_error("Domain must contain at least one dot\n");
        return 0;
    }

    // Check for valid TLD
    if (!__is_valid_tld(domain)) {
        log_error("Invalid top-level domain\n");
        return 0;
    }

    return 1;
}

/**
 * @brief Validates the given email address.
 *
 * This function checks if the provided email address is valid according to
 * several criteria. It verifies that the email is not empty and does not exceed
 * the maximum allowable length. The email must contain exactly one '@' symbol,
 * and the local part and domain are extracted based on this symbol. The lengths
 * of the local part and domain are checked to ensure they are within the allowed
 * limits. The local part is validated for allowed characters and structure, and
 * the domain is validated for valid characters, structure, and a valid top-level
 * domain (TLD). If any of these checks fail, an appropriate error message is logged,
 * and the function returns 0. If all checks pass, the function returns 1.
 *
 * @param email The email address to validate
 * @return 1 if the email is valid, 0 otherwise
 */
int validate_email(const char* email) {
    if (email == NULL || *email == '\0') {
        log_error("Email cannot be empty\n");
        return 0;
    }

    const size_t length = strlen(email);
    if (length > MAX_EMAIL_LENGTH) {
        log_error("Email address is too long\n");
        return 0;
    }

    // Find @ symbol
    const char* at_pos = strchr(email, '@');
    if (!at_pos) {
        log_error("Email must contain @ symbol\n");
        return 0;
    }

    // Check for multiple @ symbols
    if (strchr(at_pos + 1, '@')) {
        log_error("Email cannot contain multiple @ symbols\n");
        return 0;
    }

    char local_part[65] = {0};  // 64 + null terminator
    char domain[256] = {0};     // 255 + null terminator

    // Split email into local part and domain
    const size_t local_part_length = at_pos - email;
    const size_t domain_length = length - local_part_length - 1;

    if (local_part_length >= sizeof(local_part) ||
        domain_length >= sizeof(domain)) {
        log_error("Email parts too long\n");
        return 0;
    }

    strncpy(local_part, email, local_part_length);
    local_part[local_part_length] = '\0';

    strncpy(domain, at_pos + 1, domain_length);
    domain[domain_length] = '\0';

    // Validate local part
    if (!__validate_local_part(local_part)) {
        log_error("Invalid local part\n");
        return 0;
    }

    // Validate domain
    if (!__validate_domain(domain)) {
        log_error("Invalid domain\n");
        return 0;
    }

    return 1;
}
