

#ifndef INC_CORE_SYSTEM_H
#define INC_CORE_SYSTEM_H

#include <stdint.h>

#define CPU_FREQ 216000000
#define SYSTICK_FREQ 1000
#define BOOTLOADER_SIZE 0x10000U

void system_setup(void);
void system_terminate(void);
uint64_t system_get_ticks(void);

#endif /* INC_CORE_SYSTEM_H */
