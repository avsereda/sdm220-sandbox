#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H

#include <stddef.h>
#include <stdbool.h>

typedef struct _RuntimeError RuntimeError;

#define RUNTIME_ERROR_MESSAGE_SIZE 64

struct _RuntimeError {
	int code;
	char message[RUNTIME_ERROR_MESSAGE_SIZE];
};

void runtime_error_clear(RuntimeError *error);
void runtime_error_set(RuntimeError *error, int code, const char *message);

#endif /* RUNTIME_ERROR_H */
