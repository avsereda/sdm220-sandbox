#include <string.h>

#include "runtime-error.h"

void runtime_error_clear(RuntimeError *error)
{
    memset(error, 0, sizeof(RuntimeError));
}

void runtime_error_set(RuntimeError *error, int code, const char *message)
{
    error->code = code;
    strncpy(error->message, message, RUNTIME_ERROR_MESSAGE_SIZE - 1);
}
