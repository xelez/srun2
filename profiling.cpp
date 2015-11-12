/*  Copyright 2014 Alexander Ankudinov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "profiling.h"

#ifdef USE_PROFILING

#include <sys/time.h>
#include <stdio.h>
#define TV_TO_USEC(a) ((a).tv_sec * 1000000ll + (a).tv_usec)

/** @return real time in milliseconds */
long long PROFILE_get_rtime() {
    struct timeval t;
    gettimeofday(&t, NULL);
    return TV_TO_USEC(t);
}

void PROFILING_do_some_stuff() {
    //just stuff that works for about 1 sec
    long long s = 1;
    int t = 100;
    for (int i = 0; i < 100500000; ++i) {
        s = t*s + i;
    }
    printf("some_eval: %lld\n", s);
}

#else /* USE_PROFILING */
long long PROFILE_get_rtime() {}
void PROFILING_do_some_stuff() {}
#endif /* USE_PROFILING */
