#include "semaforo.h"

#include <zephyr/sys/printk.h>

void semaforo_init(struct semaforo *s)
{
	s->estado = SEMAFORO_VERMELHO;
	printk("semaforo: iniciado em %s\n", semaforo_estado_str(s->estado));
}

void semaforo_set(struct semaforo *s, enum semaforo_estado novo)
{
	printk("semaforo: %s -> %s\n", semaforo_estado_str(s->estado), semaforo_estado_str(novo));
	s->estado = novo;
}

const char *semaforo_estado_str(enum semaforo_estado estado)
{
	switch (estado) {
	case SEMAFORO_VERMELHO:
		return "VERMELHO";
	case SEMAFORO_VERDE:
		return "VERDE";
	case SEMAFORO_AMARELO:
		return "AMARELO";
	case SEMAFORO_ALERTA:
		return "ALERTA";
	default:
		return "DESCONHECIDO";
	}
}
