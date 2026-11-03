#ifndef IO_H
#define IO_H

#include <stdbool.h>
#include <stdint.h>

int io_init(void);
void io_led_set(bool on);
bool io_led_get(void);
int io_btn_get(void);
int64_t io_uptime_get(void);

#endif
