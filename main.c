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
#define F_RED 13
#define F_GRN 14
#define F_BLU 15
#define L_SYS 12

#define DC 255

#define O_RED_DC 255
#define O_ORG_DC 255
#define O_YEL_DC 255
#define O_GRN_DC 255
#define O_BLU_DC 255
#define O_PNK_DC 255
#define I_RED_DC 255
#define I_ORG_DC 255
#define I_YEL_DC 255
#define I_GRN_DC 255
#define I_BLU_DC 255
#define I_PNK_DC 255
#define F_RED_DC 255
#define F_GRN_DC 255
#define F_BLU_DC 255
#define L_SYS_DC 255

// Fade scale of 8 and step of 2 = built-in 96ms delay for the optional cross-fade
#define FADE_SCALE 8
#define NUM_LEDS 16
#define QCR_STEP 3
#define QCR_DELAY 5

typedef struct tagQCLights {
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
  
  uint16_t ring_delay;
  uint16_t sys_delay;
  
} QCLights;

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
  
  uint16_t ring_delay;
  
} QCRing;

typedef struct tagQCSys {
  
  uint8_t f_red;
  uint8_t f_grn;
  uint8_t f_blu;
  
  uint8_t l_sys;
  
  uint16_t sys_delay;
  
} QCSys;

float QCRSource[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float QCRDest[16];
float QCRInc[16];

uint8_t control_heartbeat = 0;
uint8_t heartbeat_rate_scale = 16;

QCSys heartbeat00[] = {
  {64, 0, 0, 0, 200},
  {0, 0, 0, 0, 200},
  {64, 0, 0, 0, 200},
  {0, 0, 0, 0, 3000},
};

QCSys heartbeat1[] = {
  {64, 28, 0, 0, 200},
  {0, 0, 0, 0, 200},
  {64, 28, 0, 0, 200},
  {0, 0, 0, 0, 1500},
};

QCSys heartbeat0[] = {
  {64, 48, 0, 0, 200},
  {0, 0, 0, 0, 200},
  {64, 48, 0, 0, 200},
  {0, 0, 0, 0, 1500},
};

#define HB 5
QCSys heartbeats[][5] = {
  {{64, 0, 0, 0, 200}, // Red, .2/3sec
   {0, 0, 0, 0, 200},
   {64, 0, 0, 0, 200},
   {0, 0, 0, 0, 3000}},
   
  {{64, 28, 0, 0, 200}, // Orange, .2/1.5sec
   {0, 0, 0, 0, 200},
   {64, 28, 0, 0, 200},
   {0, 0, 0, 0, 1500}},
   
  {{64, 48, 0, 0, 200}, // Yellow, .2/1.0sec
   {0, 0, 0, 0, 200},
   {64, 48, 0, 0, 200},
   {0, 0, 0, 0, 1000}},
   
  {{0, 64, 0, 0, 150}, // Green, .15/0.8sec
   {0, 0, 0, 0, 150},
   {0, 64, 0, 0, 150},
   {0, 0, 0, 0, 800}},
   
  {{0, 0, 64, 0, 150}, // Blue, .15/0.5sec
   {0, 0, 0, 0, 150},
   {0, 0, 64, 0, 150},
   {0, 0, 0, 0, 500}},
   
  {{40, 0, 64, 0, 150}, // Violet, .125/0.4sec
   {0, 0, 0, 0, 150},
   {40, 0, 64, 0, 150},
   {0, 0, 0, 0, 500}},
   
  {{64, 0, 36, 0, 100}, // Pink, .1/0.25sec
   {0, 0, 0, 0, 100},
   {64, 0, 36, 0, 100},
   {0, 0, 0, 0, 250}},
};

QCRing circle[] = {
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 128, 0, 0, 0, 0,
   128, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 128, 128, 0, 0, 0,
   128, 128, 128, 0, 0, 0,
   QCR_DELAY},
  {128, 128, 128, 128, 0, 0,
   128, 128, 128, 128, 0, 0,
   QCR_DELAY},
  {128, 128, 128, 128, 128, 0,
   128, 128, 128, 128, 128, 0,
   QCR_DELAY},
  {128, 128, 128, 128, 128, 128,
   128, 128, 128, 128, 128, 128,
   QCR_DELAY},
  {0, 128, 128, 128, 128, 128,
   0, 128, 128, 128, 128, 128,
   QCR_DELAY},
  {0, 0, 128, 128, 128, 128,
   0, 0, 128, 128, 128, 128,
   QCR_DELAY},
  {0, 0, 0, 128, 128, 128,
   0, 0, 0, 128, 128, 128,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 128,
   0, 0, 0, 0, 128, 128,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
};

void setupTargetRing(QCRing target) {
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
}

void setupTargetSys(QCSys target) {
  QCRDest[F_RED] = target.f_red;
  QCRDest[F_GRN] = target.f_grn;
  QCRDest[F_BLU] = target.f_blu;
  QCRDest[L_SYS] = target.l_sys;
  
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
    if (QCRSource[i] != QCRDest[i]) {
      QCRSource[i] -= QCRInc[i];
    }
    #else
    QCRSource[i] = QCRDest[i];
    #endif
  }
}

void startTLC() {
  TLC5940_Init();

#if (TLC5940_INCLUDE_DC_FUNCS)
//  TLC5940_SetAllDC(16);
//  TLC5940_ClockInDC();
#endif

  // Default all channels to off
  TLC5940_SetAllGS(0);

  // Manually clock in last set of values to be multiplexed
  TLC5940_ClockInGS();

  // Enable Global Interrupts
  sei();  
}

uint8_t need_to_fade = 0;

uint16_t system_lights_update_loop() {
  static uint8_t count = 0;  
  static uint8_t curIndex = 0;
  static uint8_t maxIndex = sizeof( heartbeats[HB] ) / sizeof( QCSys );
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.

  // Do nothing if we can't update the greyscale values
  if (gsUpdateFlag)
    return 0;

  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading.
  if (count == 0) {
    required_delay_millis += heartbeats[HB][curIndex].sys_delay; // hold this frame
    setupTargetSys(heartbeats[HB][curIndex]); // Sets up all the targets
    // Next frame in the pattern:
    curIndex++;
    // If we just finished the last one, loop.
    if (curIndex >= maxIndex) {
      curIndex = 0;
    }
  }
  need_to_fade = 1;
  count+= FADE_SCALE;
  required_delay_millis += QCR_STEP;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;  
}

uint16_t ring_lights_update_loop() {
  static uint8_t count = 0;  
  static uint8_t curIndex = 0;
  static uint8_t maxIndex = sizeof( circle ) / sizeof( QCRing );
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.

  // Do nothing if we can't update the greyscale values
  if (gsUpdateFlag)
    return 0;

  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading.
  if (count == 0) {
    required_delay_millis += circle[curIndex].ring_delay; // hold this frame
    setupTargetRing(circle[curIndex]); // Sets up all the targets
    // Next frame in the pattern:
    curIndex++;
    // If we just finished the last one, loop.
    if (curIndex >= maxIndex) {
      curIndex = 0;
    }
  }
  need_to_fade = 1;
  count+= FADE_SCALE;
  required_delay_millis += QCR_STEP;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;  
}

void fade_if_necessary() { // TODO: boolean?
  if (gsUpdateFlag || !need_to_fade) return;
  fadeTo();
  TLC5940_SetGSUpdateFlag();
  need_to_fade = 0;
}

uint16_t loopbody() {
}
