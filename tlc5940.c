/*

  tlc5940.c

  Copyright 2010-2011 Matthew T. Pandina. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY MATTHEW T. PANDINA "AS IS" AND ANY EXPRESS OR
  IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
  EVENT SHALL MATTHEW T. PANDINA OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
  EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <avr/interrupt.h>

#include "tlc5940.h"

// Define a macro for SPI Transmit
#if (TLC5940_USART_MSPIM)
#define TLC5940_TX(data) do {                                 \
                           while (!(UCSR0A & (1 << UDRE0)));  \
                           UDR0 = (data);                     \
                         } while (0)
#else // TLC5940_USART_MSPIM
#define TLC5940_TX(data) do {                              \
                           SPDR = (data);                  \
                           while (!(SPSR & (1 << SPIF)));  \
                         } while (0)
#endif // TLC5940_USART_MSPIM

#if (TLC5940_ENABLE_MULTIPLEXING)
// In main(), set toggleRows to the two pins that should be toggled
// to turn OFF the previous row's MOSFET, and ON the current row's MOSFET.
uint8_t toggleRows[TLC5940_MULTIPLEX_N];
uint8_t gsData[TLC5940_MULTIPLEX_N][gsDataSize];
static uint8_t gsDataCache[TLC5940_MULTIPLEX_N][gsDataSize];
uint8_t *pBack;
#else // TLC5940_ENABLE_MULTIPLEXING
uint8_t gsData[gsDataSize];
#endif // TLC5940_ENABLE_MULTIPLEXING

#if (TLC5940_USE_GPIOR0 == 0)
volatile uint8_t gsUpdateFlag;
#endif // TLC5940_USE_GPIOR0 == 0

#if (TLC5940_INCLUDE_GAMMA_CORRECT)
#if (TLC5940_PWM_BITS == 12)
#define V 4095.0
#elif (TLC5940_PWM_BITS == 11)
#define V 2047.0
#elif (TLC5940_PWM_BITS == 10)
#define V 1023.0
#elif (TLC5940_PWM_BITS == 9)
#define V 511.0
#elif (TLC5940_PWM_BITS == 8)
#define V 255.0
#else
#error "TLC5940_PWM_BITS must be 8, 9, 10, 11, or 12"
#endif // TLC5940_PWM_BITS
// Maps a linear 8-bit value to a TLC5940_PWM_BITS-bit gamma corrected value
// This array was computer-generated using the following formula:
// for (uint16_t x = 0; x < 256; x++)
//   printf("%e*V+.5, ", (pow((double)x / 255.0, 2.5)));
uint16_t TLC5940_GammaCorrect[] PROGMEM = {
  0.000000e+00*V+.5, 9.630516e-07*V+.5, 5.447842e-06*V+.5, 1.501249e-05*V+.5,
  3.081765e-05*V+.5, 5.383622e-05*V+.5, 8.492346e-05*V+.5, 1.248518e-04*V+.5,
  1.743310e-04*V+.5, 2.340215e-04*V+.5, 3.045437e-04*V+.5, 3.864838e-04*V+.5,
  4.803996e-04*V+.5, 5.868241e-04*V+.5, 7.062682e-04*V+.5, 8.392236e-04*V+.5,
  9.861648e-04*V+.5, 1.147551e-03*V+.5, 1.323826e-03*V+.5, 1.515422e-03*V+.5,
  1.722759e-03*V+.5, 1.946246e-03*V+.5, 2.186282e-03*V+.5, 2.443257e-03*V+.5,
  2.717551e-03*V+.5, 3.009536e-03*V+.5, 3.319578e-03*V+.5, 3.648035e-03*V+.5,
  3.995256e-03*V+.5, 4.361587e-03*V+.5, 4.747366e-03*V+.5, 5.152925e-03*V+.5,
  5.578591e-03*V+.5, 6.024686e-03*V+.5, 6.491527e-03*V+.5, 6.979425e-03*V+.5,
  7.488689e-03*V+.5, 8.019621e-03*V+.5, 8.572521e-03*V+.5, 9.147682e-03*V+.5,
  9.745397e-03*V+.5, 1.036595e-02*V+.5, 1.100963e-02*V+.5, 1.167672e-02*V+.5,
  1.236748e-02*V+.5, 1.308220e-02*V+.5, 1.382115e-02*V+.5, 1.458459e-02*V+.5,
  1.537279e-02*V+.5, 1.618601e-02*V+.5, 1.702451e-02*V+.5, 1.788854e-02*V+.5,
  1.877837e-02*V+.5, 1.969424e-02*V+.5, 2.063640e-02*V+.5, 2.160510e-02*V+.5,
  2.260058e-02*V+.5, 2.362309e-02*V+.5, 2.467286e-02*V+.5, 2.575014e-02*V+.5,
  2.685516e-02*V+.5, 2.798815e-02*V+.5, 2.914934e-02*V+.5, 3.033898e-02*V+.5,
  3.155727e-02*V+.5, 3.280446e-02*V+.5, 3.408077e-02*V+.5, 3.538641e-02*V+.5,
  3.672162e-02*V+.5, 3.808661e-02*V+.5, 3.948159e-02*V+.5, 4.090679e-02*V+.5,
  4.236242e-02*V+.5, 4.384870e-02*V+.5, 4.536583e-02*V+.5, 4.691403e-02*V+.5,
  4.849350e-02*V+.5, 5.010446e-02*V+.5, 5.174710e-02*V+.5, 5.342165e-02*V+.5,
  5.512829e-02*V+.5, 5.686723e-02*V+.5, 5.863868e-02*V+.5, 6.044283e-02*V+.5,
  6.227988e-02*V+.5, 6.415003e-02*V+.5, 6.605348e-02*V+.5, 6.799041e-02*V+.5,
  6.996104e-02*V+.5, 7.196554e-02*V+.5, 7.400411e-02*V+.5, 7.607694e-02*V+.5,
  7.818422e-02*V+.5, 8.032614e-02*V+.5, 8.250289e-02*V+.5, 8.471466e-02*V+.5,
  8.696162e-02*V+.5, 8.924397e-02*V+.5, 9.156189e-02*V+.5, 9.391556e-02*V+.5,
  9.630516e-02*V+.5, 9.873087e-02*V+.5, 1.011929e-01*V+.5, 1.036914e-01*V+.5,
  1.062265e-01*V+.5, 1.087985e-01*V+.5, 1.114074e-01*V+.5, 1.140536e-01*V+.5,
  1.167371e-01*V+.5, 1.194582e-01*V+.5, 1.222169e-01*V+.5, 1.250135e-01*V+.5,
  1.278482e-01*V+.5, 1.307211e-01*V+.5, 1.336324e-01*V+.5, 1.365822e-01*V+.5,
  1.395708e-01*V+.5, 1.425983e-01*V+.5, 1.456648e-01*V+.5, 1.487705e-01*V+.5,
  1.519157e-01*V+.5, 1.551004e-01*V+.5, 1.583249e-01*V+.5, 1.615892e-01*V+.5,
  1.648936e-01*V+.5, 1.682382e-01*V+.5, 1.716232e-01*V+.5, 1.750487e-01*V+.5,
  1.785149e-01*V+.5, 1.820220e-01*V+.5, 1.855701e-01*V+.5, 1.891593e-01*V+.5,
  1.927899e-01*V+.5, 1.964620e-01*V+.5, 2.001758e-01*V+.5, 2.039313e-01*V+.5,
  2.077289e-01*V+.5, 2.115685e-01*V+.5, 2.154504e-01*V+.5, 2.193747e-01*V+.5,
  2.233416e-01*V+.5, 2.273512e-01*V+.5, 2.314038e-01*V+.5, 2.354993e-01*V+.5,
  2.396381e-01*V+.5, 2.438201e-01*V+.5, 2.480457e-01*V+.5, 2.523149e-01*V+.5,
  2.566279e-01*V+.5, 2.609848e-01*V+.5, 2.653858e-01*V+.5, 2.698310e-01*V+.5,
  2.743207e-01*V+.5, 2.788548e-01*V+.5, 2.834336e-01*V+.5, 2.880572e-01*V+.5,
  2.927258e-01*V+.5, 2.974395e-01*V+.5, 3.021985e-01*V+.5, 3.070028e-01*V+.5,
  3.118527e-01*V+.5, 3.167483e-01*V+.5, 3.216896e-01*V+.5, 3.266770e-01*V+.5,
  3.317105e-01*V+.5, 3.367902e-01*V+.5, 3.419163e-01*V+.5, 3.470889e-01*V+.5,
  3.523082e-01*V+.5, 3.575743e-01*V+.5, 3.628874e-01*V+.5, 3.682475e-01*V+.5,
  3.736549e-01*V+.5, 3.791096e-01*V+.5, 3.846119e-01*V+.5, 3.901617e-01*V+.5,
  3.957594e-01*V+.5, 4.014049e-01*V+.5, 4.070985e-01*V+.5, 4.128403e-01*V+.5,
  4.186304e-01*V+.5, 4.244690e-01*V+.5, 4.303562e-01*V+.5, 4.362920e-01*V+.5,
  4.422767e-01*V+.5, 4.483105e-01*V+.5, 4.543933e-01*V+.5, 4.605254e-01*V+.5,
  4.667068e-01*V+.5, 4.729378e-01*V+.5, 4.792185e-01*V+.5, 4.855489e-01*V+.5,
  4.919292e-01*V+.5, 4.983596e-01*V+.5, 5.048401e-01*V+.5, 5.113710e-01*V+.5,
  5.179523e-01*V+.5, 5.245841e-01*V+.5, 5.312666e-01*V+.5, 5.380000e-01*V+.5,
  5.447842e-01*V+.5, 5.516196e-01*V+.5, 5.585062e-01*V+.5, 5.654441e-01*V+.5,
  5.724334e-01*V+.5, 5.794743e-01*V+.5, 5.865670e-01*V+.5, 5.937114e-01*V+.5,
  6.009079e-01*V+.5, 6.081564e-01*V+.5, 6.154571e-01*V+.5, 6.228102e-01*V+.5,
  6.302157e-01*V+.5, 6.376738e-01*V+.5, 6.451846e-01*V+.5, 6.527482e-01*V+.5,
  6.603648e-01*V+.5, 6.680345e-01*V+.5, 6.757574e-01*V+.5, 6.835336e-01*V+.5,
  6.913632e-01*V+.5, 6.992464e-01*V+.5, 7.071833e-01*V+.5, 7.151740e-01*V+.5,
  7.232186e-01*V+.5, 7.313173e-01*V+.5, 7.394701e-01*V+.5, 7.476773e-01*V+.5,
  7.559389e-01*V+.5, 7.642549e-01*V+.5, 7.726257e-01*V+.5, 7.810512e-01*V+.5,
  7.895316e-01*V+.5, 7.980670e-01*V+.5, 8.066575e-01*V+.5, 8.153033e-01*V+.5,
  8.240044e-01*V+.5, 8.327611e-01*V+.5, 8.415733e-01*V+.5, 8.504412e-01*V+.5,
  8.593650e-01*V+.5, 8.683447e-01*V+.5, 8.773805e-01*V+.5, 8.864724e-01*V+.5,
  8.956207e-01*V+.5, 9.048254e-01*V+.5, 9.140865e-01*V+.5, 9.234044e-01*V+.5,
  9.327790e-01*V+.5, 9.422104e-01*V+.5, 9.516989e-01*V+.5, 9.612445e-01*V+.5,
  9.708472e-01*V+.5, 9.805073e-01*V+.5, 9.902249e-01*V+.5, 1.000000e+00*V+.5,
};
#undef V
#endif // TLC5940_INCLUDE_GAMMA_CORRECT

#if (TLC5940_INCLUDE_DC_FUNCS)
uint8_t dcData[dcDataSize];

void TLC5940_SetDC(channel_t channel, uint8_t value) {
  channel = numChannels - 1 - channel;
  channel_t i = (channel3_t)channel * 3 / 4;

  switch (channel % 4) {
  case 0:
    dcData[i] = (dcData[i] & 0x03) | (uint8_t)(value << 2);
    break;
  case 1:
    dcData[i] = (dcData[i] & 0xFC) | (value >> 4);
    i++;
    dcData[i] = (dcData[i] & 0x0F) | (uint8_t)(value << 4);
    break;
  case 2:
    dcData[i] = (dcData[i] & 0xF0) | (value >> 2);
    i++;
    dcData[i] = (dcData[i] & 0x3F) | (uint8_t)(value << 6);
    break;
  default: // case 3:
    dcData[i] = (dcData[i] & 0xC0) | (value);
    break;
  }
}

void TLC5940_SetAllDC(uint8_t value) {
  uint8_t tmp1 = (uint8_t)(value << 2);
  uint8_t tmp2 = (uint8_t)(tmp1 << 2);
  uint8_t tmp3 = (uint8_t)(tmp2 << 2);
  tmp1 |= (value >> 4);
  tmp2 |= (value >> 2);
  tmp3 |= value;

  dcData_t i = 0;
  do {
    dcData[i++] = tmp1;              // bits: 05 04 03 02 01 00 05 04
    dcData[i++] = tmp2;              // bits: 03 02 01 00 05 04 03 02
    dcData[i++] = tmp3;              // bits: 01 00 05 04 03 02 01 00
  } while (i < dcDataSize);
}

#if (TLC5940_INCLUDE_SET4_FUNCS)
// Assumes that outputs 0-3, 4-7, 8-11, 12-15 of the TLC5940 have
// been connected together to sink more current. For a single
// TLC5940, the parameter 'channel' should be in the range 0-3
void TLC5940_Set4DC(channel_t channel, uint8_t value) {
  channel = numChannels - 1 - (channel * 4) - 3;
  channel_t i = (channel3_t)channel * 3 / 4;

  uint8_t tmp1 = (uint8_t)(value << 2);
  uint8_t tmp2 = (uint8_t)(tmp1 << 2);
  uint8_t tmp3 = (uint8_t)(tmp2 << 2);
  tmp1 |= (value >> 4);
  tmp2 |= (value >> 2);
  tmp3 |= value;

  dcData[i++] = tmp1;              // bits: 05 04 03 02 01 00 05 04
  dcData[i++] = tmp2;              // bits: 03 02 01 00 05 04 03 02
  dcData[i] = tmp3;                // bits: 01 00 05 04 03 02 01 00
}
#endif // TLC5940_INCLUDE_SET4_FUNCS

void TLC5940_ClockInDC(void) {
  setHigh(DCPRG_PORT, DCPRG_PIN);
  setHigh(VPRG_PORT, VPRG_PIN);
  dcData_t i;
  for (i = 0; i < dcDataSize; i++)
    TLC5940_TX(dcData[i]);
  pulse(XLAT_PORT, XLAT_PIN);
}
#endif // TLC5940_INCLUDE_DC_FUNCS

void TLC5940_ClockInGS(void) {
  uint8_t firstCycleFlag = 0;

  if (getValue(VPRG_PORT, VPRG_PIN)) {
    setLow(VPRG_PORT, VPRG_PIN);
    firstCycleFlag = 1;
  }
  setLow(BLANK_PORT, BLANK_PIN);
#if (TLC5940_ENABLE_MULTIPLEXING)
  // Loop i over gsDataCache[TLC5940_MULTIPLEX_N - 1][i]
  gsOffset_t offset = (gsOffset_t)gsDataSize * (TLC5940_MULTIPLEX_N - 1);
  gsData_t i = gsDataSize + 1;
  while (--i)
    TLC5940_TX(*(pBack + offset++)); // same as gsDataCache[TLC5940_MULTIPLEX_N - 1][i]
#else // TLC5940_ENABLE_MULTIPLEXING
  gsData_t i;
  for (i = 0; i < gsDataSize; i++)
    TLC5940_TX(gsData[i]);
#endif // TLC5940_ENABLE_MULTIPLEXING
  setHigh(BLANK_PORT, BLANK_PIN);
  pulse(XLAT_PORT, XLAT_PIN);
  if (firstCycleFlag)
    pulse(SCLK_PORT, SCLK_PIN);
}

void TLC5940_Init(void) {
#if (TLC5940_USART_MSPIM)
  // Baud rate must be set to 0 prior to enabling the USART as SPI
  // master, to ensure proper initialization of the XCK line.
  UBRR0 = 0;
#endif // TLC5940_USART_MSPIM
  setOutput(SCLK_DDR, SCLK_PIN);
  setOutput(DCPRG_DDR, DCPRG_PIN);
  setOutput(VPRG_DDR, VPRG_PIN);
  setOutput(XLAT_DDR, XLAT_PIN);
  setOutput(BLANK_DDR, BLANK_PIN);
  setOutput(SIN_DDR, SIN_PIN);

  setLow(SCLK_PORT, SCLK_PIN);
  setLow(DCPRG_PORT, DCPRG_PIN);
  setHigh(VPRG_PORT, VPRG_PIN);
  setLow(XLAT_PORT, XLAT_PIN);
  setHigh(BLANK_PORT, BLANK_PIN);

#if (TLC5940_USE_GPIOR0)
  setLow(TLC5940_FLAGS, TLC5940_FLAG_GS_UPDATE);
  setLow(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE);
#else // TLC5940_USE_GPIOR0
  gsUpdateFlag = 0;
#endif // TLC5940_USE_GPIOR0

#if (TLC5940_ENABLE_MULTIPLEXING)
  // Initialize the write pointer for page-flipping
  pBack = &gsDataCache[0][0];
#endif // TLC5940_ENABLE_MULTIPLEXING

#if (TLC5940_USART_MSPIM)
  // Set USART to Master SPI mode.
  UCSR0C = (1<<UMSEL01) | (1<<UMSEL00);
  // Enable TX only
  UCSR0B = (1 << TXEN0);
  // Set baud rate. Must be set _after_ enabling the transmitter.
  UBRR0 = 0;
#else // TLC5940_USART_MSPIM
  // Enable SPI, Master, set clock rate fck/2
  SPCR = (1 << SPE) | (1 << MSTR);
  SPSR = (1 << SPI2X);
#endif // TLC5940_USART_MSPIM

  // CTC with OCR2A as TOP
  TCCR2A = (1 << WGM01);

  // clk_io/256 (From prescaler)
  TCCR2B = (1 << CS02);

#if (TLC5940_PWM_BITS == 12)
  OCR2A = 15; // Generate an interrupt every 4096 clock cycles
#elif (TLC5940_PWM_BITS == 11)
  OCR2A = 7;  // Generate an interrupt every 2048 clock cycles
#elif (TLC5940_PWM_BITS == 10)
  OCR2A = 3;  // Generate an interrupt every 1024 clock cycles
#elif (TLC5940_PWM_BITS == 9)
  OCR2A = 1;  // Generate an interrupt every 512 clock cycles
#elif (TLC5940_PWM_BITS == 8)
  OCR2A = 0;  // Generate an interrupt every 256 clock cycles
#else
#error "TLC5940_PWM_BITS must be 8, 9, 10, 11, or 12"
#endif // TLC5940_PWM_BITS

  // Enable Timer/Counter0 Compare Match A interrupt
  TIMSK2 |= (1 << OCIE2A);
}

#if (TLC5940_INCLUDE_DEFAULT_ISR)
#if (TLC5940_ENABLE_MULTIPLEXING)
#if (TLC5940_USE_GPIOR0)
// This interrupt will get called every 2^TLC5940_PWM_BITS clock cycles
ISR(TIMER2_COMPA_vect) {
  static uint8_t *pFront = &gsData[0][0]; // read pointer
  static uint8_t row, cycle; // initialized to 0 by default
  setHigh(BLANK_PORT, BLANK_PIN);
  if (getValue(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE)) {
    pulse(XLAT_PORT, XLAT_PIN);
    setLow(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE);
    MULTIPLEX_PIN = toggleRows[row]; // toggle two pins at once
    if (++row == TLC5940_MULTIPLEX_N)
      row = 0;
  }
  setLow(BLANK_PORT, BLANK_PIN);
  // Below we have 2^TLC5940_PWM_BITS cycles to send the data for the next cycle

  gsOffset_t offset = (gsOffset_t)gsDataSize * row;
  gsData_t i = gsDataSize + 1;
  while (--i)
    TLC5940_TX(*(pFront + offset++));

  // Only page-flip if an update is not already in progress
  if (getValue(TLC5940_FLAGS, TLC5940_FLAG_GS_UPDATE) && cycle == 0) {
    uint8_t *tmp = pFront;
    pFront = pBack;
    pBack = tmp;
    setLow(TLC5940_FLAGS, TLC5940_FLAG_GS_UPDATE);
    __asm__ volatile ("" ::: "memory"); // ensure pBack gets re-read
    if (++cycle == TLC5940_MULTIPLEX_N)
      cycle = 0;
  } else if (cycle > 0) {
    if (++cycle == TLC5940_MULTIPLEX_N)
      cycle = 0;
  }
  setHigh(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE);
}
#else // TLC5940_USE_GPIOR0
// This interrupt will get called every 2^TLC5940_PWM_BITS clock cycles
ISR(TIMER2_COMPA_vect) {
  static uint8_t *pFront = &gsData[0][0]; // read pointer
  static uint8_t xlatNeedsPulse; // initialized to 0 by default
  static uint8_t row, cycle; // initialized to 0 by default
  setHigh(BLANK_PORT, BLANK_PIN);
  if (xlatNeedsPulse) {
    pulse(XLAT_PORT, XLAT_PIN);
    xlatNeedsPulse = 0;
    MULTIPLEX_PIN = toggleRows[row]; // toggle two pins at once
    if (++row == TLC5940_MULTIPLEX_N)
      row = 0;
  }
  setLow(BLANK_PORT, BLANK_PIN);
  // Below we have 2^TLC5940_PWM_BITS cycles to send the data for the next cycle

  gsOffset_t offset = (gsOffset_t)gsDataSize * row;
  gsData_t i = gsDataSize + 1;
  while (--i) // loop over gsData[row][i] or gsDataCache[row][i]
    TLC5940_TX(*(pFront + offset++));

  // Only page-flip if an update is not already in progress
  if (gsUpdateFlag && cycle == 0) {
    uint8_t *tmp = pFront;
    pFront = pBack;
    pBack = tmp;
    gsUpdateFlag = 0;
    __asm__ volatile ("" ::: "memory"); // ensure pBack gets re-read
    if (++cycle == TLC5940_MULTIPLEX_N)
      cycle = 0;
  } else if (cycle > 0) {
    if (++cycle == TLC5940_MULTIPLEX_N)
      cycle = 0;
  }
  xlatNeedsPulse = 1;
}
#endif // TLC5940_USE_GPIOR0
#else // TLC5940_ENABLE_MULTIPLEXING
#if (TLC5940_USE_GPIOR0)
// This interrupt will get called every 2^TLC5940_PWM_BITS clock cycles
ISR(TIMER2_COMPA_vect) {
  setHigh(BLANK_PORT, BLANK_PIN);
  if (getValue(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE)) {
    setLow(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE);
    pulse(XLAT_PORT, XLAT_PIN);
  }
  setLow(BLANK_PORT, BLANK_PIN);
  // Below we have 2^TLC5940_PWM_BITS cycles to send the data for the next cycle

  if (getValue(TLC5940_FLAGS, TLC5940_FLAG_GS_UPDATE)) {
    gsData_t i;
    for (i = 0; i < gsDataSize; i++)
      TLC5940_TX(gsData[i]);
    setHigh(TLC5940_FLAGS, TLC5940_FLAG_XLAT_NEEDS_PULSE);
    setLow(TLC5940_FLAGS, TLC5940_FLAG_GS_UPDATE);
  }
}
#else // TLC5940_USE_GPIOR0
// This interrupt will get called every 2^TLC5940_PWM_BITS clock cycles
ISR(TIMER2_COMPA_vect) {
  static uint8_t xlatNeedsPulse; // initialized to 0 by default
  setHigh(BLANK_PORT, BLANK_PIN);
  if (xlatNeedsPulse) {
    xlatNeedsPulse = 0;
    pulse(XLAT_PORT, XLAT_PIN);
  }
  setLow(BLANK_PORT, BLANK_PIN);
  // Below we have 2^TLC5940_PWM_BITS cycles to send the data for the next cycle

  if (gsUpdateFlag) {
    gsData_t i;
    for (i = 0; i < gsDataSize; i++)
      TLC5940_TX(gsData[i]);
    xlatNeedsPulse = 1;
    gsUpdateFlag = 0;
  }
}
#endif // TLC5940_USE_GPIOR0
#endif // TLC5940_ENABLE_MULTIPLEXING
#endif // TLC5940_INCLUDE_DEFAULT_ISR
