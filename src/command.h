#pragma once
#include <stdint.h>

#define COMMAND_TABLE_SIZE 32

#define ENOCMD 1 /* Codigo de erro comando não existe */

typedef enum {
	UART_CMD,
	BLE_CMD,
	BUTTON_CMD,
	TIMER_CMD,
	ERROR
} command_t;

typedef void (*command_handler_t)();

void handle_uart();
void handle_ble();
void handle_button();
void handle_timer();

uint16_t dispatch(command_t command);
