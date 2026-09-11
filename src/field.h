/*******************************************************************
 * @file field.h
 *
 * @brief Interface de extração de campo de registrador (Atividade-06).
 *******************************************************************/

#ifndef FIELD_H_
#define FIELD_H_

#include <stdint.h>

uint32_t reg_get_field(uint32_t reg, uint8_t shift, uint8_t width);

#endif /* FIELD_H_ */
