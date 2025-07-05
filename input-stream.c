#include <string.h>

#include "input-stream.h"

enum {
    OPERATION_READ_LINE = 1,
    OPERATION_READ
};

void input_stream_init(InputStream *self, InputStreamReadByteFunc read_byte_func,
                       InputStreamPollFunc poll_func)
{
    self->poll = poll_func;
    self->read_byte = read_byte_func;
    self->ready = NULL;
    self->data = NULL;
    self->data_size = 0u;
    self->alloc_size = 0u;
    self->timeout = 0u;
    self->operation = 0;
    self->available = false;
    self->user_data = NULL;

    timer_init(&self->timer);
}

bool input_stream_available(InputStream *self)
{
    if (self->available)
        return true;

    if (self->poll == NULL)
        /*
         * NOTE: We assume stream always ready!!!
         */
        return true;

    self->available = self->poll(self);
    return self->available;
}

static inline void notify(InputStream *self, RuntimeError *error)
{
    if (self->ready == NULL)
        return;

    self->ready(self, error, self->data, self->data_size, self->user_data);
    self->ready = NULL;
}

static inline void notify_error_timeout(InputStream *self)
{
    RuntimeError error;

    runtime_error_clear(&error);
    runtime_error_set(&error,
                      INPUT_STREAM_ERROR_OUT_OF_MEMORY, "Out of memory");

    notify(self, &error);
}

static inline void notify_error_out_of_memory(InputStream *self)
{
    RuntimeError error;

    runtime_error_clear(&error);
    runtime_error_set(&error,
                      INPUT_STREAM_ERROR_TIMEOUT, "Operation timeout");

    notify(self, &error);
}

static inline bool is_timeout(InputStream *self)
{
    return timer_elapsed(&self->timer) >= self->timeout;
}

static inline bool is_byte_ready(InputStream *self, uint8_t *byte)
{
    if (!input_stream_available(self)) {
        if (is_timeout(self)) {
            notify_error_timeout(self);
        }

        return false;
    }

    if (!self->read_byte(self, byte)) {
        /*
         * TODO: Log stream error
         */
        return false;
    }

    self->available = false;
    return true;
}

static inline void read_line_run(InputStream *self)
{
    uint8_t byte = 0u;

    if (is_byte_ready(self, &byte)) {
        if (self->data_size > (self->alloc_size - 1u)) {
            notify_error_out_of_memory(self);
            return;
        }

        self->data[self->data_size++] = byte;
        if ((int) byte == '\n') {
            self->data[self->data_size] = (uint8_t) '\0';
            notify(self, NULL);
        }
    }
}

static inline void read_run(InputStream *self)
{
    uint8_t byte = 0u;

    if (is_byte_ready(self, &byte)) {
        self->data[self->data_size++] = byte;

        if (self->data_size == self->alloc_size)
            notify(self, NULL);
    }
}

void input_stream_run(InputStream *self)
{
    if (!input_stream_pending(self))
        return;

    switch (self->operation) {
    case OPERATION_READ_LINE:
        read_line_run(self);
        break;

    case OPERATION_READ:
        read_run(self);
        break;

    default:
        break;
    }
}

bool input_stream_pending(InputStream *self)
{
    return self->ready != NULL;
}

static bool start_operation(InputStream *self, mseconds_t timeout, uint8_t *buffer,
                            size_t buffer_size, InputStreamAsyncReadyCallback callback, void *user_data)
{
    if (input_stream_pending(self))
        return false;

    if (buffer == NULL || buffer_size == 0u)
        return false;

    if (callback == NULL)
        return false;

    /*
     * Start operation:
     */
    timer_start(&self->timer);

    self->timeout = timeout;
    self->ready = callback;
    self->user_data = user_data;
    self->data = buffer;
    self->data_size = 0u;
    self->alloc_size = buffer_size;

    memset(self->data, 0, self->alloc_size);
    return true;
}

void input_stream_read_line_async(InputStream *self, mseconds_t timeout, uint8_t *buffer,
                                  size_t buffer_size, InputStreamAsyncReadyCallback callback, void *user_data)
{
    if (!start_operation(self, timeout,
                         buffer, buffer_size, callback, user_data))
        return;

    self->operation = OPERATION_READ_LINE;
}

void input_stream_read_async(InputStream *self, mseconds_t timeout, uint8_t *buffer,
                             size_t buffer_size, InputStreamAsyncReadyCallback callback,
                             void *user_data)
{
    if (!start_operation(self, timeout,
                         buffer, buffer_size, callback, user_data))
        return;

    self->operation = OPERATION_READ;
}
