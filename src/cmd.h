/*******************************************************************
 * @file cmd.h
 *
 * @brief Interpretacao e despacho dos comandos da shell.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#ifndef CMD_H_
#define CMD_H_

#include <stddef.h>

#define CMD_ERR_UNKNOWN       "ERR: unknown command"
#define CMD_ERR_IO            "ERR: io error"
#define CMD_ERR_LINE_TOO_LONG "ERR: line too long"

/* Escreve em out a resposta para a linha, sem o \r\n final.
 * Retorna 0 se reconheceu o comando, -ENOENT se nao. */
int cmd_dispatch(const char *line, char *out, size_t out_size);

#endif /* CMD_H_ */
