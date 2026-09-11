/*******************************************************************
 * @file tasks.h
 *
 * @brief Interface do superloop cooperativo (Atividade-10).
 *******************************************************************/

#ifndef TASKS_H_
#define TASKS_H_

#include <stdbool.h>

bool task_a_should_run(int tick);
bool task_b_should_run(int tick);

#endif /* TASKS_H_ */
