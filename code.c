/**
 ******************************************************************************
 * @file 	code.c
 * @author 	Paolo Sassi
 * @date 	21 January 2016
 * @brief 	Application main source file.
 ******************************************************************************
 * @attention
 *
 * ERIKA Enterprise - a tiny RTOS for small microcontrollers
 *
 * Copyright (C) 2002-2013  Evidence Srl
 *
 * This file is part of ERIKA Enterprise.
 *
 * ERIKA Enterprise is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation,
 * (with a special exception described below).
 *
 * Linking this code statically or dynamically with other modules is
 * making a combined work based on this code.  Thus, the terms and
 * conditions of the GNU General Public License cover the whole
 * combination.
 *
 * As a special exception, the copyright holders of this library give you
 * permission to link this code with independent modules to produce an
 * executable, regardless of the license terms of these independent
 * modules, and to copy and distribute the resulting executable under
 * terms of your choice, provided that you also meet, for each linked
 * independent module, the terms and conditions of the license of that
 * module.  An independent module is a module which is not derived from
 * or based on this library.  If you modify this code, you may extend
 * this exception to your version of the code, but you are not
 * obligated to do so.  If you do not wish to do so, delete this
 * exception statement from your version.
 *
 * ERIKA Enterprise is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 along with ERIKA Enterprise; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 ******************************************************************************
 */

#include "ee.h"

#include <stdio.h>
#include "stm32f4xx.h"
#include "stm32f4_discovery_lcd.h"

#include "i2c_lib.h"
#include "aux.h"
#include "defines.h"

/**
 * @defgroup main Application
 * @{
 */

/**
 * @defgroup globals Global Variables
 * @{
 */

/**
 * @brief Number of elapsed ranging rounds
 */
uint8_t r_num = 0;

/**
 * @brief Buffer containing the ranging data
 */
uint16_t p_buff[MAX_POINTS];

/**
 * @}
 */

/**
 * @defgroup isr Interrupt Routines
 * @{
 */
ISR2(systick_handler)
{
	/* count the interrupts, waking up expired alarms */
	CounterTick(myCounter);
}
/**
 * @}
 */

/**
 * @defgroup tasks Tasks
 * @{
 */

/**
 * @brief Handles the user interaction through the LCD display
 */
TASK(AppController)
{
static uint16_t i;
char s[20];
uint32_t p_x, p_y;

	if (r_num == 0) {
		sprintf(s, "Touch the screen to\0");
		printString(40, 120, s, FONT_BIG);
		sprintf(s, "start the ranging...\0");
		printString(40, 150, s, FONT_BIG);
		GetTouch_SC_Sync(&p_x, &p_y);
		LCD_Clear(White);
	}

	if (r_num < MAX_ROUNDS) {
		/* Waits 3 seconds, then starts the ranging */
		for (i = 0; i < 3; i++) {
			LCD_DrawFilledRect(230, 115, 300, 135, White, White);
			sprintf(s, "Capturing starts in: %d\0", 3 - i);
			printString(40, 120, s, FONT_BIG);
			sleep(1000);
		}
		LCD_DrawFilledRect(40, 115, 300, 135, White, White);
		/* Start the RFController task */
		SetRelAlarm(AlarmRFC, 10, 50);
	} else {
		LCD_Clear(White);
		printAxis();			/* draws the axis */
		printResults(p_buff);	/* draws the results */
	}
}

/**
 * @brief Starts the ranging and stores the received data into the
 * global buffer
 */
TASK(RFController)
{
static uint16_t i = 0;
uint16_t y = 0;
uint8_t low, high;
char s[20];

	if (i == 0) {
		LCD_Clear(White);
		sprintf(s, "Ranging round %d out of %d", r_num + 1, MAX_ROUNDS);
		printString(10, 10, s, FONT_SMALL);
	}
	low = high = 0;
	LCD_SetTextColor(Black);
	I2C_write(ADDRESS, COMM_REG, RES_CM);
	/* Wait for ranging completion. Normally we should wait for
	 * 65ms, but with this sensor setup we can shorten the wait time */
	sleep(45);
	I2C_read(ADDRESS, 0x03, &low);	/* first echo low byte */
	I2C_read(ADDRESS, 0x02, &high);	/* first echo high byte */
	y = (high << 8) + low;
	/* Correlation among various ranging rounds */
	if (r_num == 0) {	/* first round */
		p_buff[i] = y;
	} else {
		p_buff[i] = K * p_buff[i] + (1 - K) * y;
	}
	/* p_buff[i] = 0 means that the object is beyond the SRF range,
	 * so we put the detected distance to the maximum possible */
	if (p_buff[i] > MAX_DIST || p_buff[i] == 0) p_buff[i] = MAX_DIST;

	LCD_DrawFilledRect(130, 115, 300, 135, White, White);
	sprintf(s, "Ranging... %.2f%%", (float)(i * 100) / MAX_POINTS);
	printString(10, 120, s, FONT_BIG);

	/* Ranging operation completed */
	if (i == MAX_POINTS) {
		r_num++;
		i = 0;
		LCD_Clear(White);
		CancelAlarm(AlarmRFC);
		ActivateTask(AppController);
	} else {
		i++;
	}
}

/**
 * @brief Main task.
 */
int main(void)
{
	SystemInit();

	/* Initialize Erika related stuffs*/
	EE_system_init();

	/* Initialize systick */
	EE_systick_set_period(MILLISECONDS_TO_TICKS(1, SystemCoreClock));
	EE_systick_enable_int();
	EE_systick_start();

	/* Initialize the LCD Display and the touch interface */
	IOE_Config();
	STM32f4_Discovery_LCD_Init();
	InitTouch(-0.0853, 0.0665, -331, 15);

	/* Initialize the sonar */
	sonarInit();

	/* Clear the screen */
	LCD_Clear(White);

	/* Activates the initial task set */
	ActivateTask(AppController);

	/* Forever loop: background activities (if any) should go here */
	for (;;);
}

/**
 * @}
 */

/**
 * @}
 */
