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

Port leds (1);
MilliTimer sendTimer;
char payload[] = "Hello!";
byte needToSend;

int beacon_timer_range = 500;
int beacon_timer_base = 3000;
int beacon_timer = beacon_timer_base;

byte seen_addresses[10][26] = {0}; // "Sparse"ish array of badge groups and IDs I've seen. Let's keep them A-Z please.

// configuration settings, saved in eeprom, can be changed from serial port
struct {
    byte check;
    byte freq, rcv_group, rcv_id, bcn_group, bcn_id;
} config;

static word code2freq(byte code) {
    return code == 4 ? 433 : code == 9 ? 915 : 868;
}

static word code2type(byte code) {
    return code == 4 ? RF12_433MHZ : code == 9 ? RF12_915MHZ : RF12_868MHZ;
}

static void showConfig() {
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
    if (config.check != 150) {
        config.check = 150;
        config.freq = 9;
        config.rcv_group = 0;
        config.rcv_id = 1;
        config.bcn_group = 1;
        config.bcn_id = 1;
        saveConfig();
    }
    showConfig();
    rf12_initialize(config.rcv_id, code2type(config.freq), config.rcv_group);
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
        // Its ID is GGGII where GGG=group, II=id:
        // GGG=formatted rf12_grp, II=formatted rf12_hdr.
        Serial.print("OK ");
        Serial.print(rf12_grp);
        Serial.print("g ");
        Serial.print(rf12_hdr);
        Serial.print("hdr; data: ");
        for (byte i = 0; i < rf12_len; ++i)
            Serial.print(rf12_data[i]);
        Serial.println();
        // Let's update our nearby-badges count.
        //TODO: update
        //TODO: if in training mode:
        // If this one is our number:
        //  Pick number+1 mod 26 as our candidate; inc it until we find something we haven't
        //  seen.
    }
    
    if (sendTimer.poll(beacon_timer))
        needToSend = 1;

    if (needToSend && rf12_canSend()) {
        rf12_sendStart(0, payload, sizeof payload);
        Serial.println("Preparing to send.");
        // We'll send our own beacon now.
        // Switch to beacon mode:
        rf12_initialize(config.bcn_id, code2type(config.freq), config.bcn_group);
        while (!rf12_canSend())
          rf12_recvDone();
        needToSend = 0;
        rf12_sendStart(0, payload, sizeof payload);
        Serial.println("Sent.");
        // Back to receive:
        rf12_initialize(config.rcv_id, code2type(config.freq), config.rcv_group);
        // Now fuzz the timer from which we decide when we're going to send.
        beacon_timer = beacon_timer_base + random(beacon_timer_range) - (beacon_timer_range / 2);
    }
}
