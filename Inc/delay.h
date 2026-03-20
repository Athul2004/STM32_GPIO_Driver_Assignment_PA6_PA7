/**
 * @file   delay.h
 * @brief  Header for a simple busy-wait (blocking) delay module.
 *         Provides delay_ms() which burns CPU cycles for an approximate
 *         number of milliseconds, calibrated for the 16 MHz HSI clock.
 */

#ifndef DELAY_H_     /* Include guard: prevents multiple inclusions of this header */
#define DELAY_H_     /* Defines the guard macro                                    */

#include <stdint.h>  /* Provides uint32_t used as the delay parameter type          */

/**
 * @brief   Busy-wait delay for the specified number of milliseconds.
 * @param   ms  Number of milliseconds to wait (approximate at 16 MHz HSI).
 * @note    This is a blocking call — the CPU does nothing useful while waiting.
 *          For long delays, consider using a hardware timer or SysTick instead.
 */
void delay_ms(uint32_t ms);

#endif /* DELAY_H_ */
