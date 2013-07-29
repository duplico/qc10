// Queercon 10 Badge Prototype
/*****************************
 * This code is intended to be run on a JeeNode-compatible platform.
 *
 * <http://www.queercon.org>
 */
// MIT License. http://opensource.org/licenses/MIT
//
// Based in part on example code provided by:
// (c) 17-May-2010 <jc@wippler.nl>
// (c) 11-Jan-2011 <jc@wippler.nl>
//
// Otherwise:
// (c) 13-Dec-2012 George Louthan <georgerlouth@nthefourth.com>
// (c) 28-Jul-2013 George Louthan <georgerlouth@nthefourth.com>
//
// Required fuse settings:
// lfuse: 0xBF
// hfuse: 0xDE

#include <JeeLib.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

extern "C"
{
  #include "animations.h"
  #include "tlc5940.h"
  #include "main.h"
}

// General (overall system) configuration
#define UBER_COUNT 10
#define CONFIG_STRUCT_VERSION 1
#define STARTING_ID 0
#define BADGES_IN_SYSTEM 105
#define BADGE_METER_INTERVAL 6
#define BADGE_FRIENDLY_CUTOFF 60

// Radio Configuration
// How many beacon cycles to look at together.
#define RECEIVE_WINDOW 8 
// NB: It's nice if the listen duration is divisible by BADGES_IN_SYSTEM 
#define R_SLEEP_DURATION 10000
#define R_LISTEN_DURATION 4410
#define R_LISTEN_WAKE_PAD 200
#define R_NUM_SLEEP_CYCLES 10

// LED configuration
#define USE_LEDS 1
#define DEFAULT_CROSSFADE_STEP 2
#define PREBOOT_INTERVAL 20000
#define PREBOOT_SHOW_COUNT_AT 2000
#define PREBOOT_SHOW_UBERCOUNT_AT 12500
#define CROSSFADING 1

// Look at all my global state!!!
// Timing
uint32_t last_time;
uint32_t current_time;
uint32_t elapsed_time;

// Gaydar
uint8_t neighbor_counts[RECEIVE_WINDOW] = {0};
uint8_t window_position = 0;
uint8_t badges_seen[BADGES_IN_SYSTEM];
uint8_t total_badges_seen = 0;
uint8_t uber_badges_seen = 0;
uint8_t last_neighbor_count = 0;
uint8_t neighbor_count = 0;

const uint16_t LNA_COMMANDS[4] = {
  0x94A2, // Max
  0x94AA, // -6 dB
  0x94B2, // -14 dB
  0x94BA  // -20 dB
};

uint8_t lna_setting = 0;

// LED subsystem
uint16_t time_since_last_bling = 0;
uint16_t seconds_between_blings = 25;
uint8_t current_bling = 0;
uint8_t shown_badgecount = 0;
uint8_t shown_ubercount = 0;
uint8_t in_preboot = 1;
volatile uint8_t current_sys = 0;
uint32_t led_next_ring = 0;
uint32_t led_next_uber_fade = 0;
uint32_t led_next_sys = 0;
uint8_t idling = 1;
volatile uint8_t force_idle = 0;
uint8_t need_to_show_near_badge = 0;
uint8_t need_to_show_new_badge = 0;
uint8_t need_to_show_uber_count = 1;
uint8_t need_to_show_badge_count = 1;

// Radio subsystem and duty cycling
// millisecond clock in the current sleep cycle:
uint16_t t = 0;
// number of the current sleep cycle:
uint16_t cycle_number = R_NUM_SLEEP_CYCLES - 1; // Stay awake to start.
// whether we've successfully beaconed this cycle:
uint8_t sent_this_cycle = 0;
// at what t should we start trying to beacon:
uint16_t t_to_send = R_SLEEP_DURATION;
// my "authority", lower is more authoritative
uint16_t my_authority = BADGES_IN_SYSTEM;
// lowest badge id I've seen this cycle, used to calculate my next initial authority
// (my authority is the lowest badge to whom I can directly communicate; if one
//  of my neighbors is communicating directly with a lower id badge than I am,
//  that neighbor will be more authoritative than me.):
// whether the radio is asleep:
uint16_t lowest_badge_this_cycle = BADGES_IN_SYSTEM;
uint8_t badge_is_sleeping = 1;



