#ifndef __MAIN_H
#define __MAIN_H

#define PB0 PINB0
#define PB1 PINB1
#define PB2 PINB2
#define PB3 PINB3
#define PB5 PINB5


#define PC0 PINC0
#define PC1 PINC1
#define PC2 PINC3

#define PD1 PIND1
#define PD3 PIND3
#define PD4 PIND4
#define PD5 PIND5
#define PD6 PIND6
#define PD7 PIND7

// regex rules
//
// For defines
// ^{[^\/\#].*}$
// \#define \1
//
//  ={ :z}
//  \1

// ---------- Begin TLC5940 Configuration Section ----------

// Defines the number of TLC5940 chips that are connected in series
#define TLC5940_N 1

// Flag for including functions for manually setting the dot correction
//  0 = Do not include dot correction features (generates smaller code)
//  1 = Include dot correction features (will still read from EEPROM by default)
#define TLC5940_INCLUDE_DC_FUNCS 1

// Flag for including efficient functions for setting the grayscale
// and possibly dot correction values of four channels at once.
//  0 = Do not include functions for ganging outputs in groups of four
//  1 = Include functions for ganging outputs in groups of four
// Note: Any number of outputs can be ganged together at any time by simply
//       connecting them together. These function only provide a more efficient
//       way of setting the values if outputs 0-3, 4-7, 8-11, 12-15, ... are
//       connected together
#define TLC5940_INCLUDE_SET4_FUNCS 0

// Flag for including a default implementation of the TIMER0_COMPA_vect ISR
//  0 = For advanced users only! Only choose this if you want to override the
//      default implementation of the ISR(TIMER0_COMPA_vect) with your own custom
//      implemetation inside main.c
//  1 = Most users should use this setting. Use the default implementation of the
//      TIMER0_COMPA_vect ISR as defined in tlc5940.c
#define TLC5940_INCLUDE_DEFAULT_ISR 1

// Flag for including a gamma correction table stored in the flash memory. When
// driving LEDs, it is helpful to use the full 12-bits of PWM the TLC5940 offers
// to output a 12-bit gamma-corrected value derived from an 8-bit value, since
// the human eye has a non-linear perception of brightness.
//
// For example, calling:
//    TLC5940_SetGS(0, 2047);
// will not make the LED appear half as bright as calling:
//    TLC5940_SetGS(0, 4095);
// However, calling:
//    TLC5940_SetGS(0, pgm_read_word(&TLC5940_GammaCorrect[127]));
// will make the LED appear half as bright as calling:
//    TLC5940_SetGS(0, pgm_read_word(&TLC5940_GammaCorrect[255]));
//
//  0 = Do not store a gamma correction table in flash memory
//  1 = Stores a gamma correction table in flash memory
#define TLC5940_INCLUDE_GAMMA_CORRECT 1

// Flag for forced inlining of the SetGS, SetAllGS, and Set4GS functions.
//  0 = Do not force inline the calls to Set*GS family of functions.
//  1 = Force all calls to the Set*GS family of functions to be inlined. Use this
//      option if execution speed is critical, possibly at the expense of program
//      size, although I have found that forcing these calls to be inlined often
//      results in both smaller and faster code.
#define TLC5940_INLINE_SETGS_FUNCS 1

// Flag to enable multiplexing. This can be used to drive both common cathode
// (preferred), or common anode RGB LEDs, or even single-color LEDs. Use a
// P-Channel MOSFET such as an IRF9520 for each row to be multiplexed.
//  0 = Disable multiplexing; library functions as normal.
//  1 = Enable multiplexing; The gsData array will become two-dimensional, and
//      functions in the Set*GS family require another argument which corresponds
//      to the multiplexed row they operate on.
//#define TLC5940_ENABLE_MULTIPLEXING 1

// The following option only applies if TLC5940_ENABLE_MULTIPLEXING = 1
#if TLC5940_ENABLE_MULTIPLEXING
// Defines the number of rows to be multiplexed.
// Note: Without writing a custom ISR, that can toggle pins from multiple PORT
//       registers, the maximum number of rows that can be multiplexed is eight.
//       This option is ignored if TLC5940_ENABLE_MULTIPLEXING = 0
#define TLC5940_MULTIPLEX_N 3
#endif

// Flag to use the USART in MSPIM mode, rather than use the SPI Master bus to
// communicate with the TLC5940. One major advantage of using the USART in MSPIM
// mode is that the  transmit register is double-buffered, so you can send data
// to the TLC5940 much faster. Refer to schematics ending in _usart_mspim for
// details on how to connect the hardware before enabling this mode.
//  0 = Use normal SPI Master mode to communicate with TLC5940 (slower)
//  1 = Use the USART in double-buffered MSPIM mode to communicate with the
//      TLC5940 (faster, but requires the use of different hardware pins)
// WARNING: Before you enable this option, you must wire the chip up differently!
#define TLC5940_USART_MSPIM 1

