// Queercon 10 Badge Prototype
/*****************************
 * This code is intended to be run on a JeeNode-compatible platform.
 *
 * <http://www.queercon.org>
 */
// MIT License. http://opensource.org/licenses/mit-license.php
//
// Based in part on example code provided by:
// (c) 17-May-2010 <jc@wippler.nl>
// (c) 11-Jan-2011 <jc@wippler.nl>
//
// Otherwise:
// (c) 13-Dec-2012 George Louthan <georgerlouth@nthefourth.com>
#include <JeeLib.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#define CONFIG_STRUCT_VERSION 156
#define BADGES_IN_SYSTEM 100

#define USE_LEDS 1

//EEPROM the learning state?
#define LEARNING 1 // Whether to auto-negotiate my ID.


extern "C"
{
  #include "animations.h"
  #include "tlc5940.h"
  #include "main.h"
}

// NB: It's nice if the listen duration is divisible by BADGES_IN_SYSTEM 
#define R_SLEEP_DURATION 5000
#define R_LISTEN_DURATION 5000
#define R_LISTEN_WAKE_PAD 1000
#define R_NUM_SLEEP_CYCLES 6

// Running timer results for our busy-waiting loop.
unsigned long last_time;
unsigned long current_time;

// Configuration settings struct, to be stored on the EEPROM.
struct {
    uint8_t check;
    uint8_t freq, rcv_group, rcv_id, bcn_group, bcn_id;
    uint16_t badge_id;
    uint16_t badges_in_system;
    uint16_t r_sleep_duration, r_listen_duration, r_listen_wake_pad,
             r_num_sleep_cycles;
} config;

// Payload struct
struct qcbpayload {
  uint16_t from_id, authority, cycle_number, timestamp, badges_in_system;
  uint8_t ver;
} in_payload, out_payload;

byte badges_seen[BADGES_IN_SYSTEM];
byte neighbor_badges[BADGES_IN_SYSTEM] = {0}; // All zeros.

// Convert our configuration frequency code to the actual frequency.
static word code2freq(byte code) {
    return code == 4 ? 433 : code == 9 ? 915 : 868;
}

// Convert our configuration frequency code to RF12's frequency const.
static word code2type(byte code) {
    return code == 4 ? RF12_433MHZ : code == 9 ? RF12_915MHZ : RF12_868MHZ;
}

// Print our current configuration to the //Serial console.
static void showConfig() {
    //Serial.print("I am badge ");
    //Serial.println(config.badge_id);
    //Serial.print(' ');
    //Serial.print(code2freq(config.freq));
    //Serial.print(':');
    //Serial.print((int) config.rcv_group);
    //Serial.print(':');
    //Serial.print((int) config.rcv_id);
    //Serial.print(" -> ");
    //Serial.print(code2freq(config.freq));
    //Serial.print(':');
    //Serial.print((int) config.bcn_group);
    //Serial.print(':');
    //Serial.print((int) config.bcn_id);
    //Serial.println();
}

// Load our configuration from EEPROM (also computing some payload).
static void loadConfig() {
    byte* p = (byte*) &config;
    for (byte i = 0; i < sizeof config; ++i)
        p[i] = eeprom_read_byte((byte*) i);
    // if loaded config is not valid, replace it with defaults
    if (config.check != CONFIG_STRUCT_VERSION) {
        config.check = CONFIG_STRUCT_VERSION;
        config.freq = 4;
        config.rcv_group = 0;
        config.rcv_id = 1;
        config.bcn_group = 0;
        config.bcn_id = 1;
        config.badge_id = 1;
        config.badges_in_system = BADGES_IN_SYSTEM;
        config.r_sleep_duration = R_SLEEP_DURATION;
        config.r_listen_duration = R_LISTEN_DURATION;
        config.r_listen_wake_pad = R_LISTEN_WAKE_PAD;
        config.r_num_sleep_cycles = R_NUM_SLEEP_CYCLES;
        saveConfig();
        
        memset(badges_seen, 0, BADGES_IN_SYSTEM);
    }
    
    // Store the parts of our config that rarely change in the outgoing payload.
    out_payload.from_id = config.badge_id;
    out_payload.badges_in_system = config.badges_in_system;
    out_payload.ver = config.check;
    
    showConfig();
    rf12_initialize(config.rcv_id, code2type(config.freq), 1);
}

