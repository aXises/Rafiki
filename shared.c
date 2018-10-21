#include "shared.h"

enum Error load_keyfile(char **output, char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL || !file) {
        return SYSTEM_ERR;
    }
    char character;
    *output = malloc(0);
    int counter = 0;
    while((character = getc(file)) != EOF) {
        if (character == '\n') {
            free(*output);
            return INVALID_KEYFILE;
        }
        *output = realloc(*output, sizeof(char) * (counter + 1));
        (*output)[counter] = character;
        counter++;
    }
    if (counter < 1) {
        free(*output);
        return INVALID_KEYFILE;
    }
    *output = realloc(*output, sizeof(char) * (counter + 1));
    (*output)[counter] = '\0';
    return NOTHING_WRONG;
}

void send_message(FILE *in, char *message, ...) {
    va_list args;
    va_start(args, message);
    vfprintf(in, message, args);
    va_end(args);
    fflush(in);
}

/**
* Check if a string is an digit.
* @param string - The string to check.
* @return int - 1 if the string contains only digits.
*/
int is_string_digit(char *string) {
    for (int i = 0; i < strlen(string); i++) {
        if (!isdigit(string[i])) {
            return 0;
        }
    }
    return 1;
}

/**
* Split an string in to an array of strings by an character.
* @param string - The string to split.
* @param character - The character to split it by.
* @return char - An array of split strings.
*/
char **split(char *string, char *character) {
    char *segment;
    char **splitString = malloc(sizeof(char *));
    int counter = 0;
    while((segment = strsep(&string, character)) != NULL) {
        splitString = realloc(splitString, sizeof(char *) * (counter + 1));
        splitString[counter] = segment;
        counter++;
    }
    return splitString;
}

/**
* Check an array of strings to ensure no empty or space characters.
* @param content - The array of strings to check.
* @param length - The length of the array/
* @return int - True if the array elements contains no spaces or is not empty.
*/
int check_encoded(char **content, int length) {
    for (int i = 0; i < length; i++) {
        if (!is_string_digit(content[i]) ||
                strcmp(content[i], " ") == 0 ||
                strcmp(content[i], "") == 0) {
            return 0;
        }
    }
    return 1;
}


/**
* Match the amount of seperators in a string.
* @param str - The string to parse.
* @param expectedColumn - The expected amount of columns.
* @param expectedComma - The expected amount of commas.
* @return int - 1 if the amount of seperators match.
*/
int match_seperators(char *str, const int expectedColumn,
        const int expectedComma) {
    int colAmount = 0, commaAmount = 0;
    for (int i = 0; i < strlen(str); i++) {
        switch(str[i]) {
            case (':'):
                colAmount++;
                break;
            case (','):
                commaAmount++;
                break;
        }
    }
    return colAmount == expectedColumn && commaAmount == expectedComma;
}