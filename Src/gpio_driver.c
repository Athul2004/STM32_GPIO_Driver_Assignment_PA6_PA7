/**
 * @file   gpio_driver.c
 * @brief  GPIO driver implementation for STM32F4xx (bare-metal, no HAL).
 *         Provides functions to initialise a GPIO pin, write / read / toggle
 *         individual pins using direct register access.
 */

#include "gpio_driver.h"  /* Pull in base addresses, offsets, types, and prototypes */

/**
 * REG(base, offset) — helper macro for memory-mapped register access.
 *
 *   (base) + (offset)   → calculates the register's absolute address.
 *   (volatile uint32_t *)  → casts the result to a pointer to a 32-bit register.
 *       'volatile' tells the compiler NOT to optimise away reads/writes
 *       because the hardware value can change at any time.
 *   *(...)               → dereferences the pointer so we can read/write the register
 *                           just like a normal variable.
 */
#define REG(base, offset)  (*(volatile uint32_t *)((base) + (offset)))


/* =====================================================================
 *  GPIO_Init()
 *  PURPOSE: Enable the peripheral clock for the GPIO port and configure
 *           the requested pin as either INPUT or OUTPUT.
 *  WHY:     On STM32, a GPIO port's registers are inaccessible until the
 *           corresponding clock bit in RCC_AHB1ENR is set to 1.
 * ===================================================================== */
void GPIO_Init(GPIO_PinConfig_t *pPinConfig)
{
    /* ----- Step 1: Enable the AHB1 bus clock for the GPIO port ----- */

    if (pPinConfig->pGPIOx_BaseAddr == GPIOA_BASE_ADDR)
        /*
         * Set bit 0 (GPIOAEN) of RCC_AHB1ENR to 1.
         * This turns on the clock for GPIO Port A so that
         * GPIOA registers (MODER, ODR, BSRR …) become functional.
         * Using |= ensures we do not accidentally clear other port clocks.
         */
        REG(RCC_BASE_ADDR, RCC_AHB1ENR_OFFSET) |= (1UL << 0);

    else if (pPinConfig->pGPIOx_BaseAddr == GPIOC_BASE_ADDR)
        /*
         * Set bit 2 (GPIOCEN) of RCC_AHB1ENR to 1.
         * This turns on the clock for GPIO Port C (used for PC13 button).
         */
        REG(RCC_BASE_ADDR, RCC_AHB1ENR_OFFSET) |= (1UL << 2);

    else
        /*
         * If the base address does not match any supported port,
         * exit immediately without modifying any registers.
         * This is a safety guard against invalid configuration.
         */
        return;

    /* ----- Step 2: Configure the pin mode in MODER register ----- */

    /*
     * Read the current MODER register value into a local variable.
     * MODER uses 2 bits per pin:
     *   Bits [2*n+1 : 2*n]  control pin n.
     */
    uint32_t moder = REG(pPinConfig->pGPIOx_BaseAddr, GPIO_MODER_OFFSET);

    /*
     * Clear the 2-bit field for this pin (set it to 00 = input).
     * 0x3UL = 0b11 masks 2 bits.
     * Shifted left by (pinNumber * 2) positions to target the correct field.
     * ~(...) inverts the mask so only those 2 bits are cleared while all
     * other bits remain untouched.
     */
    moder &= ~(0x3UL << (pPinConfig->pinNumber * 2U));

    /*
     * Write the desired mode (INPUT=0 or OUTPUT=1) into the 2-bit field.
     * Cast to uint32_t to ensure proper bit width before shifting.
     * OR-ing preserves the bits we just cleared above if mode is non-zero.
     */
    moder |=  ((uint32_t)(pPinConfig->pinMode) << (pPinConfig->pinNumber * 2U));

    /*
     * Write the modified value back to the MODER register.
     * The pin is now configured as input or output on the hardware.
     */
    REG(pPinConfig->pGPIOx_BaseAddr, GPIO_MODER_OFFSET) = moder;
}


/* =====================================================================
 *  GPIO_WritePin()
 *  PURPOSE: Set a single GPIO output pin to HIGH or LOW.
 *  WHY BSRR: The Bit Set/Reset Register allows atomic pin control.
 *             Writing to bits [15:0]  sets the corresponding pin HIGH.
 *             Writing to bits [31:16] resets the corresponding pin LOW.
 *             No read-modify-write needed → no risk of race conditions.
 * ===================================================================== */
void GPIO_WritePin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber, uint8_t value)
{
    if (value)
        /*
         * Write a 1 to bit position 'pinNumber' in the lower 16 bits of BSRR.
         * This atomically SETS the pin to HIGH (logic 1 / Vdd).
         */
        REG(GPIOx_BaseAddr, GPIO_BSRR_OFFSET) = (1UL << pinNumber);
    else
        /*
         * Write a 1 to bit position 'pinNumber + 16' in the upper 16 bits of BSRR.
         * This atomically RESETS the pin to LOW (logic 0 / GND).
         */
        REG(GPIOx_BaseAddr, GPIO_BSRR_OFFSET) = (1UL << (pinNumber + 16U));
}


/* =====================================================================
 *  GPIO_ReadPin()
 *  PURPOSE: Read the logic level (0 or 1) of a single GPIO input pin.
 *  HOW: The Input Data Register (IDR) is read-only. Each bit reflects
 *       the real-time voltage level on the corresponding pin.
 * ===================================================================== */
uint8_t GPIO_ReadPin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber)
{
    /*
     * 1. REG(..., GPIO_IDR_OFFSET)  → read the 32-bit IDR register.
     * 2. >> pinNumber               → shift the desired bit down to bit-0.
     * 3. & 0x01UL                   → mask out everything except bit-0.
     * 4. (uint8_t)                  → cast to 8-bit return type (0 or 1).
     */
    return (uint8_t)((REG(GPIOx_BaseAddr, GPIO_IDR_OFFSET) >> pinNumber) & 0x01UL);
}


/* =====================================================================
 *  GPIO_TogglePin()
 *  PURPOSE: Flip the current output state of a pin (HIGH→LOW or LOW→HIGH).
 *  HOW: XOR (^=) the pin's bit in the ODR (Output Data Register).
 *       If the bit was 1 it becomes 0, and vice-versa.
 * ===================================================================== */
void GPIO_TogglePin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber)
{
    /*
     * XOR the ODR bit for this pin:
     *   If the pin is currently HIGH (1), XOR with 1 → becomes LOW  (0).
     *   If the pin is currently LOW  (0), XOR with 1 → becomes HIGH (1).
     */
    REG(GPIOx_BaseAddr, GPIO_ODR_OFFSET) ^= (1UL << pinNumber);
}
