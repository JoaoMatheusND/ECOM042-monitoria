/*******************************************************************
 * @file main.c
 *
 * @brief Testes do despacho de comandos e da montagem de linha.
 * @author Erick Mascarenhas (edlm@ic.ufal.br)
 * @version 0.1
 * @date 21/10/2026
 *******************************************************************/

#include <errno.h>
#include <string.h>

#include <zephyr/ztest.h>

#include "board_io_stub.h"
#include "cmd.h"
#include "line.h"

static char out[CONFIG_MINISHELL_RESP_SIZE];

static const char *dispatch(const char *input)
{
	(void)cmd_dispatch(input, out, sizeof(out));

	return out;
}

static void reset_stub(void *fixture)
{
	ARG_UNUSED(fixture);

	board_io_stub_reset();
}

ZTEST_SUITE(minishell_cmd, NULL, NULL, reset_stub, NULL, NULL);

ZTEST(minishell_cmd, test_help_lista_os_comandos)
{
	const char *resp = dispatch("HELP");

	zassert_not_null(strstr(resp, "HELP"), "faltou HELP");
	zassert_not_null(strstr(resp, "ECHO"), "faltou ECHO");
	zassert_not_null(strstr(resp, "\r\n"), "esperava uma linha por comando");
}

ZTEST(minishell_cmd, test_led_on_e_off_respondem_ok)
{
	zassert_str_equal(dispatch("LED ON"), "OK");
	zassert_str_equal(dispatch("LED OFF"), "OK");
}

ZTEST(minishell_cmd, test_led_sem_argumento_consulta_o_estado)
{
	zassert_str_equal(dispatch("LED"), "LED: OFF");

	(void)dispatch("LED ON");
	zassert_str_equal(dispatch("LED"), "LED: ON");

	(void)dispatch("LED OFF");
	zassert_str_equal(dispatch("LED"), "LED: OFF");
}

ZTEST(minishell_cmd, test_btn_le_o_botao)
{
	zassert_str_equal(dispatch("BTN"), "BTN: 0");

	board_io_stub_set_button(1);
	zassert_str_equal(dispatch("BTN"), "BTN: 1");
}

ZTEST(minishell_cmd, test_uptime_tem_o_formato_esperado)
{
	const char *resp = dispatch("UPTIME");
	size_t len = strlen(resp);

	zassert_equal(strncmp(resp, "UPTIME: ", 8), 0, "prefixo errado: %s", resp);
	zassert_true(len > 11U, "resposta curta demais: %s", resp);
	zassert_str_equal(&resp[len - 3], " ms", "sufixo errado: %s", resp);
}

ZTEST(minishell_cmd, test_echo_devolve_o_texto_exatamente)
{
	zassert_str_equal(dispatch("ECHO oi"), "oi");
	zassert_str_equal(dispatch("ECHO Oi MuNdO"), "Oi MuNdO",
			  "ECHO mudou maiusculas/minusculas");
	zassert_str_equal(dispatch("ECHO"), "");
}

ZTEST(minishell_cmd, test_comando_desconhecido)
{
	zassert_equal(cmd_dispatch("FOO", out, sizeof(out)), -ENOENT);
	zassert_str_equal(out, CMD_ERR_UNKNOWN);
}

ZTEST(minishell_cmd, test_comandos_sao_case_insensitive)
{
	zassert_str_equal(dispatch("led on"), "OK");
	zassert_str_equal(dispatch("LeD"), "LED: ON");
	zassert_str_equal(dispatch("bTn"), "BTN: 0");
}

ZTEST(minishell_cmd, test_linha_vazia_nao_gera_resposta)
{
	zassert_equal(cmd_dispatch("", out, sizeof(out)), 0);
	zassert_str_equal(out, "");

	zassert_equal(cmd_dispatch("   ", out, sizeof(out)), 0);
	zassert_str_equal(out, "");
}

