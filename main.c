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
#define FADE_SCALE 4
#define NUM_LEDS 16

float QCRSource[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
float QCRDest[16];
float QCRInc[16];
uint8_t led_sys_animation = 0;
uint8_t led_sys_count = 0;
uint8_t led_sys_animating = 0;
uint8_t led_sys_looping = 1;
uint8_t led_sys_cur_frame = 0;
uint8_t led_sys_num_frames = 0;
uint8_t led_sys_crossfade_step = 0;
uint8_t led_ring_animation = 0;
uint8_t led_ring_count = 0;
uint8_t led_ring_animating = 0;
uint8_t led_ring_looping = 1;
uint16_t led_ring_cur_frame = 0;
uint16_t led_ring_num_frames = 0;
uint8_t led_ring_crossfade = 0;
uint8_t led_ring_crossfade_step = 0;
uint8_t led_ring_uberfade = 0;
uint8_t led_ring_blinking = 0;
uint16_t led_ring_blink_count = 0;
QCRing current_ring;
QCRing next_ring;

#define UBER_FADEOUT_INC 0.08 * FADE_SCALE
#define UBER_FADEOUT_DELAY 3

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

void sys_fade(uint8_t fade)
{
  uint16_t i;

  for (i = L_SYS; i < NUM_LEDS; i++) {
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

uint8_t set_system_lights_animation(uint8_t animation_number, uint8_t looping, uint8_t crossfade_step) {
  led_sys_animation = animation_number;
  led_sys_count = 0;  
  led_sys_animating = 1;
  led_sys_looping = looping;
  led_sys_cur_frame = -1; //255? // Make it overflow first thing.
  led_sys_crossfade_step = crossfade_step;
  led_sys_num_frames = sizeof( heartbeats[led_sys_animation] ) / sizeof( QCSys );
  return 0;
}

uint16_t system_lights_update_loop() {
  // If we aren't doing anything, don't do anything.
  if (!led_sys_animating || gsUpdateFlag) return 0;
  
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.  
  if (led_sys_count == 256 - FADE_SCALE) {
    required_delay_millis += heartbeats[led_sys_animation][led_sys_cur_frame].sys_delay; // hold this frame
  }
  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading.
  if (led_sys_count == 0) {
    // Next frame in the pattern:
    led_sys_cur_frame++;
    // If we just finished the last one, loop.
    if (led_sys_cur_frame >= led_sys_num_frames) {
      led_sys_cur_frame = 0;
      if (!led_sys_looping) 
        led_sys_animating = 0;
    }
    setupTargetSys(heartbeats[led_sys_animation][led_sys_cur_frame]); // Sets up all the targets
  }
  sys_fade(led_sys_crossfade_step != 0);
  TLC5940_SetGSUpdateFlag();
  led_sys_count += FADE_SCALE;
  required_delay_millis += led_sys_crossfade_step;
  
  // Return how long we should wait until this is called again.
  return required_delay_millis;
}

void ring_fade(uint8_t fade) {
  uint16_t i;
  for (i = 0; i < L_SYS; i++) {
    if (led_ring_uberfade && QCRDest[i] == 0)
      continue; // If we're uber, skip lights that we're turning off.
    TLC5940_SetGS(i, pgm_read_word(&TLC5940_GammaCorrect[(uint8_t)(QCRSource[i])]));
    TLC5940_SetGSUpdateFlag();
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

uint16_t uber_ring_fade() {
  if (!led_ring_uberfade) return current_ring.ring_delay;
  uint16_t i, j;
  for (i = 0; i < L_SYS; i++) {
    if (QCRDest[i] == 0 || !led_ring_animating) {
      TLC5940_SetGS(i, pgm_read_word(&TLC5940_GammaCorrect[(uint8_t)(QCRSource[i])]));
      TLC5940_SetGSUpdateFlag();
      if (QCRSource[i] != QCRDest[i]) {
        QCRSource[i] -= UBER_FADEOUT_INC;
        if (QCRSource[i] < 0) QCRSource[i] = 0;
      }
    }
  }
  return UBER_FADEOUT_DELAY;
}

uint8_t set_ring_lights_animation(uint8_t animation_number, uint8_t looping, uint8_t crossfade,
                               uint8_t crossfade_step, uint8_t num_frames, uint8_t uberfade) {
  led_ring_animation = animation_number;
  led_ring_count = 0;
  led_ring_animating = 1;
  led_ring_looping = looping;
  led_ring_cur_frame = 0;
  led_ring_crossfade = crossfade;
  led_ring_crossfade_step = crossfade_step;
  led_ring_blinking = 0;
  led_ring_blink_count = 0;
  memcpy_P(&current_ring, &ring_animations[led_ring_animation][led_ring_cur_frame], sizeof(QCRing));
  if (num_frames == 0)
    led_ring_num_frames = ring_anim_lengths[led_ring_animation];
  else
    led_ring_num_frames = num_frames;
  led_ring_uberfade = uberfade;
  return 0;
}

uint8_t set_ring_lights_blink(uint8_t animation_number, uint8_t looping, uint8_t crossfade,
                              uint8_t crossfade_step, uint8_t num_frames, uint8_t uberfade,
                              QCRing blink_ring, uint16_t blink_count) {
  set_ring_lights_animation(animation_number, looping, crossfade,
                            crossfade_step, num_frames, uberfade);
  led_ring_blinking = 1;
  led_ring_blink_count = blink_count;
  next_ring = blink_ring;
  return 0;
}

void ring_stop_animating() {
  led_ring_animating = 0;
}

uint16_t ring_lights_update_loop() {
  // If we aren't doing anything, don't do anything.
  if (!led_ring_animating || gsUpdateFlag) return 0;
  
  uint16_t required_delay_millis = 0; // How long to delay before calling me again.
  
  if (led_ring_count == 256 - FADE_SCALE || !led_ring_crossfade) {
  }
  // We split the fade into 256 steps, so what we do is setup targets,
  // then spend some iterations fading (possibly).
  if (led_ring_count == 0 || !led_ring_crossfade) {
    // If we just finished the last one, loop.
    if (led_ring_cur_frame >= led_ring_num_frames) {
      if (led_ring_looping)
        led_ring_cur_frame = 0;
      else if (led_ring_blinking && led_ring_blink_count > 0) {
        QCRing swap_ring = current_ring;
        current_ring = next_ring;
        next_ring = swap_ring;
        led_ring_blink_count--;
      }
      else {
        // TODO: we need to get to the target before we stop animating.
        led_ring_animating = 0;
      }
    }
    required_delay_millis += current_ring.ring_delay; // hold this frame
    if (!led_ring_blinking || led_ring_cur_frame+1 < led_ring_num_frames)
      memcpy_P(&current_ring, &ring_animations[led_ring_animation][led_ring_cur_frame], sizeof(QCRing));
    setupTargetRing(current_ring); // Sets up all the targets
    // Next frame in the pattern:
    led_ring_cur_frame++;
  }
  ring_fade(led_ring_crossfade);
  led_ring_count += FADE_SCALE;
  if (led_ring_crossfade) {
    required_delay_millis += led_ring_crossfade_step;
  }
  // Return how long we should wait until this is called again.
  return required_delay_millis;  
}

volatile uint8_t signal_min = 255;
volatile uint8_t signal_max = 0;
volatile uint8_t adc_amplitude = 0;
volatile uint16_t sample_count = 0;
volatile uint8_t adc_value = 0;
volatile float voltage = 0;
volatile uint8_t new_amplitude_available = 0;

void setupAdc() {  
  ADMUX = 0b01100110;  // default to AVCC VRef, ADC Left Adjust, and ADC channel 6
  ADCSRB = 0b00000000; // Analog Input bank 1
  ADCSRA = 0b11001111; // ADC enable, ADC start, manual trigger mode, ADC interrupt enable, prescaler = 128
}

void disableAdc() {
  ADCSRA = 0;
}


// The units for the following time intervals are, approximately, centiseconds.
volatile float volume_sums = 0;
volatile uint16_t volume_samples = 0;
#define VOLUME_INTERVAL 200
volatile float volume_avg = 0;
volatile uint8_t volume_peaking = 0;
volatile uint8_t volume_peaking_last = 0;
#define VOLUME_THRESHOLD 0.05
#define PEAKS_TO_PARTY 20
#define PARTY_PEAKS_INTERVAL 200
// TODO: make longer than 1000 centiseconds.
#define PARTY_TIME 1000
#define AUDIO_SPIKE volume_peaking && !volume_peaking_last
volatile uint8_t num_peaks = 0;
volatile uint8_t peak_samples = 0;
volatile uint8_t party_mode = 0;
volatile uint16_t party_time = 0;

void enter_party_mode(uint16_t duration) {
  if (!AM_SUPERUBER) {
    setupAdc();
  }
  party_mode = 1;
  force_idle = 1;
  party_time = duration;
  current_sys = SYSTEM_BLANK_INDEX; // Blank the system lights.
}

void leave_party_mode() {
  if (!AM_SUPERUBER) {
    disableAdc();
  }
  party_mode = 0;
  party_time = 0;
  force_idle = 1;
}
 
ISR(ADC_vect) { // Analog->Digital Conversion Complete
  if ((ADMUX & 0b00000111) == 6) { // Channel 6:
    adc_value = ADCH;  // store ADC result (8-bit precision)
    ADCSRA |= 0b11000000;  // manually trigger the next ADC
    sample_count++;
    if (adc_value > signal_max)
      signal_max = adc_value;
    else if (adc_value < signal_min)
      signal_min = adc_value;
    if (sample_count == 100) { // Happens about 100 times per second:
      adc_amplitude = signal_max - signal_min;
      voltage = (adc_amplitude * 3.3) / 256;
      signal_min = 255;
      signal_max = 0;
      sample_count = 0;
      
      // If we're in party mode, decrement the party timer.
      // Then determine whether we should turn off party mode:
      if (party_mode && party_time  == 0) {
        leave_party_mode();
      }
      else if (party_mode) {
        party_time--;
      }
      
      // We listen for VOLUME_INTERVAL samples to establish an average
      // background noise level. Then we store it as the volume_avg,
      // clear the average counter, and use that average as the first sample
      // in the next volume average.
      if (volume_samples > VOLUME_INTERVAL) {
        volume_avg = volume_sums / volume_samples;
        volume_sums = volume_avg + voltage;
        volume_samples = 2;
      }
      else {
        volume_samples++;
        volume_sums += voltage;
      }
      
      // If we need to restart our running count of peaks for party mode entry:
      if (peak_samples > PARTY_PEAKS_INTERVAL) {
        num_peaks = 0;
        peak_samples = 0;
      }
      else {
        peak_samples++;
      }
      
      // Then, if it's sufficiently larger than the average,
      // set the volume peaking flag, having stored the previous one.
      volume_peaking_last = volume_peaking;
      volume_peaking = (voltage > volume_avg + VOLUME_THRESHOLD);
      
      // If we're eligible to listen for clicks to turn on party mode:
      if (!party_mode && AM_SUPERUBER && !in_preboot && AUDIO_SPIKE) {
        num_peaks++;
        if (num_peaks > PEAKS_TO_PARTY) { // PARTY TIME!
          enter_party_mode(PARTY_TIME);
        } // Or if we're in party mode and need to listen for clicks:
      } else if (AUDIO_SPIKE && party_mode && idling) {
        // We've detected a new beat.
        led_next_sys = set_system_lights_animation(SYSTEM_PARTY_INDEX, 
                                                   LOOP_FALSE, 0);
      } else if (AUDIO_SPIKE && in_preboot) { // Or if we're testing:
          led_next_sys = set_system_lights_animation(11, LOOP_FALSE, 0);
      }
    }
  }
};