// Payload struct
struct qcbpayload {
  uint8_t ver;
  uint16_t from_id, authority, cycle_number, timestamp, badges_in_system, party;
} in_payload, out_payload;

// Convert our configuration frequency code to the actual frequency.
static word code2freq(byte code) {
    return code == 4 ? 433 : code == 9 ? 915 : 868;
}

// Convert our configuration frequency code to RF12's frequency const.
static word code2type(byte code) {
    return code == 4 ? RF12_433MHZ : code == 9 ? RF12_915MHZ : RF12_868MHZ;
}

// Print our current configuration to the Serial console.
#if !(USE_LEDS)
  static void showConfig() {

      Serial.print("I am badge ");
      Serial.println(config.badge_id);
      Serial.print(' ');
      Serial.print(code2freq(config.freq));
      Serial.print(':');
      Serial.print((int) config.rcv_group);
      Serial.print(':');
      Serial.print((int) config.rcv_id);
      Serial.print(" -> ");
      Serial.print(code2freq(config.freq));
      Serial.print(':');
      Serial.print((int) config.bcn_group);
      Serial.print(':');
      Serial.print((int) config.bcn_id);
      Serial.println();
  }
#endif

// Load our configuration from EEPROM (also computing some payload).
static void loadConfig() {
  byte* p = (byte*) &config;
  for (byte i = 0; i < sizeof config; ++i)
      p[i] = eeprom_read_byte((byte*) i);
  // if loaded config is not valid, replace it with defaults
  if (config.check != CONFIG_STRUCT_VERSION) {
      config.check = CONFIG_STRUCT_VERSION;
      config.freq = 4;
      config.rcv_group = 211;
      config.rcv_id = 1;
      config.bcn_group = 211;
      config.bcn_id = 1;
      config.badge_id = STARTING_ID;
      config.badges_in_system = BADGES_IN_SYSTEM;
      
      total_badges_seen = 0;
      uber_badges_seen = 0;
      memset(badges_seen, 0, BADGES_IN_SYSTEM);
      badges_seen[config.badge_id] = 1;
      for (uint8_t i = 0; i<BADGES_IN_SYSTEM; i++) {
        saveBadge(i);
      }
      total_badges_seen++;
      if (config.badge_id < UBER_COUNT)
        uber_badges_seen++;
      saveConfig();
  }

  // Store the parts of our config that rarely change in the outgoing payload.
  out_payload.from_id = config.badge_id;
  out_payload.badges_in_system = config.badges_in_system;
  out_payload.ver = config.check;
  #if !(USE_LEDS)
    showConfig();
  #endif
  rf12_initialize(config.rcv_id, code2type(config.freq), 1);
    
  lowest_badge_this_cycle = config.badge_id;
  my_authority = config.badge_id;
  t_to_send = R_SLEEP_DURATION + (R_LISTEN_WAKE_PAD / 2) + 
              ((R_LISTEN_DURATION - R_LISTEN_WAKE_PAD) 
               / config.badges_in_system) * config.badge_id;
}

static void saveBadge(uint16_t badge_id) {
  int badge_address = (sizeof config) + badge_id;
  eeprom_write_byte((uint8_t *) badge_address, badges_seen[badge_id]);
}

static void loadBadges() {
  total_badges_seen = 0;
  uber_badges_seen = 0;
  for (byte i = 0; i < BADGES_IN_SYSTEM; i++) {
    badges_seen[i] = (eeprom_read_byte((byte*) (i + sizeof config))); 
    // Count ourselves:
    if (i == config.badge_id) {
      badges_seen[i] = 1;
      saveBadge(i);
    }
    total_badges_seen += badges_seen[i];
    if (i < UBER_COUNT) {
      uber_badges_seen += badges_seen[i];
    }
  }
}

static uint8_t has_seen_badge(uint16_t badge_id) {
  return (badges_seen[badge_id]);
}

// Returns true if this is the first time we've seen the badge.
static uint8_t save_and_check_badge(uint16_t badge_id) {
  uint8_t seen_before = has_seen_badge(badge_id);
  badges_seen[badge_id] |= 1;
#if !(USE_LEDS)
  Serial.print("--|Just saw ");
  Serial.print(badge_id);
  Serial.print(". Seen: ");
  Serial.println(has_seen_badge(badge_id));
#endif
  if (seen_before) {
    return 0;
  }
  if (badge_id < UBER_COUNT) {
    uber_badges_seen++;
  }
  total_badges_seen++;
  saveBadge(badge_id);
  return true;
}

