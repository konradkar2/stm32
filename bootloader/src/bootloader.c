#include "bl-flash.h"
#include "comms.h"
#include "core/system.h"
#include <core/logger.h>
#include <core/simple-timer.h>
#include <core/str.h>
#include <core/uart.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>
#include <stdint.h>
#include <stdio.h>

#define BOOTLOADER_SIZE	       0x10000U
#define MAIN_APP_START_ADDRESS (FLASH_BASE + BOOTLOADER_SIZE)
#define MAX_FW_LENGTH	       (FLASH_SIZE)
static void go_to_app_main(void)
{
	while (true) {
	}
	// vector_table_t * vector_table = (vector_table_t * )MAIN_APP_START_ADDRESS;
	// vector_table->reset();
}

static struct uart_driver s_uart_firmware_io = {.dev	       = USART3,
						.clock_dev     = RCC_USART3,
						.nvic_irq      = NVIC_USART3_IRQ,
						.gpio_pins     = GPIO8 | GPIO9,
						.gpio_port     = GPIOD,
						.gpio_port_clk = RCC_GPIOD,
						.gpio_af       = GPIO_AF7,
						.baud_rate     = 115200,
						.mode	       = USART_MODE_TX_RX};

void usart3_isr(void)
{
	uart_handle_irq(&s_uart_firmware_io);
}

#define DEVICE_ID  (0x69)
#define SYNC_SEQ_0 (0x11)
#define SYNC_SEQ_1 (0x22)
#define SYNC_SEQ_2 (0x33)
#define SYNC_SEQ_3 (0x44)
#define TIMEOUT_MS (5000)

enum bl_state_step {
	bl_state_step_sync,
	bl_state_step_wait_for_update_req,
	bl_state_step_device_id_req,
	bl_state_step_device_id_res,
	bl_state_step_firmware_length_req,
	bl_state_step_firmware_length_res,
	bl_state_step_erase_app,
	bl_state_step_receive_firmware,
	bl_state_step_done,
};

static const char *bl_state_step_str(enum bl_state_step step)
{
	switch (step) {

		ENUM_CASE(bl_state_step_sync)
		ENUM_CASE(bl_state_step_wait_for_update_req)
		ENUM_CASE(bl_state_step_device_id_req)
		ENUM_CASE(bl_state_step_device_id_res)
		ENUM_CASE(bl_state_step_firmware_length_req)
		ENUM_CASE(bl_state_step_firmware_length_res)
		ENUM_CASE(bl_state_step_erase_app)
		ENUM_CASE(bl_state_step_receive_firmware)
		ENUM_CASE(bl_state_step_done)
	default:
		return "bl_state_step unknown";
	}
}

struct bl_state {
	enum bl_state_step  step;
	uint8_t		    sync_seq[4];
	uint32_t	    fw_length;
	uint32_t	    fw_length_received;
	struct simple_timer timeout_timer;
};

static struct bl_state bl_state = {
    .step		= bl_state_step_sync,
    .sync_seq		= {0},
    .fw_length		= 0,
    .fw_length_received = 0,
    .timeout_timer	= {0},
};
struct comms comms = {0};

static void abort_fw_update(const char *reason)
{
	comms_send_control_packet(&comms, comms_packet_type_fw_update_aborted);

	printf("bootloader FW update aborted at: %s, reason: %s, starting the "
	       "app...\n",
	       bl_state_step_str(bl_state.step), reason);

	go_to_app_main();
}

static void receive_verify_packet(enum comms_packet_type type, struct comms_packet *packet_out)
{
	enum comms_packet_type actual_packet_type = 0;
	if (packet_out) {
		comms_receive(&comms, packet_out);
		actual_packet_type = packet_out->type;
	} else {
		struct comms_packet tmp_packet = {0};
		comms_receive(&comms, &tmp_packet);
		actual_packet_type = tmp_packet.type;
	}

	if (actual_packet_type != type) {
		printf("expected to received (%s), instead got (%s)\n", comms_packet_type_str(type),
		       comms_packet_type_str(actual_packet_type));
		abort_fw_update("invalid packet");
	}
}

static void receive_verify_control_packet(enum comms_packet_type type)
{
	receive_verify_packet(type, NULL);
}

static void check_timeout(void)
{
	if (simple_timer_has_elapsed(&bl_state.timeout_timer)) {
		abort_fw_update("timeout");
	}
}

static void advance_fsm_to(enum bl_state_step step)
{
	printf("advancing fsm to %s\n", bl_state_step_str(step));
	simple_timer_reset(&bl_state.timeout_timer);
	bl_state.step = step;
}

