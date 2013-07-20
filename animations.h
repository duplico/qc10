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


