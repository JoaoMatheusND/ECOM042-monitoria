#ifndef MINISH_BOARD_IO_H_
#define MINISH_BOARD_IO_H_

#include "dispatch.h"

int board_io_init(void);
const struct minish_hw_ops *board_io_ops(void);

#endif
