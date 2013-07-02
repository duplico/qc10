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

byte needToSend; // Whether we should send a beacon when we get the chance.

int beacon_timer_range = 500; // How much to fuzz the beacon interval
int beacon_timer_base = BEACON_TIMER_BASE; // Beacon interval base
int beacon_timer = beacon_timer_base; // Fuzzed beacon interval

int gaydar_timer = GAYDAR_TIMER; // How often to average our beacon window.

// TODO: "Sparse"ish array of badge groups and IDs I've seen?

// Configuration settings struct, to be stored on the EEPROM.
struct {
    byte check;
    byte freq, rcv_group, rcv_id, bcn_group, bcn_id;
    unsigned int badge_id;
} config;

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
    if (config.check != 153) {
        config.check = 153;
        config.freq = 4;
        config.rcv_group = 0;
        config.rcv_id = 1;
        config.bcn_group = 0;
        config.bcn_id = 1;
        config.badge_id = 1;
        saveConfig();
    }
    // Convert our badge ID int into a 2-length byte array.
    id_payload[0] = (config.badge_id & 0b1111111100000000) >> 8;
    id_payload[1] = (config.badge_id & 0b11111111);
    
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
    startTLC();
}

void loop () {
    // Check to see if we've received something valid (CRC checks out).
    if (rf12_recvDone() && rf12_crc == 0) {
        // We've received something. Is it the right length?
        if (rf12_len == 2) {            
            // Print the metadata and badge ID of the beacon we've received.
            Serial.print("<-|RCV OK ");
            Serial.print(rf12_grp);
            Serial.print("g ");
            Serial.print(rf12_hdr);
            Serial.print("hdr; data: ");
            // Construct an integer badge ID from a 2-array of bytes.
            unsigned int incoming_id = rf12_data[0] << 8 | rf12_data[1];
            Serial.println(incoming_id);
            // Increment our beacon count in the current position in our
            // sliding window.
            received_numbers[receive_cycle]+=1;
            // If we're in ID learning mode, and we've heard an ID that we
            // thought was supposed to be us:
            if (LEARNING && incoming_id == config.badge_id) {
                // Increment our ID by one.
                config.badge_id += 1;
                saveConfig();
                Serial.print("--|Duplicate ID detected. Incrementing mine to ");
                Serial.println(config.badge_id);
                // TODO: don't pick an ID that we've heard recently/before.
            }
        } else {
            // Wrong length.
            Serial.println("Malformed packet received.");
        }
    }
    
    // Check the beacon timer to see if it's about time to send ours.
    if (sendTimer.poll(beacon_timer)) {
        // Set the flag to cause us to send at the next opportunity.
        needToSend = 1;
        Serial.println("--|BCN required.");
        // Now fuzz the timer from which we decide when we're going to send.
        beacon_timer = beacon_timer_base + random(beacon_timer_range) - 
                      (beacon_timer_range / 2);
        // Advance the sliding window for received beacon counts:
        receive_cycle = (receive_cycle + 1) % RECEIVE_WINDOW;
        received_numbers[receive_cycle] = 0;
    }
    
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
    
    // Check if we NEED to send and if we CAN send.
    if (needToSend && rf12_canSend()) {
        needToSend = 0;
        // Beacon.
        rf12_sendStart(0, id_payload, sizeof id_payload);
        Serial.print("->|BCN badge number ");
        Serial.println(config.badge_id);
    }
    loopbody();
//    TLC5940_SetGS_And_GS_PWM();
}
