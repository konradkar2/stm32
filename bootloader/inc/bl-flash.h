#ifndef INC_BL_FLASH_H
#define INC_BL_FLASH_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void bl_flash_erase_main_app(void);
void bl_flash_write(const uint32_t address, const uint8_t * data, size_t len);
bool bl_flash_is_dual_bank(void);
uint32_t bl_flash_get_main_app_available_size(void);

#endif /* INC_BL_FLASH_H */
