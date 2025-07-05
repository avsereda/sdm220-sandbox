#ifndef TIMER_H
#define TIMER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef unsigned long mseconds_t;
typedef struct _Timer Timer;

struct _Timer {
	struct timespec start;
};

void timer_init(Timer *timer);
void timer_start(Timer *timer);
mseconds_t timer_elapsed(Timer *timer);
void timer_reset(Timer *timer);

#endif /* TIMER_H */
