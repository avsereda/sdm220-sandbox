/**
 * @file input-stream.h
 * @author Sereda Anton <sereda-anton@yandex.ru>
 *
 * Copyright (c) 2017 Sereda Anton
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#ifndef INPUT_STREAM_H
#define INPUT_STREAM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "timer.h"
#include "runtime-error.h"

typedef struct _InputStream InputStream;
typedef enum _InputStreamError InputStreamError;

typedef bool (*InputStreamPollFunc)(InputStream *);
typedef bool (*InputStreamReadByteFunc)(InputStream *, uint8_t *);
typedef void (*InputStreamAsyncReadyCallback)(InputStream *, RuntimeError *, uint8_t *, size_t,
        void *);

enum _InputStreamError {
    INPUT_STREAM_ERROR_OUT_OF_MEMORY = 1,
    INPUT_STREAM_ERROR_TIMEOUT
};

struct _InputStream {
    InputStreamPollFunc poll;
    InputStreamReadByteFunc read_byte;
    InputStreamAsyncReadyCallback ready;
    uint8_t *data;
    size_t data_size;
    size_t alloc_size;
    Timer timer;
    mseconds_t timeout;
    int operation;
    bool available;
    void *user_data;
};

void input_stream_init(InputStream *self, InputStreamReadByteFunc read_byte_func,
                       InputStreamPollFunc poll_func);

bool input_stream_available(InputStream *self);
void input_stream_run(InputStream *self);
bool input_stream_pending(InputStream *self);

void input_stream_read_line_async(InputStream *self, mseconds_t timeout, uint8_t *buffer,
                                  size_t buffer_size, InputStreamAsyncReadyCallback callback,
                                  void *user_data);

void input_stream_read_async(InputStream *self, mseconds_t timeout, uint8_t *buffer,
                             size_t buffer_size, InputStreamAsyncReadyCallback callback,
                             void *user_data);

#endif /* INPUT_STREAM_H */
