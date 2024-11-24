#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define MIN_LENGTH 8
#define MAX_LENGTH 128
#define MIN_UPPERCASE 1
#define MIN_LOWERCASE 1
#define MIN_DIGITS 1
#define MIN_SPECIAL 1

static const char* COMMON_PATTERNS[] = {
    "123", "abc", "qwe", "111", "000", "pass",
    "admin", "letmein", "welcome", "password",
    NULL
};

static const char* BLACKLIST[] = {
    "qwerty",
    "qwertyuiop",
    "asdfgh",
    "asdfghjkl",
    "zxcvbnm",
    NULL
};

static int __is_special_char(char c);
static int __has_repeated_chars(const char* password, int threshold);
static void __str_reverse(char* str);
static int __has_keyboard_patterns(const char* password);

/**
 * Checks if the given character is a special character.
 * A special character is either a punctuation character
 * or a whitespace character (space or tab).
 *
 * @param c the character to check
 * @return 1 if the character is a special character, false otherwise
 */
int __is_special_char(char c) {
    return (ispunct(c) || c == ' ' || c == '\t');
}

/**
 * Checks if the given password contains a sequence of the same character
 * repeated the number of times specified by the threshold.
 *
 * @param password The password to check.
 * @param threshold The number of times the same character must appear in
 *                  sequence for the function to return 1.
 * @return 1 if a repeated sequence is found, 0 otherwise.
 */
int __has_repeated_chars(const char* password, int threshold) {
    size_t size = strlen(password) - threshold;
    for (size_t i = 0; i <= size; i++) {
        int repeated = 1;
        for (int j = 1; j < threshold; j++)
            if (password[i] == password[i + j])
                repeated++;

        if (repeated == threshold) return 1;
    }

    return 0;
}

/**
 * Reverses the given string in place.
 *
 * @param str The string to reverse.
 */
void __str_reverse(char* str) {
    int start = 0;
    int end = strlen(str) - 1;
    char temp;

    while (start < end) {
        temp = str[start];
        str[start] = str[end];
        str[end] = temp;

        start++;
        end--;
    }
}

int __has_keyboard_patterns(const char* password) {
    char lower_pass[MAX_LENGTH];
    strncpy(lower_pass, password, MAX_LENGTH - 1);
    lower_pass[MAX_LENGTH - 1] = '\0';

    for (int i = 0; lower_pass[i]; i++)
        lower_pass[i] = tolower(lower_pass[i]);

    for (int i = 0; BLACKLIST[i]; i++) {
        if (strstr(lower_pass, BLACKLIST[i])) return 1;
        // Check reverse patterns
        char reversed[MAX_LENGTH];
        strcpy(reversed, BLACKLIST[i]);
        __str_reverse(reversed);
        if (strstr(lower_pass, reversed)) return 1;
    }

    return 0;
}

int validate_password(const char* password) {
    if (password == NULL || *password == '\0') return 0;

    size_t length = strlen(password);
    if (!(length >= MIN_LENGTH && length <= MAX_LENGTH)) return 0;

    int uppercase = 0, lowercase = 0, digits = 0, special = 0;
    for (size_t i = 0; i < length; i++) {
        if (isupper(password[i])) uppercase++;
        else if (islower(password[i])) lowercase++;
        else if (isdigit(password[i])) digits++;
        else if (__is_special_char(password[i])) special++;
    }

    if (uppercase < MIN_UPPERCASE) return 0;
    if (lowercase < MIN_LOWERCASE) return 0;
    if (digits < MIN_DIGITS) return 0;
    if (special < MIN_SPECIAL) return 0;
    if (__has_repeated_chars(password, 2)) return 0;

    for (int i = 0; COMMON_PATTERNS[i]; i++)
        if (strstr(password, COMMON_PATTERNS[i]))
            return 0;

    if (__has_keyboard_patterns(password)) return 0;

    return 1;
}
