/**
 ******************************************************************************
 * @file 	aux.h
 * @author	Paolo Sassi
 * @date	10 February 2016
 * @brief	Application auxiliary functions header file.
 ******************************************************************************
 */

#ifndef AUX_H_
#define AUX_H_

/**
 * @addtogroup auxiliary Auxiliary Functions
 * @{
 */

/**
 * @defgroup aux_exp_func Auxiliary Exported Functions
 * @{
 */

uint8_t sonarInit();
void printString(uint16_t x, uint16_t y, uint8_t *s, sFONT *f);
void printAxis();
void printResults(uint16_t *buff);
void sleep(int32_t msec);
uint8_t cutOff(uint16_t *buff);
void filterSamples(uint16_t *buff);

/**
 * @}
 */

/**
 * @}
 */

#endif /* AUX_H_ */
