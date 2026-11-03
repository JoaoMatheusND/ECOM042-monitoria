/*******************************************************************
 * @file uart_transport.h
 *
 * @brief Camada de transporte: uart1 como canal de comandos.
 *******************************************************************/

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Liga a RX por interrupcao e cria a thread consumidora.
 *
 * @return 0 em sucesso ou errno negativo.
 */
int uart_transport_init(void);

#ifdef __cplusplus
}
#endif
