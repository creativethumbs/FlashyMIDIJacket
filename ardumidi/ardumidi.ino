// The flashy MIDI jacket is a wearable electronic midi controller
// Use Hairless MIDI for communication
// I use Ableton as the synthesizer

#include <CapacitiveSensor.h>
#include <MOS.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN6            7
#define PIN7            8

// zipper pin
#define ZIP             9

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      21

// threshold for when we determine that the capacitive sensor is pressed
// IMPORTANT!!! THIS WILL NEED TO BE CALIBRATED BASED ON WHAT YOUR ENVIRONMENT IS!!!!!!
#define THRESHOLD     165

// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixelsA = Adafruit_NeoPixel(NUMPIXELS, PIN6, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel pixelsB = Adafruit_NeoPixel(NUMPIXELS, PIN7, NEO_GRB + NEO_KHZ800);

// the number of note keys there are
const int numNotes = 5;
CapacitiveSensor   cs_2_3 = CapacitiveSensor(2, 3); // key 1
CapacitiveSensor   cs_2_4 = CapacitiveSensor(2, 4); // key 2
CapacitiveSensor   cs_2_5 = CapacitiveSensor(2, 5); // key 3
CapacitiveSensor   cs_2_6 = CapacitiveSensor(2, 6); // key 4
CapacitiveSensor   cs_2_10 = CapacitiveSensor(2, 10); // key 5

CapacitiveSensor   cs_2_11 = CapacitiveSensor(2, 11); // controller 1
CapacitiveSensor   cs_2_12 = CapacitiveSensor(2, 12); // controller 2
CapacitiveSensor   cs_2_13 = CapacitiveSensor(2, 13); // controller 3

int channel = 0;
int velocity = 100;
int noteON = 144;  // 144 = 10010000 in binary, note on command
int noteOFF = 128; // 128 = 10000000 in binary, note off command
int controlChange = 176; // = 10110000, control change

boolean pressingNote = false;
int numNotesPressed = 0;

// binary array that checks whether the light is on or off
unsigned short notelights[NUMPIXELS];
// array that keeps track of the color of each pixel
int ledlights[NUMPIXELS][3];
unsigned int totalRed = 0;
unsigned int totalGreen = 0;
unsigned int totalBlue = 0;
int red = 0;
int green = 0;
int blue = 0;


// the capacitance value of the note strips
long capValueNotes[numNotes];
boolean isPressingNote[numNotes];
long capValueControls[3]; // there 3 control buttons
//boolean isOnControl[3];
int currCtrl = -1;

// making array of arrays just in case we want to be able to configure different note mappings, right now it's useless
int noteMappings[][numNotes] = { {60, 62, 64, 65, 67} };

// color mappings of notes for each channel
// GRB order
int colorMappings[][numNotes][3] = { { {0, 90, 0}, {45, 45, 0}, {90, 0, 0}, {0, 0, 90}, {60, 0, 30} } };

int lightidx = 0;
int capValidx = 0;
int notePressidx = 0;
int ctrlidx = 0;

boolean performanceMode = false;
boolean zippedUp = false;

void setup() {
  //  Set MIDI baud rate:
  Serial.begin(115200);
  pixelsA.begin(); // This initializes the NeoPixel library.
  pixelsB.begin();

  // turn off autocalibrate
  cs_2_3.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_4.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_5.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_6.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_10.set_CS_AutocaL_Millis(0xFFFFFFFF);

  cs_2_11.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_12.set_CS_AutocaL_Millis(0xFFFFFFFF);
  cs_2_13.set_CS_AutocaL_Millis(0xFFFFFFFF);

  pinMode(ZIP, INPUT_PULLUP); // set pin to input

  allOff();
  testMIDI();
}

void allOff() {
  // initialization 
  for (int i = 0; i < numNotes; i++) {
    capValueNotes[i] = 0;
    isPressingNote[i] = false;
  }
  
  // turn off all pixels and set the lights array to 0
  for (int i = 0; i < NUMPIXELS; i++) {
    pixelsA.setPixelColor(i, pixelsA.Color(0, 0, 0));
    pixelsB.setPixelColor(i, pixelsB.Color(0, 0, 0));

    pixelsA.show(); // update LED
    pixelsB.show(); // update LED

    notelights[i] = 0;
  }

  // turn off all the notes
  for (int i = 0; i < 127; i++) {
    for (int chan = 0; chan < 3; chan++) {
      MIDImessage(noteOFF, chan, i, 0);// turn note off
    }
  }
}

// plays a few notes to check the MIDI connection
void testMIDI() {

  MIDImessage(noteON, channel, 60, velocity);// turn note on
  delay(300);// hold note for 300ms
  MIDImessage(noteOFF, channel, 60, velocity);// turn note off
  delay(200);// wait 200ms until triggering next note

  MIDImessage(noteON, channel, 64, velocity);// turn note on
  delay(300);// hold note for 300ms
  MIDImessage(noteOFF, channel, 64, velocity);// turn note off
  delay(200);// wait 200ms until triggering next note

  MIDImessage(noteON, channel, 67, velocity);// turn note on
  delay(300);// hold note for 300ms
  MIDImessage(noteOFF, channel, 67, velocity);// turn note off

}

