// Queercon 10 Badge Prototype
// MIT License. http://opensource.org/licenses/mit-license.php
//
// Based in part on example code provided by:
// (c) 17-May-2010 <jc@wippler.nl>
// (c) 11-Jan-2011 <jc@wippler.nl>
//
// Otherwise:
// (c) 13-Dec-2012 George Louthan <georgerlouth@nthefourth.com>
//
// http://www.queercon.org

#include <JeeLib.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#define LEARNING 1
#define RECEIVE_WINDOW 9

#define BEACON_TIMER_BASE 3000
#define GAYDAR_TIMER 15000

MilliTimer sendTimer;
MilliTimer gaydarTimer;

char payload[] = "Hello!";
byte id_payload[] = {0, 0};

int received_numbers[RECEIVE_WINDOW] = {0};
byte receive_cycle = 0;

byte needToSend;

int beacon_timer_range = 500;
int beacon_timer_base = BEACON_TIMER_BASE;
int beacon_timer = beacon_timer_base;

int gaydar_timer = GAYDAR_TIMER;

// TODO: "Sparse"ish array of badge groups and IDs I've seen?

// configuration settings, saved in eeprom, can be changed from serial port
struct {
    byte check;
    byte freq, rcv_group, rcv_id, bcn_group, bcn_id;
    unsigned int badge_id;
} config;

static word code2freq(byte code) {
    return code == 4 ? 433 : code == 9 ? 915 : 868;
}

static word code2type(byte code) {
    return code == 4 ? RF12_433MHZ : code == 9 ? RF12_915MHZ : RF12_868MHZ;
}

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

static void loadConfig() {
    byte* p = (byte*) &config;
    for (byte i = 0; i < sizeof config; ++i)
        p[i] = eeprom_read_byte((byte*) i);
    // if loaded config is not valid, replace it with defaults
    if (config.check != 152) {
        config.check = 152;
        config.freq = 9;
        config.rcv_group = 0;
        config.rcv_id = 1;
        config.bcn_group = 0;
        config.bcn_id = 1;
        config.badge_id = 1;
        saveConfig();
    }
    id_payload[0] = (config.badge_id & 0b1111111100000000) >> 8;
    id_payload[1] = (config.badge_id & 0b11111111);
    
    showConfig();
    rf12_initialize(config.rcv_id, code2type(config.freq), 1);
}

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
}

void loop () {
    if (rf12_recvDone() && rf12_crc == 0) {
        // We've received something.
        if (rf12_len == 2) {            
            Serial.print("<-|RCV OK ");
            Serial.print(rf12_grp);
            Serial.print("g ");
            Serial.print(rf12_hdr);
            Serial.print("hdr; data: ");
            unsigned int incoming_id = rf12_data[0] << 8 | rf12_data[1];
            Serial.println(incoming_id);
            received_numbers[receive_cycle]+=1;
            // If we're in ID learning mode, and we've heard an ID that we
            // thought was supposed to be us:
            if (LEARNING && incoming_id == config.badge_id) {
                // Increment our ID by one.
                config.badge_id += 1;
                saveConfig();
                Serial.print("Duplicate ID detected. Incrementing mine to ");
                Serial.println(config.badge_id);
                // TODO: don't pick an ID that we've heard recently/before.
            }
            // Let's update our nearby-badges count.
            //TODO: update
        } else {
            Serial.println("Malformed packet received.");
        }
    }
    
    if (sendTimer.poll(beacon_timer)) {
        needToSend = 1;
        // Now fuzz the timer from which we decide when we're going to send.
        beacon_timer = beacon_timer_base + random(beacon_timer_range) - 
                      (beacon_timer_range / 2);
        receive_cycle = (receive_cycle + 1) % RECEIVE_WINDOW;
        received_numbers[receive_cycle] = 0;
    }
    
    if (gaydarTimer.poll(gaydar_timer)) {
        double avg_seen = 0;
        for (int i=0; i<RECEIVE_WINDOW; i++) {
            if (i != receive_cycle)
                avg_seen += received_numbers[i];
        }
        avg_seen = avg_seen / (double)(RECEIVE_WINDOW - 1);
        Serial.print("--|Badge count update: ");
        Serial.println(avg_seen);
    }
    
    if (needToSend && rf12_canSend()) {
        needToSend = 0;
        
        rf12_sendStart(0, id_payload, sizeof id_payload);
        Serial.print("->|BCN badge number ");
        Serial.println(config.badge_id);
    }
}
