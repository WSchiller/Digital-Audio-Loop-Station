#include <Arduino.h>
#include <config.h>
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>

// LCD library and initializing LCD object
#ifdef LCD1602
  #include <LiquidCrystal.h>
  LiquidCrystal lcd(29, 28, 27, 26, 25, 24);
#endif

// Setting up pins for the SD card on the Teensy Audio Shield
#define SDCARD_CS_PIN    10
#define SDCARD_MOSI_PIN  7
#define SDCARD_SCK_PIN   14

// GUItool: begin automatically generated code
// AudioPlaySdWav           playSdWav1;     //xy=226,240
AudioPlaySdRaw           playSdRaw3;     //xy=215,327
AudioPlaySdRaw           playSdRaw2;     //xy=234,268
AudioPlaySdRaw           playSdRaw1;     //xy=236,216
AudioPlaySdRaw           playSdRaw4;     //xy=240,374
AudioInputI2S            i2s1;           //xy=278,77
AudioMixer4              mixer1;         //xy=440,308
AudioAnalyzePeak         peak1;          //xy=484,124
AudioRecordQueue         queue1;         //xy=485,76
AudioOutputI2S           i2s2;           //xy=589,277
AudioConnection          patchCord1(playSdRaw3, 0, mixer1, 2);
AudioConnection          patchCord2(playSdRaw2, 0, mixer1, 1);
AudioConnection          patchCord3(playSdRaw1, 0, mixer1, 0);
AudioConnection          patchCord4(playSdRaw4, 0, mixer1, 3);
AudioConnection          patchCord5(i2s1, 0, queue1, 0);
AudioConnection          patchCord6(i2s1, 0, peak1, 0);
AudioConnection          patchCord7(mixer1, 0, i2s2, 0);
AudioConnection          patchCord8(mixer1, 0, i2s2, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=782,276
// GUItool: end automatically generated code

// Setting up pins for the buttons
Bounce buttonRecord = Bounce(0, 8);
Bounce buttonStop = Bounce(1, 8);
Bounce buttonPlay = Bounce(2, 8);
Bounce buttonSelect = Bounce(3, 8);
Bounce buttonLoop = Bounce(4, 8);
Bounce buttonPrev = Bounce(5, 8);
Bounce buttonNext = Bounce(6, 8);

// comment out whichever is not in use
// const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// prototype
// void getTracks(File);
void startPlaying();
void continuePlaying();
void stopPlaying();
void startRecording();
void continueRecording();
void stopRecording();
void startLoop();
void continueLoop();
void nextTrack();
void prevTrack();
// void select();
// void deselect();
#ifdef LCD1602
  void updateLCD(int mode, String filename="") {
    switch(mode){
      case 1:
        lcd.clear();
        lcd.print("Recording...");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("Playing Track");
        lcd.setCursor(0, 1);
        lcd.print(filename);
        break;
      case 3:
        lcd.clear();
        lcd.print("Looping...");
        break;
      default:
        lcd.setCursor(0, 0);
        lcd.print("Current Track");
        lcd.setCursor(0, 1);
        lcd.print(filename);
        break;;
    }
  }
#endif

const int NUM_TRACKS = 4;
int filenumber = 0;
int curTrack = 0;
int loopTrack[4] = {0, 0, 0, 0};
const char * filelist[4] = {
  "RECORD1.RAW", "RECORD2.RAW", "RECORD3.RAW", "RECORD4.RAW"
};

// File root;
// String tracks[NUM_TRACKS];  // for reading in file names
File frec; // for recording

// TODO: overdub
// 0: idle, 1: record, 2: play, 3: loop
int mode = 0;
#ifdef RECORD_LED
  int recLed = 30;
#endif

void setup() {
  Serial.begin(9600);
  AudioMemory(8);
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(0.5);
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  // root = SD.open("/");
  // getTracks(root);

  pinMode(0, INPUT_PULLUP); // record
  pinMode(1, INPUT_PULLUP); // stop (recording, playing, and looping)
  pinMode(2, INPUT_PULLUP); // play
  pinMode(3, INPUT_PULLUP); // select
  pinMode(4, INPUT_PULLUP); // loop
  pinMode(5, INPUT_PULLUP); // next
  pinMode(6, INPUT_PULLUP); // prev
  pinMode(recLed, OUTPUT);

  #ifdef LCD1602
    lcd.begin(16, 2);
    lcd.print("Current Track");
    lcd.setCursor(0, 1);
    lcd.print(filelist[curTrack]);
  #endif

  delay(500);
}

void loop() {
  buttonRecord.update();
  buttonStop.update();
  buttonPlay.update();
  buttonSelect.update();
  buttonLoop.update();
  buttonNext.update();
  buttonPrev.update();

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");
    if (mode == 2) stopPlaying();
    if (mode == 0) startRecording();
  }
  if (buttonStop.fallingEdge()) {
    Serial.println("Stop Button Press");
    if (mode == 1) stopRecording();
    if (mode == 2 || mode == 3) stopPlaying();
  }
  if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Press");
    if (mode == 1) stopRecording();
    if (mode == 0) startPlaying();
  }
  if (buttonSelect.fallingEdge()) {
    if(loopTrack[curTrack] == 0) {
      loopTrack[curTrack] = 1;
    } else {
      loopTrack[curTrack] = 0;
    }
    Serial.println("Select Button Press");
  }
  if (buttonLoop.fallingEdge()) {
    Serial.println("Loop Button Press");
    if (mode == 0) startLoop();
  }
  if (buttonNext.fallingEdge()) {
    Serial.println("Next Button Press");
    if (mode == 0) nextTrack();
  }
  if (buttonPrev.fallingEdge()) {
    Serial.println("Previous Button Press");
    if (mode == 0) prevTrack();
  }
  #ifdef LCD1602
    if (buttonRecord.fallingEdge() || buttonStop.fallingEdge() || buttonPlay.fallingEdge() || buttonLoop.fallingEdge() || buttonNext.fallingEdge() || buttonPrev.fallingEdge()){ updateLCD(mode, filelist[curTrack]); };
  #endif

  // If we're playing or recording, carry on...
  if (mode == 1) {
    continueRecording();
  }
  if (mode == 2) {
    continuePlaying();
  }
  if (mode == 3) {
    continueLoop();
  }
  
  // volume knob
  int knob = analogRead(A2);
  float vol = (float)knob / 1280.0;
  sgtl5000_1.volume(vol);
}

