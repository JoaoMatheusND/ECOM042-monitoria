#ifndef BOARD_IO_H_
#define BOARD_IO_H_

#include "core/shell_cmd.h"

int board_io_init(void);
const struct shell_hw_ops *board_io_ops(void);

#endif
