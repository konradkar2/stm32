#ifndef INC_BL_FLASH_H
#define INC_BL_FLASH_H

#include <stdint.h>
#include <stddef.h>

void bl_flash_eraise_main_app(void);
void bl_flash_write(const uint32_t address, const uint8_t * data, size_t len);

#endif /* INC_BL_FLASH_H */