// void getTracks(File dir) {
//   int idx = 0;
//   while(true){
//     File entry = dir.openNextFile();
//     if(!entry) {break;}
//     // need to change "cppStandard" to "gnu++17"
//     tracks[idx] = strdup(entry.name());
//     entry.close();
//     idx++;
//   }
// }

void startRecording() {
  Serial.println("startRecording");
  #ifdef RECORD_LED
    digitalWrite(recLed, HIGH);
  #endif
  if (SD.exists(filelist[curTrack])) {
    SD.remove(filelist[curTrack]);
  }
  frec = SD.open(filelist[curTrack], FILE_WRITE);
  if (frec) {
    queue1.begin();
    mode = 1;
  }
}

void continueRecording() {
  if (queue1.available() >= 2) {
    byte buffer[512];
    memcpy(buffer, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    memcpy(buffer+256, queue1.readBuffer(), 256);
    queue1.freeBuffer();
    frec.write(buffer, 512);
  }
}

void stopRecording() {
  Serial.println("stopRecording");
  #ifdef RECORD_LED
    digitalWrite(recLed, LOW);
  #endif
  queue1.end();
  if (mode == 1) {
    while (queue1.available() > 0) {
      frec.write((byte*)queue1.readBuffer(), 256);
      queue1.freeBuffer();
    }
    frec.close();
  }
  mode = 0;
}

void startPlaying() {
  Serial.println("startPlaying");
  playSdRaw1.play(filelist[curTrack]);
  mode = 2;
}

void continuePlaying() {
  if (!playSdRaw1.isPlaying()) {
    playSdRaw1.stop();
    mode = 0;
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  if (mode == 2) playSdRaw1.stop();
  if (mode == 3) {
    playSdRaw1.stop();
    playSdRaw2.stop();
    playSdRaw3.stop();
    playSdRaw4.stop();
  }
  mode = 0;
}

void startLoop() {
  Serial.println("looping"); 
  mode = 3;
}

void continueLoop() {
  if(!playSdRaw1.isPlaying() && loopTrack[0] == 1) {
    playSdRaw1.play(filelist[0]);
  }
  if(!playSdRaw2.isPlaying() && loopTrack[1] == 1) {
    playSdRaw2.play(filelist[1]);
  }
  if(!playSdRaw3.isPlaying() && loopTrack[2] == 1) {
    playSdRaw3.play(filelist[2]);
  }
  if(!playSdRaw4.isPlaying() && loopTrack[3] == 1) {
    playSdRaw4.play(filelist[3]);
  }
}

void nextTrack() {
  Serial.println("nextTrack");
  curTrack += 1;
  if(curTrack >= 4) {
    curTrack = 0;
  }
}

void prevTrack() {
  Serial.println("prevTrack");
  curTrack -= 1;
  if(curTrack <= -1) {
    curTrack = 3;
  }
}

// void select() {

// }

// void deselect() {

// }