#include <string.h>
#include "string_utils.h"

bool string_starts_with(const char *str, const char *prefix) {
    return strncmp(prefix, str, strlen(prefix)) == 0;
}

bool char_is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

bool string_is_empty(const char *str) {
    while(*str) {
        if (! char_is_whitespace(*str++ ))
            return false;
    }
    return true;
}
