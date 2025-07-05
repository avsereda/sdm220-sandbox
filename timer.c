#include <stdio.h>
#include <stdlib.h>

#include "timer.h"

void timer_init(Timer *timer)
{
    timer_start(timer);
}

static inline void get_time(struct timespec *tp)
{
    if (clock_gettime(CLOCK_REALTIME, tp) != 0) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
}

void timer_start(Timer *timer)
{
    get_time(&timer->start);
}

static inline void timespec_diff(struct timespec *start, struct timespec *stop,
                                 struct timespec *result)
{
    /* if ((stop->tv_nsec - start->tv_nsec) < 0u) { */
    /*     result->tv_sec = stop->tv_sec - start->tv_sec - 1u; */
    /*     result->tv_nsec = stop->tv_nsec - start->tv_nsec + 1000000000u; */
    /* } else { */
    /*     result->tv_sec = stop->tv_sec - start->tv_sec; */
    /*     result->tv_nsec = stop->tv_nsec - start->tv_nsec; */
    /* } */




    return;
}

mseconds_t timer_elapsed(Timer *timer)
{
    mseconds_t msec_diff = 0u;
    mseconds_t result = 0u;
    struct timespec now = {0, };
    struct timespec *start = NULL;

    get_time(&now);
    start = &timer->start;

    if (now.tv_nsec >= start->tv_nsec)
        msec_diff = (mseconds_t) (now.tv_nsec - start->tv_nsec);
    else
        msec_diff = (mseconds_t) (start->tv_nsec - now.tv_nsec);

    msec_diff /= 1000000u;
    result = (mseconds_t)((now.tv_sec - start->tv_sec) * 1000u);

    if (now.tv_nsec >= start->tv_nsec)
        result += msec_diff;
    else
        result -= msec_diff;

    return result;
}

void timer_reset(Timer *timer)
{
    timer_start(timer);
}
