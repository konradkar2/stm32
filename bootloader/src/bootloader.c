#include <libopencm3/stm32/memorymap.h>
#include <libopencm3/cm3/vector.h>

#define BOOTLOADER_SIZE 0x8000U
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)

static void go_to_app_main(void)
{
	vector_table_t * vector_table = (vector_table_t * )MAIN_APP_START_ADDRESS;
	vector_table->reset();
}

int main(void){

	go_to_app_main();
	return 0;
}
