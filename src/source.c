#include "source.h"

#include <stddef.h>

#include <zephyr/sys/printk.h>

#include "command.h"
#include "commands.h"
#include "dispatcher.h"

/**
 * @brief Tabela estática de mapeamento de comandos.
 */
static const command_t s_command_table[] = {
	{.name = "LED_ON", .execute = cmd_led_on},
	{.name = "LED_OFF", .execute = cmd_led_off},
	{.name = "BTN_PRESS", .execute = cmd_button_press},
	{.name = "BTN_RELEASE", .execute = cmd_button_release},
	{.name = "STATUS", .execute = cmd_get_status},
	{.name = "RESET", .execute = cmd_reset},
};
static const size_t s_command_table_len = sizeof(s_command_table) / sizeof(s_command_table[0]);

/**
 * @brief Converte o código de status de execução para representação textual.
 *
 * @param s Código de status retornado.
 * @return const char* Mensagem descritiva do status.
 */
static const char *status_str(cmd_status_t s)
{
	switch (s) {
	case CMD_OK:
		return "OK";
	case CMD_ERR_NOT_FOUND:
		return "ERRO: comando nao encontrado";
	case CMD_ERR_INVALID_ARGS:
		return "ERRO: argumento invalido";
	case CMD_ERR_EXEC_FAILED:
		return "ERRO: falha na execucao";
	default:
		return "ERRO: status desconhecido";
	}
}

void source_send(const char *source, const char *name, const char *arg)
{
	cmd_args_t args = {.arg = arg};
	cmd_status_t st = command_dispatch(s_command_table, s_command_table_len, name, &args);

	printk("[%s] %s -> %s\n\n", source, name, status_str(st));
}
