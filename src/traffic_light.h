/*******************************************************************
 * @file traffic_light.h
 *
 * @brief Interface da máquina de estado do semáforo (Atividade-09).
 *******************************************************************/

#ifndef TRAFFIC_LIGHT_H_
#define TRAFFIC_LIGHT_H_

enum tl_state {
	TL_RED,
	TL_GREEN,
	TL_YELLOW,
};

enum tl_state tl_next_state(enum tl_state current);

#endif /* TRAFFIC_LIGHT_H_ */