// Save our current configuration to the EEPROM. Also calls loadConfig().
// In general, this should be called after any configuration change because it
// does a little housekeeping.
static void saveConfig() {
    byte* p = (byte*) &config;
    for (byte i = 0; i < sizeof config; ++i)
        eeprom_write_byte((byte*) i, p[i]);
}

void setup() {
    delay(100);
  wdt_enable(WDTO_1S);
  #if !(USE_LEDS)
    Serial.begin(57600);
    Serial.println(57600);
  #else
    randomSeed(analogRead(0)); // For randomly choosing blings
  #endif
  loadConfig();
  loadBadges();
  last_time = millis();
  current_time = millis();
  #if USE_LEDS
    startTLC();
    set_system_lights_animation(SYSTEM_PREBOOT_INDEX, LOOP_TRUE, 0);
    if (AM_SUPERUBER) { // uber
      set_ring_lights_animation(SUPERUBER_INDEX, LOOP_FALSE, CROSSFADING, DEFAULT_CROSSFADE_STEP, 0, UBERFADE_FALSE);
    }
    
    // Setup my party flasher to be a random color:
    heartbeats[SYSTEM_PARTY_INDEX][0].f_red = random(0,2) * 96;
    heartbeats[SYSTEM_PARTY_INDEX][0].f_grn = random(0,2) * 96;
    heartbeats[SYSTEM_PARTY_INDEX][0].f_blu = random(0,2) * 96;
    if (!heartbeats[SYSTEM_PARTY_INDEX][0].f_red && !heartbeats[SYSTEM_PARTY_INDEX][0].f_grn && heartbeats[SYSTEM_PARTY_INDEX][0].f_blu) {
      heartbeats[SYSTEM_PARTY_INDEX][0].f_red = 96;
      heartbeats[SYSTEM_PARTY_INDEX][0].f_blu = 48;
    }
    
    heartbeats[11][0].f_red = heartbeats[SYSTEM_PARTY_INDEX][0].f_red;
    heartbeats[11][0].f_grn = heartbeats[SYSTEM_PARTY_INDEX][0].f_grn;
    heartbeats[11][0].f_blu = heartbeats[SYSTEM_PARTY_INDEX][0].f_blu;
    
    
    // TODO: epilepsy warning.
    // heartbeats[SYSTEM_PARTY_INDEX][0][0..2]
    
  #endif
  setupAdc();
}

void set_heartbeat(uint8_t target_sys) {
  if (!party_mode)
    current_sys = target_sys;
}

void set_gaydar_state(uint16_t cur_neighbor_count) {
  if (last_neighbor_count == 0 && cur_neighbor_count != 0 && need_to_show_new_badge == 0 && current_sys == 0) {
    need_to_show_near_badge = 1;
  }
  if (cur_neighbor_count == 0) {
    set_heartbeat(0);
    seconds_between_blings = 60;
  } else if (cur_neighbor_count == 1) {
    set_heartbeat(1);
    seconds_between_blings = 45;
  } else if (cur_neighbor_count <= 3) {
    set_heartbeat(2);
    seconds_between_blings = 35;
  } else if (cur_neighbor_count <= 8) {
    set_heartbeat(3);
    seconds_between_blings = 27;
  } else if (cur_neighbor_count <= 15) {
    set_heartbeat(4);
    seconds_between_blings = 20;
  } else if (cur_neighbor_count <= 23) {
    set_heartbeat(5);
    seconds_between_blings = 15;
  } else {
    set_heartbeat(6);
    seconds_between_blings = 8;
  }
  last_neighbor_count = cur_neighbor_count;
}

void show_badge_count() {
  if (party_mode)
    return;
  // 14 is all? 24 is none.
  uint8_t end_index = 25 - (total_badges_seen / BADGE_METER_INTERVAL);
  if (total_badges_seen == 1)
    end_index = 26;
  if (end_index < 14)
    end_index = 14;
  if (end_index == 14) { // TODO: Test
    led_next_ring = set_ring_lights_blink(BADGECOUNT_INDEX, LOOP_FALSE, 
                                          CROSSFADING, DEFAULT_CROSSFADE_STEP,
                                          end_index, UBERFADE_TRUE,
                                          qcr_blinky_long, 16);
    return;
  }
  // TODO: test
  led_next_ring = set_ring_lights_blink(BADGECOUNT_INDEX, LOOP_FALSE, 
                                        CROSSFADING, DEFAULT_CROSSFADE_STEP,
                                        end_index, UBERFADE_FALSE,
                                        qcr_blinky_short, 16);
}

