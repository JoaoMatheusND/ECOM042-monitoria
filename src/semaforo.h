#ifndef SEMAFORO_H
#define SEMAFORO_H

enum semaforo_estado {
	SEMAFORO_VERMELHO,
	SEMAFORO_VERDE,
	SEMAFORO_AMARELO,
	SEMAFORO_ALERTA,
};

struct semaforo {
	enum semaforo_estado estado;
};

void semaforo_init(struct semaforo *s);
void semaforo_set(struct semaforo *s, enum semaforo_estado novo);
const char *semaforo_estado_str(enum semaforo_estado estado);

#endif
