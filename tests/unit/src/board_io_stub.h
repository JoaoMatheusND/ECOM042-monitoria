/*******************************************************************
 * @file board_io_stub.h
 *
 * @brief Controle do stub de board_io usado nos testes.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#ifndef BOARD_IO_STUB_H_
#define BOARD_IO_STUB_H_

#include <stdbool.h>

void board_io_stub_reset(void);
void board_io_stub_set_button(int state);
void board_io_stub_fail(bool fail);

#endif /* BOARD_IO_STUB_H_ */
