#ifndef INC_CORE_SIMPLE_TIMER_H
#define INC_CORE_SIMPLE_TIMER_H

#include <stdint.h>
#include <stdbool.h>

struct simple_timer {
    uint64_t wait_time;  
    uint64_t target_time;
    bool auto_reset;
    bool has_elapsed_before;
};

void simple_timer_setup(struct simple_timer * t, uint64_t wait_time, bool auto_reset);
bool simple_timer_has_elapsed(struct simple_timer * t);
void simple_timer_reset(struct simple_timer * t);

#endif /* INC_CORE_SIMPLE_TIMER_H */
