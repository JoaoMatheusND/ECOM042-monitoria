#ifndef SOURCE_H
#define SOURCE_H

/**
 * @brief Envia um comando em nome de uma fonte de evento e registra a resposta.
 *
 * @param source Identificador da fonte emissora (ex.: "UART", "BLE", "LED").
 * @param name Nome do comando a ser despachado.
 * @param arg Argumento a ser repassado ao comando (ou NULL se inexistente).
 */
void source_send(const char *source, const char *name, const char *arg);

#endif /* SOURCE_H */