static boolean has_seen_badge(uint16_t badge_id) {
  return (badges_seen[badge_id] & 0b0000001);
}

static boolean can_see_badge(uint16_t badge_id) {
  return (neighbor_badges[badge_id] & 0b0000001);
}

// Returns true if this is the first time we've seen the badge.
static boolean just_saw_badge(uint16_t badge_id) {
  neighbor_badges[badge_id] |= 0b00000011;
  boolean seen_before = has_seen_badge(badge_id);
  badges_seen[badge_id] |= 0b00000001;
  if (seen_before) {
    return false;
  }
  // TODO: save badges_seen to EEPROM.
  return true;
}

// Save our current configuration to the EEPROM. Also calls loadConfig().
// In general, this should be called after any configuration change because it
// does a little housekeeping.
static void saveConfig() {
    byte* p = (byte*) &config;
    for (byte i = 0; i < sizeof config; ++i)
        eeprom_write_byte((byte*) i, p[i]);
    loadConfig();
}

static void saveBadge(uint16_t badge_id) {
  int badge_address = (sizeof config) + badge_id;
  eeprom_write_byte((uint8_t *) badge_id, badges_seen[badge_id]);
}

void setup () {
    //Serial.begin(57600);
    //Serial.println(57600);
    loadConfig();
#if USE_LEDS
    startTLC();
    set_system_lights_animation(2, 1);
//    set_ring_lights_animation(0, 0);
#endif
    last_time = millis();
    current_time = millis();
}

