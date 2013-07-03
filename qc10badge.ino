// Queercon 10 Badge Prototype
/*****************************
 * This code is intended to be run on a JeeNode-compatible platform.
 * 
 *
 *
 *
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

#define CONFIG_STRUCT_VERSION 154
#define BADGES_IN_SYSTEM 100

#define LEARNING 1 // Whether to auto-negotiate my ID.
#define RECEIVE_WINDOW 9 // How many beacon cycles to average together.

#define BEACON_TIMER_BASE 3000 // Approximately how often to beacon.
#define GAYDAR_TIMER 15000 // How frequently to update our badge count average.

extern "C"
{
  #include "tlc5940.h"
  #include "main.h"
}

MilliTimer sendTimer;
MilliTimer gaydarTimer;

byte id_payload[] = {0, 0}; // Our badge ID in byte array form to beacon.

int received_numbers[RECEIVE_WINDOW] = {0}; // Sliding window of beacon counts.
byte receive_cycle = 0; // Current window position

int beacon_timer_range = 500; // How much to fuzz the beacon interval
int beacon_timer_base = BEACON_TIMER_BASE; // Beacon interval base
int beacon_timer = beacon_timer_base; // Fuzzed beacon interval

int gaydar_timer = GAYDAR_TIMER; // How often to average our beacon window.


// NB: It's nice if the listen duration is divisible by num_badges
unsigned int R_SLEEP_DURATION = 5000;
unsigned int R_LISTEN_DURATION = 5000;
unsigned int R_LISTEN_WAKE_PAD = 1000;
unsigned int R_NUM_SLEEP_CYCLES = 6;
unsigned int NUM_BADGES = BADGES_IN_SYSTEM;


unsigned long last_time;
unsigned long current_time;

// TODO: "Sparse"ish array of badge groups and IDs I've seen?

// Configuration settings struct, to be stored on the EEPROM.
struct {
    uint8_t check;
    uint8_t freq, rcv_group, rcv_id, bcn_group, bcn_id;
    uint16_t badge_id;
    uint8_t badges_in_system; // TODO: make an unsigned int.
} config;

// Payload struct
struct qcbpayload {
  uint16_t from_id, authority, cycle_number, timestamp, badges_in_system;
  uint8_t ver;
} in_payload, out_payload;

// Convert our configuration frequency code to the actual frequency.
static word code2freq(byte code) {
    return code == 4 ? 433 : code == 9 ? 915 : 868;
}

// Convert our configuration frequency code to RF12's frequency const.
static word code2type(byte code) {
    return code == 4 ? RF12_433MHZ : code == 9 ? RF12_915MHZ : RF12_868MHZ;
}

// Print our current configuration to the serial console.
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

// Load our configuration from EEPROM (also computing our ID payload).
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
        saveConfig();
    }
    // Convert our badge ID int into a 2-length byte array.
    id_payload[0] = (config.badge_id & 0b1111111100000000) >> 8;
    id_payload[1] = (config.badge_id & 0b11111111);
    
    NUM_BADGES = config.badges_in_system;
    
    showConfig();
    rf12_initialize(config.rcv_id, code2type(config.freq), 1);
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

void setup () {
    Serial.begin(57600);
    Serial.println(57600);
    Serial.println("Send and Receive");
    loadConfig();
//    startTLC();
    last_time = millis();
    current_time = millis();
}

void loop () {
  static uint16_t t = 0;
  static uint16_t cycle_number = 0;
  static boolean sent_this_cycle = false;
  static uint16_t t_to_send = R_SLEEP_DURATION + (R_LISTEN_WAKE_PAD / 2) + 
    ((R_LISTEN_DURATION - R_LISTEN_WAKE_PAD) / NUM_BADGES) * config.badge_id;
  static uint16_t my_authority = config.badge_id;
  static uint16_t lowest_badge_this_cycle = config.badge_id;
  static boolean badge_is_sleeping = false;
  
  current_time = millis();
  t += (current_time - last_time);
  last_time = current_time;

  if (cycle_number != R_NUM_SLEEP_CYCLES && t < R_SLEEP_DURATION) {
    // Radio sleeps unless we're in the insomniac cycle
    if (!badge_is_sleeping) {
      rf12_sleep(0);
      Serial.println("--|Sleeping radio.");
      badge_is_sleeping = true;
    }
  }
  else if (t < R_SLEEP_DURATION + R_LISTEN_DURATION) {
    if (badge_is_sleeping) {
      rf12_sleep(-1);
      Serial.print("--|Cycle ");
      Serial.print(cycle_number);
      Serial.print("/");
      Serial.print(R_NUM_SLEEP_CYCLES);
      Serial.print(" t:");
      Serial.println(t);
      Serial.println("--|Waking radio.");
      badge_is_sleeping = false;
    }
    // Radio listens
    
    if (rf12_recvDone() && rf12_crc == 0) {
        // We've received something. Is it the right length?
        if (rf12_len == sizeof in_payload) {      
            // TODO: confirm correct version.      
            // Print the metadata and badge ID of the beacon we've received.
            Serial.print("<-|RCV OK ");
            Serial.print(rf12_grp);
            Serial.print("g ");
            Serial.print(rf12_hdr);
            Serial.print("hdr; beacon from ");
            in_payload = *(qcbpayload *)rf12_data;
            Serial.print(in_payload.from_id);
            Serial.print(" authority ");
            Serial.println(in_payload.authority);
            /*
            // Increment our beacon count in the current position in our
            // sliding window.
            received_numbers[receive_cycle]+=1;
            */
            // If we're in ID learning mode, and we've heard an ID that we
            // thought was supposed to be us:
            if (LEARNING && in_payload.from_id == config.badge_id) {
                // Increment our ID by one.
                config.badge_id += 1;
                saveConfig();
                Serial.print("--|Duplicate ID detected. Incrementing mine to ");
                Serial.println(config.badge_id);
                t_to_send = R_SLEEP_DURATION + (R_LISTEN_WAKE_PAD / 2) + 
                            ((R_LISTEN_DURATION - R_LISTEN_WAKE_PAD) / NUM_BADGES) * config.badge_id;
                // TODO: don't pick an ID that we've heard recently/before.
            }

            lowest_badge_this_cycle = min(in_payload.from_id, lowest_badge_this_cycle);
            if (in_payload.authority < my_authority) {
              Serial.print("|--Detected a higher authority ");
              Serial.print(in_payload.authority);
              Serial.print(" than my current ");
              Serial.println(my_authority);
              Serial.print("Alter clock cycle by ");
              Serial.print((int)in_payload.cycle_number - (int)cycle_number);
              Serial.print(" and t by ");
              Serial.println((int)in_payload.timestamp - (int)t);
              my_authority = in_payload.authority;
              cycle_number = in_payload.cycle_number;
              if (in_payload.timestamp < t_to_send) {
                sent_this_cycle = false;
              }
              t = in_payload.timestamp;
            }
          } else {
            // Wrong length.
            Serial.println("Malformed packet received.");
        }
    }
    if (!sent_this_cycle && t > t_to_send) {
      // Beacon.
      Serial.println("--|BCN required.");
      out_payload.from_id = config.badge_id;
      out_payload.authority = my_authority;
      out_payload.cycle_number = cycle_number;
      out_payload.timestamp = t;
      out_payload.badges_in_system = NUM_BADGES;
      out_payload.ver = config.check;
      if (rf12_canSend()) {
        sent_this_cycle = true;
        // Beacon.
        Serial.print("->|BCN badge number ");
        rf12_sendStart(0, &out_payload, sizeof out_payload);
        Serial.println(config.badge_id);
      }
      
      
    }
  }
  else {
    // Time for a new sleep cycle
    t = 0;
    cycle_number++;
    sent_this_cycle = false;
    if (cycle_number > R_NUM_SLEEP_CYCLES) {
      // Time for a new interval
      my_authority = lowest_badge_this_cycle;
      lowest_badge_this_cycle = config.badge_id;
      cycle_number = 0;
    }
  }
  
  // TODO: do something if this badge is authoritative.
  //       This badge is authoritative if my_authority = config.badge_id.
  
  /*
    // Check the gaydar timer to see if it's time to compute an average count.
    if (gaydarTimer.poll(gaydar_timer)) {
        double avg_seen = 0; // Holds a sum, then an average.
        for (int i=0; i<RECEIVE_WINDOW; i++) {
            // Skip the window position we're currently accumulating,
            // as it could be wrong (mid-cycle).
            if (i != receive_cycle)
                avg_seen += received_numbers[i];
        }
        // Compute the average (need window size - 1 because we skipped the
        // current position).
        avg_seen = avg_seen / (double)(RECEIVE_WINDOW - 1);
        Serial.print("--|Badge count update: ");
        Serial.println(avg_seen);
    }
//    loopbody();
*/
}