void show_uber_count() {
  if (party_mode)
    return;
  if (AM_UBER) { // TODO: test
    // Display "ALL" animation
    led_next_ring = set_ring_lights_blink(UBERCOUNT_INDEX, LOOP_FALSE, CROSSFADING, 
                                          DEFAULT_CROSSFADE_STEP, 17, UBERFADE_TRUE,
                                          qcr_blinky_long, 16);
  }
  else { // TODO: test
    uint8_t end_index = 7 + uber_badges_seen;
    led_next_ring = set_ring_lights_blink(UBERCOUNT_INDEX, LOOP_FALSE, CROSSFADING, 
                                          DEFAULT_CROSSFADE_STEP, end_index, UBERFADE_FALSE,
                                          qcr_ubercount_blinky, 16);
  }
}

void do_ring_update() {
  // Determine whether we should turn on idle
  // force_idle allows us to force a re-up of our idle state.
  if (force_idle || (!led_ring_animating && !idling)) {
    idling = 1;
    force_idle = 0; // Happens at end of animation, always.
    time_since_last_bling = 0; // All ring animations should delay blings.
    // We just became idle, so we blank the ring if we're non-superuber
    //  or if we're uber and in party mode, or if we're in preboot.
    //  and if we're uber and not in party mode, we do an uber idle.
    if (AM_SUPERUBER && !party_mode && !in_preboot) {
      led_next_ring = set_ring_lights_animation(UBER_START_INDEX + config.badge_id, 
                                                LOOP_TRUE, CROSSFADING,
                                                DEFAULT_CROSSFADE_STEP, 0, UBERFADE_FALSE);
    }
    else {
      led_next_ring = set_ring_lights_animation(BLANK_INDEX, LOOP_TRUE, 
                                                CROSSFADE_FALSE, 0, 0, 
                                                UBERFADE_FALSE);
    }
  }
  if (party_mode) {
    if (need_to_show_badge_count)
      need_to_show_badge_count = 0;
    if (need_to_show_uber_count)
      need_to_show_uber_count = 0;
    if (need_to_show_near_badge) // Don't show the nearbadge animation please.
      need_to_show_near_badge = 0;
  }
  
  // New badge:
  //  Preemptive.
  // Happens ONLY due to radio action (can't even be triggered in preboot, when radio is off):
  if (need_to_show_new_badge) {
    idling = 0;
    need_to_show_new_badge = 0;
    led_next_ring = set_ring_lights_animation(NEWBADGE_INDEX, LOOP_FALSE, CROSSFADE_FALSE, 
                                              DEFAULT_CROSSFADE_STEP, 0, UBERFADE_FALSE);
    led_next_sys = set_system_lights_animation(SYSTEM_NEWBADGE_INDEX, LOOP_TRUE, 0);
    // Pre-empt the near badge animation.
    need_to_show_near_badge = 0;
  }
  
  // Badge count:
  //  Non-preemptive (except idle)
  // Happens either:
  //  (a) in preboot (flagged at boot)
  //  (b) due to radio action (only way to activate flag)
  if (idling && need_to_show_badge_count) {
    idling = 0;
    need_to_show_badge_count = 0;
    show_badge_count();
  }
  
  // Uber count:
  //  Non-preemptive (except idle)
  // Happens either:
  //  (a) in preboot (flagged at boot)
  //  (b) due to radio action (only way to activate flag)
  if (idling && need_to_show_uber_count) {
    idling = 0;
    need_to_show_uber_count = 0;
    show_uber_count();
  }
  
  // Near badge:
  //  Happens if we were lonely (no neighbors) and now I have 1+ neighbors.
  //  Preemptive.
  // Happens ONLY due to radio action (can't even be triggered in preboot, when radio is off):
  if (need_to_show_near_badge && !party_mode) {
    idling = 0;
    force_idle = 0; // Don't permit idle interruptions of this.
    need_to_show_near_badge = 0;
    led_next_ring = set_ring_lights_animation(NEARBADGE_INDEX, LOOP_FALSE, CROSSFADE_FALSE, 
                                              DEFAULT_CROSSFADE_STEP, 0, UBERFADE_FALSE);
  }
  
  // Bling:
  //  Happens occasionally.
  //  Non-preemptive (except idle)
  // Happens due to the passage of time.
  // Cannot be triggered in preboot, because in preboot, 
  //  time_since_last_bling doesn't increment.
  if (idling && !party_mode && (time_since_last_bling > seconds_between_blings * 1000)) {
    // Time to do a "bling":
    current_bling = random(BLING_START_INDEX, 
                           BLING_START_INDEX + BLING_COUNT + (AM_FRIENDLY ? UBLING_COUNT : 0));
    
    led_next_ring = set_ring_lights_animation(BLING_START_INDEX + current_bling, LOOP_FALSE, 
                                              CROSSFADING, 
                                              DEFAULT_CROSSFADE_STEP, 0, AM_UBER);
    time_since_last_bling = 0; // With longer times, maybe not clear this?
    idling = 0;
  }
  
  // Just the animation loop:
  if (led_ring_animating && current_time >= led_next_ring) {
    led_next_ring = ring_lights_update_loop() + current_time;
  }
  if (current_time >= led_next_uber_fade) {
    led_next_uber_fade = uber_ring_fade() + current_time;
  }
}