void loop () {
  
  static unsigned long led_next_ring = 0;
  static unsigned long led_next_sys = 0;
  // millisecond clock in the current sleep cycle:
  static uint16_t t = 0;
  // number of the current sleep cycle:
  static uint16_t cycle_number = 0;
  // whether we've successfully beaconed this cycle:
  static boolean sent_this_cycle = false;
  // at what t should we start trying to beacon:
  static uint16_t t_to_send = config.r_sleep_duration + (config.r_listen_wake_pad / 2) + 
    ((config.r_listen_duration - config.r_listen_wake_pad) / config.badges_in_system) * config.badge_id;
  // my "authority", lower is more authoritative
  static uint16_t my_authority = config.badge_id;
  // lowest badge id I've seen this cycle, used to calculate my next initial authority
  // (my authority is the lowest badge to whom I can directly communicate; if one
  //  of my neighbors is communicating directly with a lower id badge than I am,
  //  that neighbor will be more authoritative than me.):
  static uint16_t lowest_badge_this_cycle = config.badge_id;
  // whether the radio is asleep:
  static boolean badge_is_sleeping = false;
  
  // Compute t using elapsed time since last iteration of this loop.
  current_time = millis();
  t += (current_time - last_time);
  last_time = current_time;
#if USE_LEDS
  static boolean need_to_fade = false;
  if (current_time >= led_next_ring) {
    led_next_ring += ring_lights_update_loop();
    need_to_fade = true;
  }
  if (current_time >= led_next_sys) {
    led_next_sys += system_lights_update_loop();
    need_to_fade = true;
  }
  if (need_to_fade) {
    fade_if_necessary(1);
  }
#endif

  // Radio duty cycle.
  if (cycle_number != config.r_num_sleep_cycles && t < config.r_sleep_duration) {
    // Radio sleeps unless we're in the last sleep cycle of an interval
    if (!badge_is_sleeping) {
      // Go to sleep if necessary, printing cycle information.
      rf12_sleep(0);
      //Serial.print("--|Cycle ");
      //Serial.print(cycle_number);
      //Serial.print("/");
      //Serial.print(config.r_num_sleep_cycles);
      //Serial.print(" t:");
      //Serial.println(t);
      //Serial.println("--|Sleeping radio.");
      badge_is_sleeping = true;
    }
  }
  else if (t < config.r_sleep_duration + config.r_listen_duration) {
    // This is the part of the sleep cycle during which we should be listening
    if (badge_is_sleeping) {
      // Wake up if necessary, printing cycle information.
      rf12_sleep(-1);
      //Serial.print("--|Cycle ");
      //Serial.print(cycle_number);
      //Serial.print("/");
      //Serial.print(config.r_num_sleep_cycles);
      //Serial.print(" t:");
      //Serial.println(t);
      //Serial.println("--|Waking radio.");
      badge_is_sleeping = false;
    }
    // Radio listens
    
    if (rf12_recvDone() && rf12_crc == 0) {
        // We've received something. Is it valid?
        if (rf12_len == sizeof in_payload) {      
            // TODO: confirm correct version.      
            // Print the metadata and badge ID of the beacon we've received.
            //Serial.print("<-|RCV OK ");
            //Serial.print(rf12_grp);
            //Serial.print("g ");
            //Serial.print(rf12_hdr);
            //Serial.print("hdr; beacon from ");
            in_payload = *(qcbpayload *)rf12_data;
            //Serial.print(in_payload.from_id);
            //Serial.print(" authority ");
            //Serial.println(in_payload.authority);
            // Increment our beacon count in the current position in our
            // sliding window.
            //received_numbers[receive_cycle]+=1;
            // If we're in ID learning mode, and we've heard an ID that we
            // thought was supposed to be us:
            if (LEARNING && in_payload.from_id == config.badge_id) {
                // Increment our ID by one.
                config.badge_id += 1;
                saveConfig();
                //Serial.print("--|Duplicate ID detected. Incrementing mine to ");
                //Serial.println(config.badge_id);
                t_to_send = config.r_sleep_duration + (config.r_listen_wake_pad / 2) + 
                            ((config.r_listen_duration - config.r_listen_wake_pad) / config.badges_in_system) * config.badge_id;
                // TODO: don't pick an ID that we've heard recently/before.
            }

            lowest_badge_this_cycle = min(in_payload.from_id, lowest_badge_this_cycle);
            if (in_payload.authority < my_authority) {
              //Serial.print("|--Detected a higher authority ");
              //Serial.print(in_payload.authority);
              //Serial.print(" than my current ");
              //Serial.println(my_authority);
              //Serial.print("Alter clock cycle by ");
              //Serial.print((int)in_payload.cycle_number - (int)cycle_number);
              //Serial.print(" and t by ");
              //Serial.println((int)in_payload.timestamp - (int)t);
              my_authority = in_payload.authority;
              cycle_number = in_payload.cycle_number;
              if (in_payload.timestamp < t_to_send) {
                sent_this_cycle = false;
              }
              t = in_payload.timestamp;
            }
          } else {
            // Wrong length.
            //Serial.println("Malformed packet received.");
        }
    }
    if (!sent_this_cycle && t > t_to_send) {
      // Beacon.
      //Serial.println("--|BCN required.");
      out_payload.authority = my_authority;
      out_payload.cycle_number = cycle_number;
      out_payload.timestamp = t;
      if (rf12_canSend()) {
        sent_this_cycle = true;
        // Beacon.
        //Serial.print("->|BCN badge number ");
        rf12_sendStart(0, &out_payload, sizeof out_payload);
        //Serial.println(config.badge_id);
      }
    }
  }
  else {
    // Time for a new sleep cycle
    t = 0;
    cycle_number++;
    sent_this_cycle = false;
    if (cycle_number > config.r_num_sleep_cycles) {
      // Time for a new interval
      my_authority = lowest_badge_this_cycle;
      lowest_badge_this_cycle = config.badge_id;
      cycle_number = 0;
    }
  }
}
