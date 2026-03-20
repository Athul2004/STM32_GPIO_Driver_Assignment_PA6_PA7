/**
 * @file   gpio_driver.h
 * @brief  GPIO driver header for STM32F4xx (Cortex-M4).
 *         Defines base addresses, register offsets, pin numbers,
 *         pin modes, configuration structure, and function prototypes
 *         needed to configure and control GPIO pins without HAL.
 */

#ifndef GPIO_DRIVER_H_   /* Include guard: prevents this header from being included more than once */
#define GPIO_DRIVER_H_   /* Defines the include guard macro so subsequent includes are skipped      */

#include <stdint.h>      /* Provides fixed-width integer types (uint8_t, uint32_t, etc.)            */

/* =====================================================================
 *  Memory-Mapped Peripheral Base Addresses (from STM32F446RE datasheet)
 * ===================================================================== */

/**
 * GPIOA_BASE_ADDR (0x40020000):
 *   Starting address of the GPIOA peripheral register block.
 *   All GPIOA registers (MODER, IDR, ODR, BSRR …) live at offsets from here.
 *   Purpose: PA5 (onboard LED), PA6, PA7 (external LEDs) belong to Port A.
 */
#define GPIOA_BASE_ADDR       0x40020000UL

/**
 * GPIOC_BASE_ADDR (0x40020800):
 *   Starting address of the GPIOC peripheral register block.
 *   Purpose: PC13 (user push-button on Nucleo board) belongs to Port C.
 */
#define GPIOC_BASE_ADDR       0x40020800UL

/**
 * RCC_BASE_ADDR (0x40023800):
 *   Starting address of the Reset and Clock Control (RCC) register block.
 *   Purpose: We need to enable the AHB1 bus clock for GPIO ports before
 *            we can read or write any GPIO register.
 */
#define RCC_BASE_ADDR         0x40023800UL

/* =====================================================================
 *  GPIO Register Offsets (relative to GPIOx base address)
 * ===================================================================== */

/**
 * GPIO_MODER_OFFSET (0x00):
 *   Offset of the Mode Register.  Each pin uses 2 bits to select its mode:
 *     00 = Input, 01 = Output, 10 = Alternate function, 11 = Analog.
 *   Purpose: Configure a pin as input or output.
 */
#define GPIO_MODER_OFFSET     0x00UL

/**
 * GPIO_IDR_OFFSET (0x10):
 *   Offset of the Input Data Register (read-only).
 *   Each bit reflects the current logic level on the corresponding pin.
 *   Purpose: Read the state of an input pin (e.g., button).
 */
#define GPIO_IDR_OFFSET       0x10UL

/**
 * GPIO_ODR_OFFSET (0x14):
 *   Offset of the Output Data Register.
 *   Each bit controls the output level of the corresponding pin.
 *   Purpose: Toggle a pin by XOR-ing the relevant bit.
 */
#define GPIO_ODR_OFFSET       0x14UL

/**
 * GPIO_BSRR_OFFSET (0x18):
 *   Offset of the Bit Set/Reset Register (write-only).
 *   Bits [15:0]  → Set the corresponding pin HIGH (write 1).
 *   Bits [31:16] → Reset the corresponding pin LOW  (write 1).
 *   Purpose: Atomically set or clear a single pin without a
 *            read-modify-write cycle (avoids race conditions).
 */
#define GPIO_BSRR_OFFSET      0x18UL

/**
 * RCC_AHB1ENR_OFFSET (0x30):
 *   Offset of the AHB1 peripheral clock enable register inside RCC.
 *   Bit 0 = GPIOAEN (enable clock for Port A).
 *   Bit 2 = GPIOCEN (enable clock for Port C).
 *   Purpose: A GPIO port does not work until its clock is enabled here.
 */
#define RCC_AHB1ENR_OFFSET    0x30UL

/* =====================================================================
 *  Pin Number Definitions
 *  Each value is the bit position inside the 16-bit port.
 * ===================================================================== */

#define GPIO_PIN_5            5U    /* PA5: connected to onboard LD2 LED on Nucleo-F446RE  */
#define GPIO_PIN_6            6U    /* PA6: connected to external LED 1                     */
#define GPIO_PIN_7            7U    /* PA7: connected to external LED 2                     */
#define GPIO_PIN_13           13U   /* PC13: connected to the blue user push-button (B1)    */

/* =====================================================================
 *  GPIO Mode Constants (2-bit values written into MODER register)
 * ===================================================================== */

#define GPIO_MODE_INPUT       0x00U /* 00 → pin is a digital input                          */
#define GPIO_MODE_OUTPUT      0x01U /* 01 → pin is a general-purpose push-pull output        */

/* =====================================================================
 *  GPIO Pin Configuration Structure
 *  Passed to GPIO_Init() to configure one pin at a time.
 * ===================================================================== */
typedef struct
{
    uint32_t pGPIOx_BaseAddr; /* Base address of the GPIO port (e.g., GPIOA_BASE_ADDR)        */
    uint8_t  pinNumber;       /* Pin number within the port (0–15), e.g., GPIO_PIN_5          */
    uint8_t  pinMode;         /* Desired mode for the pin (GPIO_MODE_INPUT / GPIO_MODE_OUTPUT)*/
} GPIO_PinConfig_t;

/* =====================================================================
 *  Function Prototypes
 * ===================================================================== */

/**
 * @brief  Enables the port clock and sets the pin mode (input / output).
 * @param  pPinConfig  Pointer to a filled-in GPIO_PinConfig_t structure.
 */
void    GPIO_Init(GPIO_PinConfig_t *pPinConfig);

/**
 * @brief  Sets or clears a single output pin using the BSRR register.
 * @param  GPIOx_BaseAddr  Base address of the GPIO port.
 * @param  pinNumber       Pin number (0–15).
 * @param  value           0 = LOW, non-zero = HIGH.
 */
void    GPIO_WritePin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber, uint8_t value);

/**
 * @brief  Reads the current logic level of a single input pin via IDR.
 * @param  GPIOx_BaseAddr  Base address of the GPIO port.
 * @param  pinNumber       Pin number (0–15).
 * @return 0 or 1 representing the pin state.
 */
uint8_t GPIO_ReadPin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber);

/**
 * @brief  Toggles the output state of a pin by XOR-ing the ODR bit.
 * @param  GPIOx_BaseAddr  Base address of the GPIO port.
 * @param  pinNumber       Pin number (0–15).
 */
void    GPIO_TogglePin(uint32_t GPIOx_BaseAddr, uint8_t pinNumber);

#endif /* GPIO_DRIVER_H_ */