void do_sys_update() {
  if (led_sys_animating && current_time >= led_next_sys) {
    led_next_sys = system_lights_update_loop() + current_time;
  }
  // If we're booted and either (a) not showing an animation or
  //   (b) just finished the newbadge animation.
  //   go back to heartbeat and/or blank-for-party.
  if (!in_preboot && 
      (!led_sys_animating || 
       (!party_mode && idling && led_sys_animation != current_sys))) {
    // Go back to heartbeating (or blank in party mode)
    set_gaydar_state(neighbor_count); // Make sure we use the right heartbeat.
    led_next_sys = set_system_lights_animation(current_sys, LOOP_TRUE, 0);
  }
}

void do_led_control(uint32_t elapsed_time) {
  
  do_ring_update();
  do_sys_update();
  
  // If it's time to leave preboot:
  if (in_preboot && current_time > PREBOOT_INTERVAL) {
    in_preboot = 0;
    force_idle = 1;
    if (!AM_SUPERUBER) {
      disableAdc();
    }
  }
}

void loop () {
  wdt_reset();
  current_time = millis();
  elapsed_time = current_time - last_time;
  last_time = current_time;
  t += elapsed_time;
  if (!in_preboot)
    time_since_last_bling += elapsed_time;
  
  #if USE_LEDS
    do_led_control(elapsed_time);
  #endif
  
  //////// RADIO SECTION /////////
  // Radio duty cycle.
  if (cycle_number != 1 && t < R_SLEEP_DURATION) {
    // Radio sleeps unless we're in the last sleep cycle of an interval
    if (!badge_is_sleeping) {
      // Go to sleep if necessary, printing cycle information.
      rf12_sleep(0);
#if !(USE_LEDS)
      Serial.print("--|Cycle ");
      Serial.print(cycle_number);
      Serial.print("/");
      Serial.print(R_NUM_SLEEP_CYCLES);
      Serial.print(" t:");
      Serial.println(t);
      Serial.println("--|Sleeping radio.");
#endif
      badge_is_sleeping = true;
    }
  }
  else if (t < R_SLEEP_DURATION + R_LISTEN_DURATION) {
    // This is the part of the sleep cycle during which we should be listening
    if (badge_is_sleeping) {
      // Wake up if necessary, printing cycle information.
      rf12_sleep(-1);
      #if !(USE_LEDS)
        Serial.print("--|Cycle ");
        Serial.print(cycle_number);
        Serial.print("/");
        Serial.print(R_NUM_SLEEP_CYCLES);
        Serial.print(" t:");
        Serial.println(t);
        Serial.println("--|Waking radio.");
      #endif
      badge_is_sleeping = false;
    }
    // Radio listens
    
    if (rf12_recvDone() && rf12_crc == 0) {
      // We've received something. Is it valid?
      if (rf12_len == sizeof in_payload) {
          in_payload = *(qcbpayload *)rf12_data;
          if (in_payload.from_id < BADGES_IN_SYSTEM) {            
            #if !(USE_LEDS)
              // Print the metadata and badge ID of the beacon we've received.
              Serial.print("<-|RCV OK ");
              Serial.print(rf12_grp);
              Serial.print("g ");
              Serial.print(rf12_hdr);
              Serial.print("hdr; beacon from ");
              Serial.println(in_payload.from_id);
            #endif
            // Increment our beacon count in the current position in our
            // sliding window.
            neighbor_counts[window_position]+=1;
            led_next_sys = set_system_lights_animation(10, LOOP_FALSE, 0); // TODO: don't do this.
            // See if this is a new friend by calling this non-idempotent function:
            if (save_and_check_badge(in_payload.from_id)) {
              // Set to 2 if we just saw a superuber badge, 1 otherwise:
              need_to_show_new_badge = in_payload.from_id < UBER_COUNT ? 2 : 1;
              if (need_to_show_new_badge == 2) { // It was uber
                need_to_show_uber_count = 1;
              }
              else {
                need_to_show_badge_count = 1;
              }
            }
            // If this marks a new max we should start immediately showing it.
            if (neighbor_counts[window_position] > neighbor_count) {
              set_gaydar_state(neighbor_counts[window_position]);
              neighbor_count = neighbor_counts[window_position];
            }
            lowest_badge_this_cycle = min(in_payload.from_id, lowest_badge_this_cycle);
            if (in_payload.authority < UBER_COUNT) {
              if (!party_mode && in_payload.party != 0) {
                enter_party_mode(in_payload.party);
              }
            }
            if (in_payload.authority < my_authority || (in_payload.authority == my_authority && in_payload.from_id < config.badge_id)) {
            #if !(USE_LEDS)
              Serial.print("|--Detected a higher authority ");
              Serial.print(in_payload.authority);
              Serial.print(" than my current ");
              Serial.println(my_authority);
              Serial.print("Alter clock cycle by ");
              Serial.print((int)in_payload.cycle_number - (int)cycle_number);
              Serial.print(" and t by ");
              Serial.println((int)in_payload.timestamp - (int)t);
            #endif
            my_authority = in_payload.authority;
            cycle_number = in_payload.cycle_number;
            t = in_payload.timestamp;
          }
        } // ID check
      } else {
        // Wrong length.
        #if !(USE_LEDS)
          Serial.println("Malformed packet received.");
        #endif
      } // length check
    }
    if (!sent_this_cycle && t > t_to_send) {
      // Beacon.
      #if !(USE_LEDS)
        Serial.println("--|BCN required.");
      #endif
      out_payload.authority = my_authority;
      out_payload.cycle_number = cycle_number;
      out_payload.party = party_time;
      out_payload.timestamp = t;
      if (rf12_canSend()) {
        sent_this_cycle = true;
        // Beacon.
        #if !(USE_LEDS)
          Serial.println("->|BCN my number ");
        #else
          //set_system_lights_animation(9, LOOP_FALSE, 0);
        #endif
        rf12_sendStart(0, &out_payload, sizeof out_payload);
        my_authority = config.badge_id;
      }
    }
  }
  else {
    // Time for a new sleep cycle
    t = 0;
    neighbor_count = 0;
    for (int i=0; i<RECEIVE_WINDOW; i++) {
      if (neighbor_counts[i] > neighbor_count) {
        neighbor_count = neighbor_counts[i];
      }
    }
    window_position = (window_position + 1) % RECEIVE_WINDOW;
    neighbor_counts[window_position] = 0;
#if USE_LEDS
    set_gaydar_state(neighbor_count);
#else
    Serial.print("--|Update: ");
    Serial.print(neighbor_count);
    Serial.print(" neighbors");
#endif
    cycle_number++;
    if (!sent_this_cycle) {
      // Backoff on the LNA if we couldn't send.
      if (lna_setting < 3) {
        rf12_control(LNA_COMMANDS[++lna_setting]);
      }
    }
    sent_this_cycle = false;
    if (cycle_number > R_NUM_SLEEP_CYCLES) {
      // Time for a new interval
      my_authority = lowest_badge_this_cycle;
      lowest_badge_this_cycle = config.badge_id;
      cycle_number = 0;
    }
    else if (cycle_number == R_NUM_SLEEP_CYCLES - 1) {
      // Let's also show our current badge count.
      // Doing it like this should let people compare!
      if (need_to_show_badge_count == 0) need_to_show_badge_count = 1;
      if (!need_to_show_uber_count) need_to_show_uber_count = 1;
    }
  }
}
