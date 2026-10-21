#ifndef TRANSPORT_H_
#define TRANSPORT_H_

#include "command.h"

/**
 * @brief Inicie uma vez a recepcao UART e a thread de comandos.
 * @param io Operacoes validas, mantidas durante toda a execucao da thread.
 * @return Zero em sucesso, -EINVAL para callbacks invalidos ou erro da UART.
 */
int transport_init(const struct shell_io *io);

#endif