int main(void)
{
	system_setup();
	uart_setup(&s_uart_firmware_io);
	stdout = create_logger();
	printf("Booting device...\n");

	comms_setup(&comms, &s_uart_firmware_io);
	printf("Comms setup done\n");

	if (bl_flash_is_dual_bank()) {

		printf("dual bank is enabled, cannot perform flash operation\n");
		return 1;
	}

	simple_timer_setup(&bl_state.timeout_timer, TIMEOUT_MS, false);

	printf("Waiting for FW update sync...\n");

	while (true) {
		check_timeout();

		switch (bl_state.step) {
		case bl_state_step_sync: {
			if (uart_data_available(&s_uart_firmware_io)) {
				bl_state.sync_seq[0] = bl_state.sync_seq[1];
				bl_state.sync_seq[1] = bl_state.sync_seq[2];
				bl_state.sync_seq[2] = bl_state.sync_seq[3];
				bl_state.sync_seq[3] = uart_read_byte(&s_uart_firmware_io);

				if (bl_state.sync_seq[0] == SYNC_SEQ_0 &&
				    bl_state.sync_seq[1] == SYNC_SEQ_1 &&
				    bl_state.sync_seq[2] == SYNC_SEQ_2 &&
				    bl_state.sync_seq[3] == SYNC_SEQ_3) {

					printf("sync seq observed, sending seq observed\n");

					comms_send_control_packet(&comms,
								  comms_packet_type_seq_observed);
					advance_fsm_to(bl_state_step_wait_for_update_req);
				} else {
					check_timeout();
				}
			}
		} break;
		case bl_state_step_wait_for_update_req: {
			if (comms_packet_available(&comms)) {
				receive_verify_control_packet(comms_packet_type_fw_update_req);
				comms_send_control_packet(&comms, comms_packet_type_fw_update_res);
				advance_fsm_to(bl_state_step_device_id_req);
			}

		} break;
		case bl_state_step_device_id_req: {
			comms_send_control_packet(&comms, comms_packet_type_device_id_req);
			advance_fsm_to(bl_state_step_device_id_res);

		} break;
		case bl_state_step_device_id_res: {

			if (comms_packet_available(&comms)) {
				struct comms_packet device_id_res_packet = {0};
				receive_verify_packet(comms_packet_type_device_id_res,
						      &device_id_res_packet);

				if (device_id_res_packet.length != 1) {
					abort_fw_update("invalid length of device_id_req packet");
				}
				if (device_id_res_packet.data[0] != DEVICE_ID) {
					abort_fw_update("invalid device id");
				}
				advance_fsm_to(bl_state_step_firmware_length_req);
			}
		} break;
		case bl_state_step_firmware_length_req: {
			comms_send_control_packet(&comms, comms_packet_type_fw_length_req);
			advance_fsm_to(bl_state_step_firmware_length_res);
		} break;
		case bl_state_step_firmware_length_res: {
			if (comms_packet_available(&comms)) {
				struct comms_packet fw_length_packet = {0};
				receive_verify_packet(comms_packet_type_fw_length_res,
						      &fw_length_packet);

				if (fw_length_packet.length != 4) {
					abort_fw_update("invalid length of fw_length_packet");
				}

				const uint32_t fw_length =
				    fw_length_packet.data[0] << 0 | fw_length_packet.data[1] << 8 |
				    fw_length_packet.data[2] << 16 | fw_length_packet.data[3] << 24;

				printf("new firmware size is %lu\n", fw_length);

				if (fw_length > bl_flash_get_main_app_available_size()) {
					abort_fw_update("firmware size exceeded");
				}

				bl_state.fw_length = fw_length;
				advance_fsm_to(bl_state_step_erase_app);
			}
		} break;
		case bl_state_step_erase_app: {
			bl_flash_erase_main_app();
			comms_send_control_packet(&comms, comms_packet_type_ready_for_firmware);
			advance_fsm_to(bl_state_step_receive_firmware);
		} break;
		case bl_state_step_receive_firmware: {
			if (comms_packet_available(&comms)) {
				struct comms_packet data_packet = {0};
				receive_verify_packet(comms_packet_type_data, &data_packet);

				bl_flash_write(MAIN_APP_START_ADDRESS + bl_state.fw_length_received,
					       data_packet.data, data_packet.length);
				bl_state.fw_length_received += data_packet.length;

				if (bl_state.fw_length_received >= bl_state.fw_length) {
					advance_fsm_to(bl_state_step_done);
				}

				simple_timer_reset(&bl_state.timeout_timer);
			}

		} break;
		case bl_state_step_done: {
			printf("fw update done!\n");
			go_to_app_main();

		} break;
		}
	}

	return 0;
}

