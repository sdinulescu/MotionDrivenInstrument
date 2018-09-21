/*
 * Listening OSC
 * Stejara Dinulescu
 * Program listens for OSC messages from motion capture webcam input and
 * uses it to drive sound
 */


/* CITATIONS: 
 * oscP5sendreceive by andreas schlegel
 * example shows how to send and receive osc messages.
 * oscP5 website at http://www.sojamo.de/oscP5
 *
 * sound example taken from processing.org 
 * examples show how to create sine waves, envelopes, tri oscillator
 * sound website at https://processing.org/tutorials/sound/
 */

//silences -> stopping and starting
//amplitude

import oscP5.*;
import netP5.*;
import processing.sound.*;

OscP5 oscP5;

int DEST_PORT = 8888;
String DEST_HOST = "127.0.0.1";

//Values for Osc
float maxMotion = 0; 
float maxX = 0;
float maxY = 0;

//Sine oscillators -> X and Y square positions passed in
SinOsc[] sWaves;
float[] sFreq;
int numSines = 20;

//Tri oscillator and envelope -> max motion value is passed in 
TriOsc tri;
Env env;
float attackTime = 0.001;
float sustainTime = 0.004;
float sustainLevel = 0.1;
float releaseTime = 0.2;
float duration = 200;
int trigger = 0;
int note = 0;

void setup() {
  size(400, 400);
  background(255);
  frameRate(25);
  
  //set up tri oscillator and envelope
  tri = new TriOsc(this);
  env = new Env(this);
  
  //set up sine oscillators
  sWaves = new SinOsc[numSines];
  sFreq = new float[numSines];
  for (int i = 0; i < numSines; i++) {
    float sVol = (1.0 / numSines) / (i + 1);
    sWaves[i] = new SinOsc(this);
    sWaves[i].play();
    sWaves[i].amp(sVol);
  }

  /* start oscP5, listening for incoming messages at destination port */
  oscP5 = new OscP5(this, DEST_PORT);

  /* myRemoteLocation is a NetAddress. a NetAddress takes 2 parameters,
   * an ip address and a port number. myRemoteLocation is used as parameter in
   * oscP5.send() when sending osc packets to another computer, device, 
   * application. usage see below. for testing purposes the listening port
   * and the port of the remote location address are the same, hence you will
   * send messages back to this sketch.
   */
}

/* incoming osc message are forwarded to the oscEvent method. */
void oscEvent(OscMessage msg) {
  String addr = msg.addrPattern();
  /* print the address pattern and the typetag of the received OscMessage */

  if ( addr.contains("Square") ) {
    maxMotion = msg.get(0).floatValue();
    maxX = msg.get(1).floatValue();
    maxY = msg.get(2).floatValue();
    //println("### received an osc message: " + addr + " " + maxMotion + " " + maxX + " " + maxY);
  }
}

void handleSines() { //function to start and stop sines in certain built-in intervals
  if (frameCount % 240 == 0) {
   for (int i = 0; i < numSines; i++) {
     sWaves[i].stop();
    }
  }
  
  if (frameCount % 480 == 0 ) {
    for (int i = 0; i < numSines; i++) {
      float sVol = (1.0 / numSines) / (i + 1) + 500;
      sWaves[i] = new SinOsc(this);
      sWaves[i].play();
      sWaves[i].amp(sVol);
    }
  }
}

void playSines() {
  float yOffset = 0;
  float detune = 0;

  //max x and max y values from squares, to be passed in sine oscillators
  if (maxX != 0 && maxY != 0) {
    yOffset = map(maxX, 0, height, 0, 1);
    detune = map(maxY, 0, width, -0.5, 0.5);
  } else {
    yOffset = map(mouseY, 0, height, 0, 1); 
    detune = map(mouseY, 0, width, -0.5, 0.5);
  } 

  float freq = pow(1000, yOffset) + 150; //frequency -> from processing documentation

  //sine oscillator (from processing documentation) -> uses x and y positions
  for (int i = 0; i < numSines; i++) {
    float val = freq * (i + 1 * detune);
    sFreq[i] = val;
    sWaves[i].freq(sFreq[i]);
  }
}

void draw() {
  //handleSines(); 
  //playSines();
  
  float mappedX = map(maxX, 0, 500, 0, 1);
  float mappedY = map(maxY, 0, 500, 0, 1);
  
  sustainLevel = sqrt(mappedX*mappedX + mappedY*mappedY); //distance formula between square x and y values changes the sustain time of tri oscillator
  attackTime = mappedY; //square y values change the attackTime

  //"melody" tri oscillator that plays notes based on the max square motion that is passed in
  tri.play( midiToFreq( (int)maxMotion/10000 + 60 ), mappedX  ); //square X values change amplitude
  env.play( tri, attackTime, sustainTime, sustainLevel, releaseTime );
}

float midiToFreq(int note) { //translates to notes (from processing documentation)
  return (pow(2, ((note-69)/12.0)))*440;
}
