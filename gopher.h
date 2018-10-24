#ifndef GOPHER_H
#define GOPHER_H

#include "shared.h"

#define EXPECTED_ARGC 2

/**
 * Enum for gopher arguments
 */
enum Argument {
    PORT = 1
};

/**
 * Function prototypes
 */
void exit_with_error(int error);
void check_args(int argc, char **argv);
enum Error get_socket(int *output, char *port);
#endif