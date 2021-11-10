// Copyright (c) 2021 Bjørn Brodtkorb

#ifndef RETRY_H
#define RETRY_H

#include "utilities.h"
#include "time.h"

//----------------------------------------p----------------------------------------------------------

typedef struct {
    Time start_timeout;
    Time max_timeout;
    Time timeout;
    Time timeout_with_jitter;

    Time time;
    int count;

    // A random value between 0 and timeout / jitter_fraction is added or subtracted from the 
    // timeout each backoff.
    int jitter_fraction;
} Backoff;

//--------------------------------------------------------------------------------------------------

void backoff_init(Backoff* backoff, Time start_timeout, Time max_timeout, int jitter_fraction);
bool backoff_timeout(Backoff* backoff);
void next_backoff(Backoff* backoff);
void backoff_reset(Backoff* backoff);

#endif
