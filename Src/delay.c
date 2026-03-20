/**
 * @file   delay.c
 * @brief  Busy-wait delay implementation for STM32F4xx at 16 MHz HSI.
 *         Uses a calibrated inner loop to approximate 1 ms per iteration.
 */

#include "delay.h"   /* Brings in the delay_ms() prototype and uint32_t type */

/**
 * LOOPS_PER_MS (4000):
 *   Approximate number of empty loop iterations that take ~1 ms
 *   when the CPU runs at the default 16 MHz HSI (High-Speed Internal) clock.
 *
 *   Calculation hint:
 *     At 16 MHz, one clock cycle = 62.5 ns.
 *     A simple loop body (increment + compare + branch) ≈ 4 cycles on Cortex-M4.
 *     4 cycles × 62.5 ns = 250 ns per iteration.
 *     1 ms / 250 ns ≈ 4000 iterations.
 *
 *   'UL' suffix ensures the constant is unsigned long, matching uint32_t.
 */
#define LOOPS_PER_MS   4000UL

/**
 * @brief  Blocks the CPU for approximately 'ms' milliseconds.
 * @param  ms  Number of milliseconds to wait.
 *
 * Implementation detail:
 *   - Outer loop runs 'ms' times (once per millisecond).
 *   - Inner loop counts LOOPS_PER_MS iterations to burn ~1 ms of CPU time.
 *   - 'volatile' on 'count' prevents the compiler from optimising away the
 *     empty loop body, since the variable appears to have a side-effect.
 */
void delay_ms(uint32_t ms)
{
    for (uint32_t i = 0; i < ms; i++)          /* Outer: repeat 'ms' times       */
    {
        volatile uint32_t count;               /* Inner loop counter (volatile!)  */
        for (count = 0; count < LOOPS_PER_MS; count++)
            ;  /* Empty body — just burn CPU cycles. The semicolon IS the loop body. */
    }
}
