// Copyright (c) 2021 Bjørn Brodtkorb

#include "backoff.h"
#include "random.h"

//--------------------------------------------------------------------------------------------------

void backoff_init(Backoff* backoff, Time start_timeout, Time max_timeout, int jitter_fraction) {
    backoff->max_timeout = max_timeout;
    backoff->start_timeout = start_timeout;
    backoff->jitter_fraction = jitter_fraction;

    backoff_reset(backoff);
}

//--------------------------------------------------------------------------------------------------

bool backoff_timeout(Backoff* backoff) {
    return backoff->count == 0 || get_elapsed(backoff->time, get_time()) > backoff->timeout_with_jitter;
}

//--------------------------------------------------------------------------------------------------

void next_backoff(Backoff* backoff) {
    backoff->time = get_time();
    backoff->count++;

    // @Hack for making the user interface a bit simpler. We want the retransmission to fire immediately
    // the first time. After the first time, the timeout is used.
    if (backoff->count) {
        backoff->timeout = limit(backoff->timeout * 2, backoff->max_timeout);
        backoff->timeout_with_jitter = backoff->timeout + (s32)random() % (backoff->timeout / backoff->jitter_fraction);
    }
}

//--------------------------------------------------------------------------------------------------

void backoff_reset(Backoff* backoff) {
    backoff->count = 0;
    backoff->timeout = backoff->start_timeout;
}
