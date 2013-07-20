#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "tlc5940.h"
#include "animations.h"


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

float QCRSource[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float QCRDest[16];
float QCRInc[16];

///////////////////////////////////////////////
/////////// if I knew how this stupid arduino /
/////////// thing dealt with header files /////
/////////// the below wouldn't be cluttering //
/////////// up my code... /////////////////////
///////////////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////////////////
/////////// if I knew how this stupid arduino /
/////////// thing dealt with header files /////
/////////// the above wouldn't be cluttering //
/////////// up my code... /////////////////////
///////////////////////////////////////////////

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

void fadeTo(uint8_t fade)
{
  uint16_t i;

  for (i = 0; i < NUM_LEDS; i++) {
    TLC5940_SetGS(i, pgm_read_word(&TLC5940_GammaCorrect[(uint8_t)(QCRSource[i])]));
    if (fade) {
      if (QCRSource[i] != QCRDest[i]) {
        QCRSource[i] -= QCRInc[i];
      }
    }
    else {
      QCRSource[i] = QCRDest[i];
    }
  }
}

void startTLC() {
  TLC5940_Init();

#if (TLC5940_INCLUDE_DC_FUNCS)
  TLC5940_SetAllDC(7);
  TLC5940_ClockInDC();
#endif

  // Default all channels to off
  TLC5940_SetAllGS(0);

  // Manually clock in last set of values to be multiplexed
  TLC5940_ClockInGS();

  // Enable Global Interrupts
  sei();  
}

uint8_t need_to_fade = 0;

uint8_t led_sys_animation = 0;
uint8_t led_sys_count = 0;
uint8_t led_sys_animating = 0;
uint8_t led_sys_looping = 1;
uint8_t led_sys_cur_frame = 0;
uint8_t led_sys_num_frames = 0;

void set_system_lights_animation(uint8_t animation_number, uint8_t looping) {
  led_sys_animation = animation_number;
  led_sys_count = 0;  
  led_sys_animating = 1;
  led_sys_looping = looping;
  led_sys_cur_frame = 0;
  led_sys_num_frames = sizeof( heartbeats[led_sys_animation] ) / sizeof( QCSys );
}

uint16_t system_lights_update_loop() {
  // If we aren't doing anything, don't do anything.
  if (!led_sys_animating || gsUpdateFlag) return 0;
  // TODO: consider returning a long wait if we're not animating.
  
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.  

  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading.
  if (led_sys_count == 0) {
    setupTargetSys(heartbeats[led_sys_animation][led_sys_cur_frame]); // Sets up all the targets
    required_delay_millis += heartbeats[led_sys_animation][led_sys_cur_frame].sys_delay; // hold this frame
    // Next frame in the pattern:
    led_sys_cur_frame++;
    // If we just finished the last one, loop.
    if (led_sys_cur_frame >= led_sys_num_frames) {
      led_sys_cur_frame = 0;
      if (!led_sys_looping) 
        led_sys_animating = 0;
    }
  }
  need_to_fade = 1;
  led_sys_count += FADE_SCALE;
  required_delay_millis += QCR_STEP;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;  
}

uint8_t led_ring_animation = 0;
uint8_t led_ring_count = 0;
uint8_t led_ring_animating = 0;
uint8_t led_ring_looping = 1;
uint16_t led_ring_cur_frame = 0;
uint16_t led_ring_num_frames = 0;

void set_ring_lights_animation(uint8_t animation_number, uint8_t looping) {
  led_ring_animation = animation_number;
  led_ring_count = 0;  
  led_ring_animating = 1;
  led_ring_looping = looping;
  led_ring_cur_frame = 0;
//  led_ring_num_frames = sizeof( ring_animations[led_ring_animation] ) / sizeof( QCRing );
  led_ring_num_frames = ring_anim_lengths[led_ring_animation];
}

uint16_t ring_lights_update_loop() {
  // If we aren't doing anything, don't do anything.
  if (!led_ring_animating || gsUpdateFlag) return 0;
  // TODO: consider returning a long wait if we're not animating.
  
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.  

  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading.
  if (led_ring_count == 0) {
    required_delay_millis += ring_animations[led_ring_animation][led_ring_cur_frame].ring_delay; // hold this frame
    setupTargetRing(ring_animations[led_ring_animation][led_ring_cur_frame]); // Sets up all the targets
    // Next frame in the pattern:
    led_ring_cur_frame++;
    // If we just finished the last one, loop.
    if (led_ring_cur_frame > led_ring_num_frames) {
      led_ring_cur_frame = 0;
      if (!led_ring_looping) {
        led_ring_animating = 0;
      }
    }
  }
  need_to_fade = 1;
  led_ring_count += FADE_SCALE;
  required_delay_millis += QCR_STEP;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;  
}

void fade_if_necessary(uint8_t fade) {
  if (gsUpdateFlag || !need_to_fade) return;
  fadeTo(fade);
  TLC5940_SetGSUpdateFlag();
  need_to_fade = 0;
}
