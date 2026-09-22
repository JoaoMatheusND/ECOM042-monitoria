/*******************************************************************
 * @file command.h
 *
 * @brief Command Pattern: interface comum a todo comando.
 * @author coquinha0 (rapidoesso@gmail.com)
 * @version 0.1
 * @date 22/09/2026
 *******************************************************************/

#ifndef COMMAND_H_
#define COMMAND_H_

/**
 * @brief Representa um comando: um nome (usado pelo despachante pra
 * localizá-lo) e a ação que ele sabe executar sozinho.
 */
typedef struct {
	const char *name;
	void (*execute)(void);
} command_t;

#endif /* COMMAND_H_ */
