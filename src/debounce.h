/*******************************************************************
 * @file debounce.h
 *
 * @brief Interface do filtro de debounce (Atividade-08).
 *******************************************************************/

#ifndef DEBOUNCE_H_
#define DEBOUNCE_H_

#include <stdbool.h>

#define DEBOUNCE_THRESHOLD 3

bool debounce_update(bool raw_sample);

#endif /* DEBOUNCE_H_ */
