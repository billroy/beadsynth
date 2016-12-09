BEADSynth README

### What is it?

This is a simple 5-parameter sound synthesizer based on the excellent Mozzi
sound synthesis library for Arduino.  It can generate a wide variety of
beeps, bloops, dings, and squawks.

You configure the synth using commands over the serial port, and trigger it
using digital inputs or the serial port.

More on Mozzi here: https://github.com/sensorium/Mozzi

### Synthesis model

The synth generates a sine wave of duration [attack] + [decay] ms, followed by
[quiet] ms of silence.  The attack and decay parameters control an EAD gain
envelope on the generated signal.

The sine wave frequency starts at [begin_freq] and sweeps linearly up or down
to [end_freq] over the duration of audible sound (attack + decay).  If begin_freq and end_frew are the same, the frequency is constant.

When the 'r' command is used to repeat a sound, there is a silent interval of
[quiet] ms between repetitions.

### Software setup

1. Install the Mozzi library into the Arduino IDE per the Mozzi instructions.

2. Copy and paste the text of beadsynth.ino into a new file in the Arduino IDE.

3. Upload to your Arduino.

4. Connect with the serial monitor at 115200 baud to command the synthesizer.

    Tip: Try bloader instead of the Serial Monitor:

    $ npm install -g bloader

    $ bloader -b 115200

    More at https:github.com/billroy/bloader.js

### Hardware setup

The sound is output on pin 9.  Consult Mozzi for guidance on how to connect
your headphone/speaker/amplifier, moving the pin, and on filtering the audio
to remove synthesis artifacts.

A low input on any of the pins defined in the trigger_pins array triggers the
synth.  By default, trigger_pins is defined to use all the remaining digital
pins on a standard arduino:

    uint8_t trigger_pins[] = {2,3,4,5, 6,7,8,10, 11,12};

So a LOW on pin 2 will trigger the sound in slot 0, a low on 3 the sound in slot
1, and so on.

While a sound is playing, pin 13 is high; otherwise it is low.

The 'i' command reads a specified analog input and passes the value along to the
next command; this can be useful for connecting an external sensor to control
one or more parameters.


### Command interpreter

A simple command interpreter listens for character-based commands on the
serial port and executes parameter changes and sound playback.

#### Command arguments

The command interpreter uses 'postfix' notation, which means you type
the argument before the command.  So, to set the beginning frequency:

    440b

The last number you enter is stored so you can use it multiple times;
this example sets the beginning and ending frequency and the attack and
decay times all to 440, and then plays the sound:

    440beadp    // play a violin-like 440 A

Unrecognized commands and whitespace are silently ignored.

#### p: play the current sound

The p command plays the current sound once.

Serial input is paused while sound is playing and resumed when it finishes.

    p       // play the current sound
    ppp     // play the current sound 3 times

#### r: repeat the current sound N times

The r command plays the current sound N times.

Serial input is paused while sound is playing and resumed when it finishes.

    3r          // play the current sound 3 times
    3000r       // play the current sound 3000 times

There is a quiet interval of [quiet] ms between repeats.

#### b, e: set begin/end frequency in Hz

The b and e commands set the begin and end frequency in hertz.

The Mozzi synthesis library uses a sampling frequency around 16kHz, so
frequencies above 8 kHz will sound funky and aliased, which may or may not
be useful in your application.

    440bp           // change the begin frequency to 440; end is not changed
    880ep           // change only the end frequency to 880
    440b880ep       // play a sweep from 440 to 880

#### a,d: attack and decay times in ms

These parameters control the Exponential Attack Decay gain envelope that shapes
the volume of the sound.

    50a2000dp       // play a ding sound with a long tail

#### q: quiet time between repeats

    1000q5r         // set a 1 second silence and play 5 times

#### i: read analog input

You can read an analog input and use it to control a parameter.

    2ibp            // read analog pin 2 and set its value as the begin freq

#### =: see parameters

Dumps the parameter settings to the serial port.

    =
    begin:440
    end:440
    attack:440
    decay:440
    quiet:0

#### >: Write parameters to slot

You can store 32 parameter sets in eeprom in slots 0-31, for later quick recall
and use.  Use the '>' command to write a slot:

    440b880e50a200d10>      // write a tone to slot 10

#### <: Read parameters from slot

To retrieve a stored parameter set, use the '<' command:

    10<         // fetch slot 10
    10<p11<3r   // fetch slot 10, play it, then fetch slot 11 and play 3x

If you load the data from an uninitialized slot you will hear some funky siren-
like synth abuse that may or may not be useful in your application.

#### @: Dump eeprom

A no-frills dump of eeprom for debugging.


### Example sounds

    Drip: 110b880e80a75dp
    Ding: 4000be50a2000dp
    Hawk: 2000b220e50b500dp
    PewPewPew: 2000b220e50b50d3r
