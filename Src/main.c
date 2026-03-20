/**
 * @file   main.c
 * @brief  Application entry point for the STM32 GPIO Driver Assignment.
 *
 *         Functionality:
 *           1. PA5  (onboard LED)   — toggled by pressing the user button (PC13).
 *           2. PA6 & PA7 (external) — blink alternately every 250 ms.
 *
 *         All GPIO access goes through the bare-metal gpio_driver API.
 */

#include <stdint.h>        /* Fixed-width integer types (uint8_t, uint32_t, etc.)       */
#include "gpio_driver.h"   /* GPIO driver: base addresses, config struct, init/write/read/toggle */
#include "delay.h"         /* Busy-wait delay function (delay_ms)                       */

/* --------------- Application Timing Constants --------------- */

/**
 * DEBOUNCE_MS (50):
 *   Time in milliseconds to wait after detecting a button press before
 *   re-reading the button.  Mechanical switches "bounce" (rapidly open and
 *   close) for a few milliseconds, producing false triggers.  Waiting 50 ms
 *   lets the contact settle so we read a stable value.
 */
#define DEBOUNCE_MS    50

/**
 * BLINK_MS (250):
 *   Duration in milliseconds that each external LED stays ON during the
 *   alternating blink pattern.  250 ms ON + 250 ms OFF = 2 Hz blink rate.
 */
#define BLINK_MS       250

/* --------------- Forward declarations of helper functions --------------- */

static void setup_pins(void);     /* Configures all GPIO pins used in the project   */
static void handle_button(void);  /* Reads button, debounces, toggles onboard LED   */
static void alternate_leds(void); /* Alternates PA6 and PA7 external LEDs           */


/* =====================================================================
 *  main()
 *  PURPOSE: Program entry point — initialise hardware then loop forever,
 *           checking the button and blinking the external LEDs.
 * ===================================================================== */
int main(void)
{
    setup_pins();   /* One-time hardware initialisation (clocks + pin modes + LED off) */

    while (1)       /* Infinite super-loop — embedded systems never return from main() */
    {
        handle_button();   /* Check if user pressed button → toggle PA5 LED */
        alternate_leds();  /* Blink PA6 and PA7 alternately (blocks ~500 ms) */
    }
}


/* =====================================================================
 *  setup_pins()
 *  PURPOSE: Configure every GPIO pin used in this project.
 *           • PA5, PA6, PA7  → output mode (LEDs)
 *           • PC13           → input mode  (button)
 *           Also ensures all LEDs start in the OFF state.
 * ===================================================================== */
static void setup_pins(void)
{
    GPIO_PinConfig_t cfg;  /* Reusable configuration structure */

    /* --- PA5: onboard LD2 LED (green) on Nucleo-F446RE --- */
    cfg.pGPIOx_BaseAddr = GPIOA_BASE_ADDR;  /* Target Port A                      */
    cfg.pinNumber       = GPIO_PIN_5;        /* Pin 5                              */
    cfg.pinMode         = GPIO_MODE_OUTPUT;  /* General-purpose push-pull output    */
    GPIO_Init(&cfg);
    /*
     * GPIO_Init() does two things here:
     *   1. Enables the GPIOA clock in RCC_AHB1ENR (bit 0 = GPIOAEN).
     *   2. Writes 01 (output) into MODER bits [11:10] for PA5.
     */

    /* --- PA6: external LED 1 --- */
    cfg.pinNumber = GPIO_PIN_6;   /* Only change the pin; port and mode stay the same */
    GPIO_Init(&cfg);
    /*
     * Clock is already enabled from PA5 init; GPIO_Init() sets it again
     * harmlessly (OR-ing a bit that is already 1).
     * Writes 01 into MODER bits [13:12] for PA6.
     */

    /* --- PA7: external LED 2 --- */
    cfg.pinNumber = GPIO_PIN_7;   /* Switch to pin 7, still Port A output */
    GPIO_Init(&cfg);
    /* Writes 01 into MODER bits [15:14] for PA7. */

    /* --- PC13: user push-button (active-low, directly to GND when pressed) --- */
    cfg.pGPIOx_BaseAddr = GPIOC_BASE_ADDR;  /* Switch to Port C                    */
    cfg.pinNumber       = GPIO_PIN_13;       /* Pin 13                              */
    cfg.pinMode         = GPIO_MODE_INPUT;   /* Digital input (button reading)       */
    GPIO_Init(&cfg);
    /*
     * GPIO_Init() now:
     *   1. Enables GPIOC clock in RCC_AHB1ENR (bit 2 = GPIOCEN).
     *   2. Writes 00 (input) into MODER bits [27:26] for PC13.
     */

    /* --- Ensure all LEDs are OFF at startup --- */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_5, 0);  /* PA5 LED OFF (BSRR reset) */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_6, 0);  /* PA6 LED OFF              */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_7, 0);  /* PA7 LED OFF              */
}