void loop() {

  MOS_Call(checkPerformance);

  if (performanceMode) {
    // check inputs
    zippedUp = false;
    numNotesPressed = 0;
    totalRed = 0;
    totalGreen = 0;
    totalBlue = 0;

    MOS_Call(checkNote1Touch);
    MOS_Call(checkNote2Touch);
    MOS_Call(checkNote3Touch);
    MOS_Call(checkNote4Touch);
    MOS_Call(checkNote5Touch);

    MOS_Call(checkChannel1Touch);
    MOS_Call(checkChannel2Touch);
    MOS_Call(checkChannel3Touch);

    // make outputs

    MOS_Call(playNote);
    MOS_Call(displayLights);
    MOS_Call(changeControl);
  }

  else if (!zippedUp) {
    zippedUp = true;
    allOff();
  }

}

// unzipped means ready to perform
void checkPerformance(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    performanceMode = (digitalRead(ZIP) == HIGH);
    MOS_Break(tcb);
  }

}

void checkNote1Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueNotes[0] =  cs_2_3.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkNote2Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueNotes[1] =  cs_2_4.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkNote3Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueNotes[2] =  cs_2_5.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkNote4Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueNotes[3] =  cs_2_6.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkNote5Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueNotes[4] =  cs_2_10.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkChannel1Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueControls[0] = cs_2_11.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkChannel2Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueControls[1] = cs_2_12.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}

void checkChannel3Touch(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    capValueControls[2] = cs_2_13.capacitiveSensor(30);
    MOS_Break(tcb);
  }

}


void changeControl(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    for (ctrlidx = 0; ctrlidx < 3; ctrlidx++) {
      if (capValueControls[ctrlidx] > THRESHOLD && currCtrl != ctrlidx) {
        MIDImessage(controlChange, 0, ctrlidx, 100);
        channel = ctrlidx;
        currCtrl = ctrlidx;
        MOS_Break(tcb);
      }
    }

    MOS_Break(tcb);
  }
}

void playNote(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    for (capValidx = 0; capValidx < numNotes; capValidx++) {

      if (capValueNotes[capValidx] > THRESHOLD) {
        if (!isPressingNote[capValidx]) {
          MIDImessage(noteON, channel, noteMappings[0][capValidx], velocity);
          isPressingNote[capValidx] = true;
        }

        numNotesPressed += 1;
        totalRed += colorMappings[0][capValidx][0];
        totalGreen += colorMappings[0][capValidx][1];
        totalBlue += colorMappings[0][capValidx][2];
      }

      else if (capValueNotes[capValidx] <= THRESHOLD && isPressingNote[capValidx]) {
        MIDImessage(noteOFF, channel, noteMappings[0][capValidx], velocity);
        isPressingNote[capValidx] = false;

      }


    }
    MOS_Break(tcb);
  }

}

void displayLights(PTCB tcb) {
  MOS_Continue(tcb);

  while (1) {
    pressingNote = false;
    for (notePressidx = 0; notePressidx < numNotes; notePressidx++) {
      if (isPressingNote[notePressidx]) {
        pressingNote = true;
        break;
      }
    }

    if (pressingNote) notelights[0] = 1;
    else notelights[0] = 0;

    // mixing the colors
    red = totalRed / max(numNotesPressed, 1);
    green = totalGreen / max(numNotesPressed, 1);
    blue = totalBlue / max(numNotesPressed, 1);

    ledlights[0][0] = red;
    ledlights[0][1] = green;
    ledlights[0][2] = blue;

    for (lightidx = 0; lightidx < NUMPIXELS; lightidx++) {

      if (notelights[lightidx] == 1) {

        pixelsA.setPixelColor(lightidx, pixelsA.Color(ledlights[lightidx][0], ledlights[lightidx][1], ledlights[lightidx][2]));
        pixelsB.setPixelColor(lightidx, pixelsB.Color(ledlights[lightidx][0], ledlights[lightidx][1], ledlights[lightidx][2]));

      }

      else {
        pixelsA.setPixelColor(lightidx, pixelsA.Color(0, 0, 0));
        pixelsB.setPixelColor(lightidx, pixelsB.Color(0, 0, 0));

      }

      pixelsA.show(); // send the updated pixel color to the hardware.
      pixelsB.show();


    }

    // shift everything to the right so that everything propagates
    for (lightidx = NUMPIXELS - 1; lightidx > 0; lightidx--) {
      notelights[lightidx] = notelights[lightidx - 1];

      ledlights[lightidx][0] = ledlights[lightidx - 1][0];
      ledlights[lightidx][1] = ledlights[lightidx - 1][1];
      ledlights[lightidx][2] = ledlights[lightidx - 1][2];

    }
    // reset first pixel
    notelights[0] = 0;
    ledlights[lightidx][0] = 0;
    ledlights[lightidx][1] = 0;
    ledlights[lightidx][2] = 0;

    MOS_Delay(tcb, 100);

  }
}

//send MIDI message
void MIDImessage(int command, int channel, int MIDInote, int MIDIvelocity) {

  Serial.write(command + channel);//send note on or note off command
  Serial.write(MIDInote);//send pitch data
  Serial.write(MIDIvelocity);//send velocity data

}



