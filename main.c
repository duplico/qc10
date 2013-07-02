#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "tlc5940.h"

#define RED 0
#define GREEN 1
#define BLUE 2

#define NUM_RGB_LEDS 4

typedef struct tagRGBFade
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
  uint16_t stepPause;
  uint16_t pause;  
} RGBFade;

float RGBSource[3] = { 0, 0, 0 };
float RGBDest[3];
float RGBInc[3];

const RGBFade rainbow[] =
{
  { 255, 0, 0, 4, 1000 },        // Red
  { 255, 127, 0, 4, 1000 },      // Orange
  { 255, 255, 0, 4, 1000 },      // Yellow
  { 0, 255, 0, 4, 1000 },        // Green
  { 0, 0, 255, 4, 1000 },        // Blue
  { 111, 0, 255, 4, 1000 },      // Indigo
  { 143, 0, 255, 4, 1000 },      // Violet
};

void setupTargetColour( RGBFade target )
{
  RGBDest[RED] = target.r;
  RGBDest[GREEN] = target.g;
  RGBDest[BLUE] = target.b;

  RGBInc[RED] = ( RGBSource[RED] - RGBDest[RED] ) / 256;
  RGBInc[GREEN] = ( RGBSource[GREEN] - RGBDest[GREEN] ) / 256;
  RGBInc[BLUE] = ( RGBSource[BLUE] - RGBDest[BLUE] ) / 256;
}

void fadeTo( uint16_t num_leds )
{
  uint8_t r, g, b;
  uint16_t i, j;

  r = (uint8_t) ( RGBSource[RED] );
  g = (uint8_t) ( RGBSource[GREEN] );
  b = (uint8_t) ( RGBSource[BLUE] );

  for (i = 0; i < num_leds; i++)
  {
    TLC5940_SetGS((i * 3) % numChannels, pgm_read_word(&TLC5940_GammaCorrect[r]));
    TLC5940_SetGS((i * 3 + 1) % numChannels, pgm_read_word(&TLC5940_GammaCorrect[g]));
    TLC5940_SetGS((i * 3 + 2) % numChannels, pgm_read_word(&TLC5940_GammaCorrect[b]));
  }
  
  RGBSource[RED] -= RGBInc[RED];
  RGBSource[GREEN] -= RGBInc[GREEN];
  RGBSource[BLUE] -= RGBInc[BLUE];
}

void startTLC() {

  TLC5940_Init();

#if (TLC5940_INCLUDE_DC_FUNCS)
  TLC5940_SetAllDC(15);
  TLC5940_ClockInDC();
#endif

  // Default all channels to off
  TLC5940_SetAllGS(0);

  // Manually clock in last set of values to be multiplexed
  TLC5940_ClockInGS();

  // Enable Global Interrupts
  sei();  
}

void loopbody() {
  static uint8_t count = 0;
  
  static uint8_t curIndex = 0;
  static uint8_t maxIndex = sizeof( rainbow ) / sizeof( RGBFade );

    if (gsUpdateFlag) return; // Do nothing if we can't update the greyscale values.
    if( count == 0 )
    {
      _delay_ms(rainbow[curIndex].pause);
      setupTargetColour( rainbow[curIndex] );
      curIndex++;
      if( curIndex >= maxIndex)
      {
        curIndex = 0;
      }
    }
    fadeTo( NUM_RGB_LEDS );
    
    TLC5940_SetGSUpdateFlag();
    // usefully wraps at 255.
    count++;
    _delay_ms(rainbow[curIndex].stepPause);
}

int themain(void) {

  startTLC();

  for (;;) {
    loopbody();
  }

  return 0;
}
