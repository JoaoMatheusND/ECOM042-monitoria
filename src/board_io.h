#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include <stdbool.h>

int io_init(void);
int led_set(bool on);
int led_get(void);
int button_read(void);

#endif