#include "commands.h"
#include "semaforo.h"

#include <zephyr/sys/printk.h>

void cmd_verde(void *ctx)
{
	semaforo_set(ctx, SEMAFORO_VERDE);
}

void cmd_amarelo(void *ctx)
{
	semaforo_set(ctx, SEMAFORO_AMARELO);
}

void cmd_vermelho(void *ctx)
{
	semaforo_set(ctx, SEMAFORO_VERMELHO);
}

void cmd_pedestre(void *ctx)
{
	struct semaforo *s = ctx;

	if (s->estado != SEMAFORO_VERDE) {
		printk("pedestre: semaforo em %s, nada a fazer\n", semaforo_estado_str(s->estado));
		return;
	}

	printk("pedestre: fechando para travessia\n");
	semaforo_set(s, SEMAFORO_AMARELO);
	semaforo_set(s, SEMAFORO_VERMELHO);
}

void cmd_alerta(void *ctx)
{
	semaforo_set(ctx, SEMAFORO_ALERTA);
}
