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

#define BLING_START_INDEX 0
#define BLING_COUNT 10
#define UBLING_START_INDEX 10
#define UBLING_COUNT 10
#define UBER_START_INDEX 20
#define UBER_COUNT 10
#define NEWBADGE_INDEX 30
#define NEARBADGE_INDEX 31
#define BADGECOUNT_INDEX 32
#define SUPERUBER_INDEX 33
#define UBERCOUNT_INDEX 34

#define SYSTEM_PREBOOT_INDEX 7
