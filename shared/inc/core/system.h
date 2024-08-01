

#ifndef INC_CORE_SYSTEM_H
#define INC_CORE_SYSTEM_H

#include <stdint.h>

#define CPU_FREQ 216000000
#define SYSTICK_FREQ 1000

void system_setup(void);
uint64_t system_get_ticks(void);

#endif /* INC_CORE_SYSTEM_H */