// Defines the number of bits used to define a single PWM cycle. The default
// is 12, but it may be lowered to achieve faster refreshes, at the expense
// of the ISR being called more frequently. If TLC5940_INCLUDE_GAMMA_CORRECT = 1
// then changing TLC5940_PWM_BITS will automatically rescale the gamma correction
// table to use the appropriate maximum value, at the expense of precision.
//  12 = Normal 12-bit PWM mode. Possible output values between 0-4095
//  11 = 11-bit PWM mode. Possible output values between 0-2047
//  10 = 10-bit PWM mode. Possible output values between 0-1023
//   9 =  9-bit PWM mode. Possible output values between 0-511
//   8 =  8-bit PWM mode. Possible output values between 0-255
// Note: Lowering this value will decrease the amount of time you have in the
//       ISR to send the TLC5940 updated values, potentially limiting the
//       number of devices you can connect in series, and it will decrease the
//       number of cycles available to main(), since the ISR will be called
//       more often. Lowering this value will however, reduce flickering and
//       will allow for much quicker updates.
#define TLC5940_PWM_BITS 12

// Determines whether or not GPIOR0 is used to store flags. This special-purpose
// register is designed to store bit flags, as it can set, clear or test a
// single bit in only 2 clock cycles.
//
// Note: If enabled, you must make sure that the flag bits assigned below do not
//       conflict with any other GPIOR0 flag bits your application might use.
#define TLC5940_USE_GPIOR0 1

// GPIOR0 flag bits used
#ifdef TLC5940_USE_GPIOR0
#define TLC5940_FLAG_GS_UPDATE 0
#define TLC5940_FLAG_XLAT_NEEDS_PULSE 1
#endif

// BLANK is only configurable if the TLC5940 is using the USART in MSPIM mode
#ifdef TLC5940_USART_MSPIM
#define BLANK_DDR DDRD
#define BLANK_PORT PORTD
#define BLANK_PIN PD6
#endif

// DDR, PORT, and PIN connected to DCPRG
#define DCPRG_DDR DDRD
#define DCPRG_PORT PORTD
// DCPRG is always configurable, but the default pin needs to change if
// the TLC5940 is using USART MSPIM mode, because PD4 is needed for XCK
#ifdef TLC5940_USART_MSPIM
#define DCPRG_PIN PD3
#else
#define DCPRG_PIN PD4
#endif

// DDR, PORT, and PIN connected to VPRG
#define VPRG_DDR DDRD
#define VPRG_PORT PORTD
#define VPRG_PIN PD7

// DDR, PORT, and PIN connected to XLAT
#ifdef TLC5940_USART_MSPIM
#define XLAT_DDR DDRD
#define XLAT_PORT PORTD
#define XLAT_PIN PD5
#else
#define XLAT_DDR DDRB
#define XLAT_PORT PORTB
#define XLAT_PIN PB1
#endif

// The following options only apply if TLC5940_ENABLE_MULTIPLEXING = 1
#ifdef TLC5940_ENABLE_MULTIPLEXING
// DDR, PORT, and PIN registers used for driving the multiplexing IRF9520 MOSFETs
// Note: All pins used for multiplexing must share the same DDR, PORT, and PIN
//       registers. These options are ignored if TLC5940_ENABLE_MULTIPLEXING = 0
#define MULTIPLEX_DDR DDRC
#define MULTIPLEX_PORT PORTC
#define MULTIPLEX_PIN PINC

// List of PIN names of pins that are connected to the multiplexing IRF9520
// MOSFETs. You can define up to eight unless you use a custom ISR that can
// toggle PINs on multiple PORTs.
// Note: All pins used for multiplexing must share the same DDR, PORT, and PIN
//       registers. These options are ignored if TLC5940_ENABLE_MULTIPLEXING = 0
// Also: If you add any pins here, do not forget to add those variables to the
//       MULTIPLEXING_DEFINES flag below!
#define R_PIN PC0
#define G_PIN PC1
#define B_PIN PC2

#endif

// ---------- End TLC5940 Configuration Section ----------
#include "animations.h"
uint8_t set_system_lights_animation(uint8_t, uint8_t, uint8_t);
uint8_t set_ring_lights_animation(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);
uint8_t set_ring_lights_blink(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, uint8_t, QCRing, uint16_t);
void ring_stop_animating();
uint16_t system_lights_update_loop();
uint16_t ring_lights_update_loop();
uint16_t uber_ring_fade();
void startTLC();
extern uint8_t led_ring_animating;
extern uint8_t led_sys_animating;

#endif
