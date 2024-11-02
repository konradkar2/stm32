#include "bl-flash.h"
#include <libopencm3/stm32/flash.h>

bool bl_flash_is_dual_bank(void)
{
	return !(FLASH_OPTCR & (1 << 29));
}

#define MAIN_APP_SECTOR_START (2)
#define MAIN_APP_SECTOR_END   (11)

uint16_t sector_size_kb[] = {
    [0] = 32,  [1] = 32,  [2] = 32,  [3] = 32,	[4] = 128,  [5] = 256,
    [6] = 256, [7] = 256, [8] = 256, [9] = 256, [10] = 256, [11] = 256,
};

void bl_flash_erase_main_app(void)
{
	flash_unlock();
	for (uint8_t sector = MAIN_APP_SECTOR_START; sector < MAIN_APP_SECTOR_END; ++sector) {
		flash_erase_sector(sector, FLASH_CR_PROGRAM_X32);
	}
	flash_lock();
}

uint32_t bl_flash_get_main_app_available_size(void)
{
	uint32_t sum = 0;
	for (uint8_t sector = MAIN_APP_SECTOR_START; sector < MAIN_APP_SECTOR_END; ++sector) {
		sum += (sector_size_kb[sector] * 1024);
	}

	return sum;
}

void bl_flash_write(const uint32_t address, const uint8_t *data, size_t len)
{
	flash_unlock();
	flash_program(address, data, len);
	flash_lock();
}