/* =====================================================================
 *  handle_button()
 *  PURPOSE: Detect a user button press on PC13, debounce it, and toggle
 *           the onboard LED on PA5 exactly once per press.
 *
 *  LOGIC:
 *   - Button is active-low: pressed = 0, released = 1.
 *   - If not pressed, return immediately (no delay overhead).
 *   - If pressed, wait DEBOUNCE_MS then re-check to filter out noise.
 *   - Toggle PA5 via XOR on ODR.
 *   - Wait for the user to release the button before returning,
 *     so the LED does not toggle multiple times per press.
 * ===================================================================== */
static void handle_button(void)
{
    /* Read PC13: if pin is HIGH (1), button is NOT pressed → do nothing */
    if (GPIO_ReadPin(GPIOC_BASE_ADDR, GPIO_PIN_13) != 0)
        return;

    /* --- First press detected (pin LOW) → debounce --- */
    delay_ms(DEBOUNCE_MS);  /* Wait 50 ms for contact bounce to settle */

    /* Re-read PC13 after the debounce delay */
    if (GPIO_ReadPin(GPIOC_BASE_ADDR, GPIO_PIN_13) != 0)
        return;  /* It was just noise/bounce; button is actually released */

    /* --- Confirmed press → toggle onboard LED (PA5) --- */
    GPIO_TogglePin(GPIOA_BASE_ADDR, GPIO_PIN_5);
    /*
     * XOR on ODR bit 5:
     *   LED was ON  → turns OFF.
     *   LED was OFF → turns ON.
     */

    /* --- Wait for button release (active-low: wait until pin goes HIGH) --- */
    while (GPIO_ReadPin(GPIOC_BASE_ADDR, GPIO_PIN_13) == 0)
        delay_ms(10);   /* Poll every 10 ms to avoid hammering the bus */

    delay_ms(DEBOUNCE_MS);  /* Extra debounce after release edge */
}


/* =====================================================================
 *  alternate_leds()
 *  PURPOSE: Blink PA6 and PA7 in an alternating pattern.
 *           First PA6 ON / PA7 OFF for 250 ms,
 *           then  PA6 OFF / PA7 ON for 250 ms.
 *           Total cycle time = 500 ms → 2 Hz blink frequency.
 * ===================================================================== */
static void alternate_leds(void)
{
    /* Phase 1: PA6 ON, PA7 OFF */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_6, 1);  /* Set PA6 HIGH via BSRR lower half  */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_7, 0);  /* Reset PA7 LOW via BSRR upper half */
    delay_ms(BLINK_MS);  /* Hold this state for 250 ms */

    /* Phase 2: PA6 OFF, PA7 ON */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_6, 0);  /* Reset PA6 LOW  */
    GPIO_WritePin(GPIOA_BASE_ADDR, GPIO_PIN_7, 1);  /* Set PA7 HIGH   */
    delay_ms(BLINK_MS);  /* Hold this state for 250 ms */
}