ZTEST(minishell_cmd, test_led_com_argumento_invalido)
{
	zassert_str_equal(dispatch("LED FOO"), CMD_ERR_UNKNOWN);
	zassert_str_equal(dispatch("LED ON EXTRA"), CMD_ERR_UNKNOWN);
}

ZTEST(minishell_cmd, test_falha_de_io_e_reportada)
{
	board_io_stub_fail(true);

	zassert_str_equal(dispatch("LED ON"), CMD_ERR_IO);
	zassert_str_equal(dispatch("BTN"), CMD_ERR_IO);
}

ZTEST(minishell_cmd, test_argumentos_invalidos_sao_rejeitados)
{
	zassert_equal(cmd_dispatch(NULL, out, sizeof(out)), -EINVAL);
	zassert_equal(cmd_dispatch("HELP", NULL, sizeof(out)), -EINVAL);
	zassert_equal(cmd_dispatch("HELP", out, 0U), -EINVAL);
}

/* ------------------------------------------------------------------ */

ZTEST_SUITE(minishell_line, NULL, NULL, NULL, NULL, NULL);

static line_event_t feed_str(line_t *line, const char *s)
{
	line_event_t ev = LINE_EVENT_NONE;

	while (*s != '\0') {
		ev = line_feed(line, *s);
		s++;
	}

	return ev;
}

ZTEST(minishell_line, test_linha_fica_pronta_no_terminador)
{
	line_t line;

	line_reset(&line);
	zassert_equal(feed_str(&line, "LED ON\n"), LINE_EVENT_READY);
	zassert_str_equal(line_str(&line), "LED ON");
}

ZTEST(minishell_line, test_cr_tambem_termina_a_linha)
{
	line_t line;

	line_reset(&line);
	zassert_equal(feed_str(&line, "BTN\r"), LINE_EVENT_READY);
	zassert_str_equal(line_str(&line), "BTN");
}

ZTEST(minishell_line, test_backspace_apaga_o_ultimo_caractere)
{
	line_t line;

	line_reset(&line);
	(void)feed_str(&line, "ABC");

	zassert_equal(line_feed(&line, 0x08), LINE_EVENT_ERASE);
	zassert_equal(line_feed(&line, 0x7F), LINE_EVENT_ERASE);
	zassert_equal(line_feed(&line, '\n'), LINE_EVENT_READY);
	zassert_str_equal(line_str(&line), "A");
}

ZTEST(minishell_line, test_backspace_em_linha_vazia_nao_faz_nada)
{
	line_t line;

	line_reset(&line);

	zassert_equal(line_feed(&line, 0x08), LINE_EVENT_NONE);
	zassert_equal(line_feed(&line, '\n'), LINE_EVENT_READY);
	zassert_str_equal(line_str(&line), "");
}

ZTEST(minishell_line, test_linha_no_limite_e_aceita)
{
	line_t line;
	size_t i;

	line_reset(&line);

	for (i = 0U; i < CONFIG_MINISHELL_LINE_SIZE; i++) {
		zassert_equal(line_feed(&line, 'x'), LINE_EVENT_ECHO, "byte %zu recusado", i);
	}

	zassert_equal(line_feed(&line, '\n'), LINE_EVENT_READY);
	zassert_equal(strlen(line_str(&line)), (size_t)CONFIG_MINISHELL_LINE_SIZE);
}

ZTEST(minishell_line, test_linha_longa_demais_e_descartada_por_inteiro)
{
	line_t line;
	size_t i;

	line_reset(&line);

	for (i = 0U; i < CONFIG_MINISHELL_LINE_SIZE; i++) {
		(void)line_feed(&line, 'x');
	}

	zassert_equal(line_feed(&line, 'y'), LINE_EVENT_NONE);
	zassert_equal(line_feed(&line, 'z'), LINE_EVENT_NONE);

	zassert_equal(line_feed(&line, '\n'), LINE_EVENT_OVERFLOW);

	/* a proxima linha comeca limpa */
	zassert_equal(feed_str(&line, "BTN\n"), LINE_EVENT_READY);
	zassert_str_equal(line_str(&line), "BTN");
}
