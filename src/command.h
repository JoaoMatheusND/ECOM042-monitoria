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

const command_handler_t command_table[COMMAND_TABLE_SIZE]= {
  [UART_CMD] = &handle_uart,
  [TIMER_CMD] = &handle_timer,
  [BUTTON_CMD] = &handle_button,
  [BLE_CMD] = &handle_ble,
};
