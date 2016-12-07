//  BEAD Synthesizer 0.0
//
//  Copyright 2016 by Bill Roy.  License: MIT - see LICENSE file
//
//  This is a simple sound-effect synthesizer based on the excellent Mozzi.
//  It generates sounds by sweeping from a beginning to an end frequency
//  and uses an EAD envelope with attack and decay parameters to shape the gain
//
#include <MozziGuts.h>
#include <Oscil.h>
#include <EventDelay.h>
#include <Ead.h>
#include <Line.h>
#include <tables/sin8192_int8.h>
#include <EEPROM.h>

#define CONTROL_RATE 64
#define TRIGGER_PIN 8

Oscil <8192, AUDIO_RATE> aOscil(SIN8192_DATA);;
EventDelay noteDelay;
Ead envelope(CONTROL_RATE);
Line <long> aSlide;
int gain;

// default parameters
int begin_freq      = 110;
int end_freq        = 880;
unsigned int attack = 50;
unsigned int decay  = 75;
unsigned int quiet  = 0;

// synth state control
byte start_playing = 0;
byte playing = 0;
unsigned int repeat = 1;        // gratuitous beep at startup; set to 0 for silent start

// serial input handling
unsigned int accumulator;
unsigned int arg;
byte accumulator_touched;

void showParams() {
    Serial.println();
    Serial.print("begin:"); Serial.println(begin_freq);
    Serial.print("end:"); Serial.println(end_freq);
    Serial.print("attack:"); Serial.println(attack);
    Serial.print("decay:"); Serial.println(decay);
    Serial.print("quiet:"); Serial.println(quiet);
    Serial.print("arg:"); Serial.println(arg);
}

/*
    There are 32 'slots' in eeprom that can be used to save parameter sets
        3>      save current settings in slot 3
        3<      load current settings from slot 3
        3<p0<p  load and play slot three, then slot 0

    eeprom slot definition

        Offset  Variable
        ------  --------
        0-1     begin_freq
        2-3     end_freq
        4-5     attack
        6-7     decay
        8-9     quiet
*/
#define eg EEPROM.get
#define ep EEPROM.put

void readSlot(int slot) {
    int offset = (slot & 0x1f) << 4;
    eg(offset, begin_freq);
    eg(offset + 2, end_freq);
    eg(offset + 4, attack);
    eg(offset + 6, decay);
    eg(offset + 8, quiet);
}

void writeSlot(int slot) {
    int offset = (slot & 0x1f) << 4;
    ep(offset, begin_freq);
    ep(offset + 2, end_freq);
    ep(offset + 4, attack);
    ep(offset + 6, decay);
    ep(offset + 8, quiet);
    ep(offset + 10, (long) 0xFFFFFFFFUL);
    ep(offset + 14, (int) 0xFFFF);
}

void dumpEeprom() {
    int slot, offset;
    Serial.println();
    for (slot=0; slot<32; slot++) {
        Serial.print(slot);
        Serial.print(":");
        int value;
        eg(slot * 16, value); Serial.print(" "); Serial.print(value);
        eg(slot * 16 + 2, value); Serial.print(" "); Serial.print(value);
        eg(slot * 16 + 4, value); Serial.print(" "); Serial.print(value);
        eg(slot * 16 + 6, value); Serial.print(" "); Serial.print(value);
        eg(slot * 16 + 8, value); Serial.print(" "); Serial.print(value);
        for (offset=10; offset < 16; offset++) {
            Serial.print(" ");
            Serial.print(EEPROM.read(slot * 16 + offset), HEX);
        }
        Serial.println();
    }
    Serial.println();
}

int getAnalogInput(int pin) {
    int value = mozziAnalogRead(pin);

    // first reading is zero.  reread here if we see a zero to skip it.
    if (value) return value;
    return mozziAnalogRead(pin);
}

void setup() {
    Serial.begin(115200);
    Serial.println("beadsynth here! v0.0");
    startMozzi(CONTROL_RATE);
}

void updateControl() {
    if (playing && !noteDelay.ready()) {
        //Serial.print("playing: ");
        aOscil.setFreq((int) aSlide.next());
        gain = (int) envelope.next();
        return;
    }

    if (start_playing || repeat) {
        //Serial.println("starting a repeat");
        digitalWrite(13, 1);
        playing = 1;
        start_playing = 0;
        if (repeat) repeat--;
        aOscil.setFreq(begin_freq);
        unsigned long steps = (attack + decay) / (1000 / CONTROL_RATE);
        aSlide.set(begin_freq, end_freq, steps);
        envelope.set(attack, decay);
        envelope.start();
        noteDelay.start(attack + decay + quiet);
    }
    else {
        //Serial.println("stopping play");
        playing = 0;
        gain = 0;
        digitalWrite(13, 0);
    }
}

int updateAudio() {
    return (int) (gain * aOscil.next())>>8;
}

void loop() {
    audioHook();

    // feed serial input to the command handler
    while (Serial.available() && !playing && !start_playing) handleCommand(Serial.read());

    // look for a trigger input on the watch pin
    if (digitalRead(TRIGGER_PIN) && !playing && !start_playing) {
        repeat = 1;
        start_playing = 1;
    }
}


void handleCommand(char cmd) {
    Serial.print(cmd);  // echo

    // accumulate numeric inputs
    if (isDigit(cmd)) {
        accumulator = (10 * accumulator) + cmd - '0';
        accumulator_touched = 1;
        return;
    }

    // not a digit - end accumulation and save arg
    if (accumulator_touched) arg = accumulator;
    accumulator_touched = 0;
    accumulator = 0;
    switch (cmd) {
        case 'b': begin_freq = arg; break;
        case 'e': end_freq   = arg; break;
        case 'a': attack     = arg; break;
        case 'd': decay      = arg; break;
        case 'q': quiet      = arg; break;
        case 'r': repeat     = arg; start_playing = 1; break;
        case 'p': repeat     = 1;   start_playing = 1; break;
        case 'i': arg = getAnalogInput(arg); break;
        case '>': writeSlot(arg);   break;
        case '<': readSlot(arg);    break;
        case '=': showParams();     break;
        case '@': dumpEeprom(); break;
        case '\r':
        case '\n':
            Serial.println();
            Serial.println();
            break;

    }
}
