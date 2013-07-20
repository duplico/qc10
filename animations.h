#pragma once

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

extern QCRing *ring_animations[];
extern uint8_t ring_anim_lengths[];
extern QCSys heartbeats[][4];
















/*
QCRing ring_animations[][16] = { // Inefficient use of space. So sue me.
  /////
  #define QCR_DELAY 10
  {
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 128, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 128, 0, 128, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 128, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 128, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 128, 0, 128, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 128, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY}},
   
   //////
   // EFFECT: Bouncing color pairs.
  #define QCR_DELAY 100
  {
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY}},

// EFFECT: Bouncing single LED
#define QCR_DELAY 100

{{0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY}},
///////
// EFFECT: Backwards rotation of color pairs, twice
{
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY}},


// EFFECT: Alternating opposite lights while spinning clockwise.
#define QCR_DELAY 100

  {
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 128, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   128, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 128, 0, 0,
   QCR_DELAY},
  {0, 128, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 128, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 128, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 128, 0,
   QCR_DELAY},
  {0, 0, 128, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 128,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 128, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 128,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {128, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY},
  {0, 0, 0, 0, 0, 0,
   0, 0, 0, 0, 0, 0,
   QCR_DELAY}},

};
*/
