#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "tlc5940.h"

#define FADE 0

#define O_RED 0
#define O_ORG 1
#define O_YEL 2
#define O_GRN 3
#define O_BLU 4
#define O_PNK 5
#define I_RED 6
#define I_ORG 7
#define I_YEL 8
#define I_GRN 9
#define I_BLU 10
#define I_PNK 11
#define F_RED 12
#define F_GRN 13
#define F_BLU 14
#define L_SYS 15

#define FADE_SCALE 8
#define NUM_LEDS 16
#define QCR_STEP 5
#define QCR_DELAY 5

typedef struct tagQCRing {
  uint8_t o_red;
  uint8_t o_org;
  uint8_t o_yel;
  uint8_t o_grn;
  uint8_t o_blu;
  uint8_t o_pnk;

  uint8_t i_red;
  uint8_t i_org;
  uint8_t i_yel;
  uint8_t i_grn;
  uint8_t i_blu;
  uint8_t i_pnk;
  
  uint8_t f_red;
  uint8_t f_grn;
  uint8_t f_blu;
  
  uint8_t l_sys;
  
  uint16_t step_delay;
  uint16_t pattern_delay;
  
} QCRing;

float QCRSource[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float QCRDest[16];
float QCRInc[16];

const QCRing circle[] = {
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {128, 128, 0, 0, 0, 0,
   128, 128, 0, 0, 0, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {128, 128, 128, 0, 0, 0,
   128, 128, 128, 0, 0, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {128, 128, 128, 128, 0, 0,
   128, 128, 128, 128, 0, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {128, 128, 128, 128, 128, 0,
   128, 128, 128, 128, 128, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {128, 128, 128, 128, 128, 128,
   128, 128, 128, 128, 128, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 128, 128, 128, 128, 128,
   0, 128, 128, 128, 128, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 0, 128, 128, 128, 128,
   0, 0, 128, 128, 128, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 0, 0, 128, 128, 128,
   0, 0, 0, 128, 128, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 128,
   0, 0, 0, 0, 128, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, QCR_STEP,
   QCR_DELAY},
};

void setupTargetColour( QCRing target )
{
  QCRDest[O_RED] = target.o_red;
  QCRDest[O_ORG] = target.o_org;
  QCRDest[O_YEL] = target.o_yel;
  QCRDest[O_GRN] = target.o_grn;
  QCRDest[O_BLU] = target.o_blu;
  QCRDest[O_PNK] = target.o_pnk;
  QCRDest[I_RED] = target.i_red;
  QCRDest[I_ORG] = target.i_org;
  QCRDest[I_YEL] = target.i_yel;
  QCRDest[I_GRN] = target.i_grn;
  QCRDest[I_BLU] = target.i_blu;
  QCRDest[I_PNK] = target.i_pnk;
  QCRDest[F_RED] = target.f_red;
  QCRDest[F_GRN] = target.f_grn;
  QCRDest[F_BLU] = target.f_blu;
  QCRDest[L_SYS] = target.l_sys;

  QCRInc[O_RED] = (QCRSource[O_RED] - QCRDest[O_RED]) / (256 / FADE_SCALE);
  QCRInc[O_ORG] = (QCRSource[O_ORG] - QCRDest[O_ORG]) / (256 / FADE_SCALE);
  QCRInc[O_YEL] = (QCRSource[O_YEL] - QCRDest[O_YEL]) / (256 / FADE_SCALE);
  QCRInc[O_GRN] = (QCRSource[O_GRN] - QCRDest[O_GRN]) / (256 / FADE_SCALE);
  QCRInc[O_BLU] = (QCRSource[O_BLU] - QCRDest[O_BLU]) / (256 / FADE_SCALE);
  QCRInc[O_PNK] = (QCRSource[O_PNK] - QCRDest[O_PNK]) / (256 / FADE_SCALE);
  QCRInc[I_RED] = (QCRSource[I_RED] - QCRDest[I_RED]) / (256 / FADE_SCALE);
  QCRInc[I_ORG] = (QCRSource[I_ORG] - QCRDest[I_ORG]) / (256 / FADE_SCALE);
  QCRInc[I_YEL] = (QCRSource[I_YEL] - QCRDest[I_YEL]) / (256 / FADE_SCALE);
  QCRInc[I_GRN] = (QCRSource[I_GRN] - QCRDest[I_GRN]) / (256 / FADE_SCALE);
  QCRInc[I_BLU] = (QCRSource[I_BLU] - QCRDest[I_BLU]) / (256 / FADE_SCALE);
  QCRInc[I_PNK] = (QCRSource[I_PNK] - QCRDest[I_PNK]) / (256 / FADE_SCALE);
  QCRInc[F_RED] = (QCRSource[F_RED] - QCRDest[F_RED]) / (256 / FADE_SCALE);
  QCRInc[F_GRN] = (QCRSource[F_GRN] - QCRDest[F_GRN]) / (256 / FADE_SCALE);
  QCRInc[F_BLU] = (QCRSource[F_BLU] - QCRDest[F_BLU]) / (256 / FADE_SCALE);
  QCRInc[L_SYS] = (QCRSource[L_SYS] - QCRDest[L_SYS]) / (256 / FADE_SCALE);
}

void fadeTo()
{
  uint16_t i;

  for (i = 0; i < NUM_LEDS; i++) {
    TLC5940_SetGS(i, pgm_read_word(&TLC5940_GammaCorrect[(uint8_t)(QCRSource[i])]));
    #if FADE
    QCRSource[i] -= QCRInc[i];
    #else
    QCRSource[i] = QCRDest[i];
    #endif
  }
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

uint16_t loopbody() {
  static uint8_t count = 0;  
  static uint8_t curIndex = 0;
  static uint8_t maxIndex = sizeof( circle ) / sizeof( QCRing );
  
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.

  // Do nothing if we can't update the greyscale values
  if (gsUpdateFlag)
    return;

  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend 255 loop iterations executing the fade.
  if (count == 0) {
    setupTargetColour(circle[curIndex]); // Sets up all the targets
    // Next LED settings:
    curIndex++;
    // Loop the animation:
    if (curIndex >= maxIndex) {
      curIndex = 0;
    }
  }
  fadeTo();
  TLC5940_SetGSUpdateFlag(); // Need to change the lights
  count+= FADE_SCALE;
  if (count==0) { // count overflowed
    required_delay_millis += circle[curIndex].pattern_delay;
  }
  required_delay_millis += circle[curIndex].step_delay;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;
//  return 0;
}
