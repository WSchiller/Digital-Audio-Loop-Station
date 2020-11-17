#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <Bounce.h>
#include "config.h"


// LCD library and initializing LCD object
#ifdef LCD2004
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(0x27, 20, 4);
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
AudioPlaySdRaw           playSdRaw5;
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
Bounce buttonRecord = Bounce(0, 15);
Bounce buttonStop = Bounce(1, 15);
Bounce buttonPlay = Bounce(2, 15);
Bounce buttonSelectTrack = Bounce(27, 15);
Bounce buttonSelectLoop = Bounce(8, 15);
Bounce buttonLoop = Bounce(21, 15);
Bounce buttonPrev = Bounce(34, 15);
Bounce buttonNext = Bounce(39, 15);

// comment out whichever is not in use
// const int myInput = AUDIO_INPUT_LINEIN;
const int myInput = AUDIO_INPUT_MIC;

// File root;
// String tracks[NUM_TRACKS];  // for reading in file names
File frec; // for recording

// TODO: overdub
// 0: idle, 1: record, 2: play, 3: loop
int mode = 0;
#ifdef RECORD_LED
  int recLed = 31;
#endif
#ifdef PLAY_LED
  int playLed = 24;
#endif

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
void nextLoop();
void prevLoop();
void selectLoop();
void selectTrack();
void deselectLoop();
void deselectTrack();

const int NUM_TRACKS = 4;
int filenumber = 0;
int curLoop, curTrack = 0;
int loopNum[5] = {0, 0, 0, 0, 0};
int trackNum[4] = {0, 0, 0, 0}; 
const char * filelist[5][4] = {
  {"RECORD1.RAW", "RECORD2.RAW", "RECORD3.RAW", "RECORD4.RAW"},
  {"RECORD5.RAW", "RECORD6.RAW", "RECORD7.RAW", "RECORD8.RAW"},
  {"RECORD9.RAW", "RECORD10.RAW", "RECORD11.RAW", "RECORD12.RAW"},
  {"RECORD13.RAW", "RECORD14.RAW", "RECORD15.RAW", "RECORD16.RAW"},
  {"RECORD17.RAW", "RECORD18.RAW", "RECORD19.RAW", "RECORD20.RAW"}
};

#ifdef LCD2004
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
        if(loopNum[curLoop] == 1 && trackNum[curTrack] == 1){
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Current Loop:");
          lcd.setCursor(0, 1);
          lcd.print("Loop #");
          lcd.print(curLoop + 1);
          lcd.setCursor(0, 2);
          lcd.print("Current Track:");
          lcd.setCursor(0, 3);
          lcd.print(filename);
          lcd.print("*");
          }
        else if(loopNum[curLoop] == 1 && trackNum[curTrack] == 0) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Current Loop:");
          lcd.setCursor(0, 1);
          lcd.print("Loop #");
          lcd.print(curLoop + 1);
          lcd.setCursor(0, 2);
          lcd.print("Current Track:");
          lcd.setCursor(0, 3);
          lcd.print(filename);
        }
        else {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Current Loop:");
          lcd.setCursor(0, 1);
          lcd.print("Loop #");
          lcd.print(curLoop + 1);      
        }
        break;
    }
  }
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
  pinMode(2, INPUT_PULLUP); // play track
  pinMode(8, INPUT_PULLUP); // select loop
  pinMode(27, INPUT_PULLUP); // select track
  pinMode(21, INPUT_PULLUP); // play loop
  pinMode(34, INPUT_PULLUP); // next
  pinMode(39, INPUT_PULLUP); // prev
  pinMode(recLed, OUTPUT);
  pinMode(playLed, OUTPUT);

  #ifdef LCD2004
    lcd.begin();
    lcd.print("Current Loop:");
    lcd.setCursor(0, 1);
    lcd.print("Loop #");
    lcd.print(curLoop + 1);
  #endif

  delay(500);
}

void loop() {
  buttonRecord.update();
  buttonStop.update();
  buttonPlay.update();
  buttonSelectLoop.update();
  buttonSelectTrack.update();
  buttonLoop.update();
  buttonNext.update();
  buttonPrev.update();

  // Respond to button presses
  if (buttonRecord.fallingEdge()) {
    Serial.println("Record Button Press");
    if (mode == 2) stopPlaying();
    if (mode == 0 && trackNum[curTrack] == 1) startRecording();
  }
  if (buttonStop.fallingEdge()) {
    Serial.println("Stop Button Press");
    Serial.println(mode);
    if (mode == 1) stopRecording();
    if (mode == 2 || mode == 3) stopPlaying();
  }
  if (buttonPlay.fallingEdge()) {
    Serial.println("Play Button Press");
    if (mode == 1) stopRecording();
    if (mode == 0 && trackNum[curTrack] == 1) startPlaying();
  }
  if (buttonSelectLoop.fallingEdge()) {
    if (loopNum[curLoop] == 0) {
       selectLoop();
    } 
    else {
      deselectLoop();
    }
    Serial.println("Select Loop Button Pressed");
  }
  if (buttonSelectTrack.fallingEdge()) {
    if(loopNum[curLoop] == 1){
      if (trackNum[curTrack] == 0) {
        selectTrack();
      }
      else {
        deselectTrack();
      }
    }
  }
  if (buttonLoop.fallingEdge()) {
    Serial.println("Loop Button Press");
    if (mode == 0) startLoop();
  }
  if (buttonNext.fallingEdge()) {
    Serial.println("Next Button Press");
    if (mode == 0 && loopNum[curLoop] == 1) {
      nextTrack();
    }
    else if(mode == 0 && loopNum[curLoop] == 0) {
      nextLoop();
    }
  }
  if (buttonPrev.fallingEdge()) {
    Serial.println("Previous Button Press");
    if (mode == 0 && loopNum[curLoop] == 1){
      prevTrack();
    }
    else if(mode == 0 && loopNum[curLoop] == 0) {
      prevLoop();
    }
  }
  #ifdef LCD2004
    if (buttonRecord.fallingEdge() || buttonStop.fallingEdge() || buttonPlay.fallingEdge() || buttonLoop.fallingEdge() || buttonNext.fallingEdge() || buttonPrev.fallingEdge() || buttonSelectLoop.fallingEdge() || buttonSelectTrack.fallingEdge()){ updateLCD(mode, filelist[curLoop][curTrack]); };
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
  if(mode == 4) {
    startPlaying();
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
  if (SD.exists(filelist[curLoop][curTrack])) {
    SD.remove(filelist[curLoop][curTrack]);
  }
  frec = SD.open(filelist[curLoop][curTrack], FILE_WRITE);
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

  mode = 4;
}

void startPlaying() {
  if(mode == 4) {
    Serial.println("Playback");
  }
  else {
    Serial.println("Start Playing");
  }
  #ifdef PLAY_LED
    digitalWrite(playLed, HIGH);
  #endif
  playSdRaw1.play(filelist[curLoop][curTrack]);
  mode = 2;
}

void continuePlaying() {
  if (!playSdRaw1.isPlaying()) {
    playSdRaw1.stop();
    //mode = 4;
    stopPlaying();
  }
}

void stopPlaying() {
  Serial.println("stopPlaying");
  #ifdef PLAY_LED
    digitalWrite(playLed, LOW);
  #endif
  if (mode == 2 || mode == 4) playSdRaw1.stop();
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
  #ifdef PLAY_LED
    digitalWrite(playLed, HIGH);
  #endif
}

void continueLoop() {
  if(!playSdRaw1.isPlaying() && trackNum[0] == 1) {
    playSdRaw1.play(filelist[curLoop][0]);
    while(playSdRaw1.isPlaying()){
        if (buttonStop.fallingEdge()){
            stopPlaying();
            delay(1000); 
           }
        }
      }
  if(!playSdRaw2.isPlaying() && trackNum[1] == 1) {
    playSdRaw2.play(filelist[curLoop][1]);
    while(playSdRaw2.isPlaying()){
        if (buttonStop.fallingEdge()){
            stopPlaying();
            delay(1000); 
           }
      }
  }
  if(!playSdRaw3.isPlaying() && trackNum[2] == 1) {
    playSdRaw3.play(filelist[curLoop][2]);
    while(playSdRaw3.isPlaying()){
        if (buttonStop.fallingEdge()){
            stopPlaying();
            delay(1000); 
           }
      }
  }
  if(!playSdRaw4.isPlaying() && trackNum[3] == 1) {
    playSdRaw4.play(filelist[curLoop][3]);
    while(playSdRaw4.isPlaying()){
        if (buttonStop.fallingEdge()){
            stopPlaying();
            delay(1000); 
           }
      }
  }
}

void nextTrack() {
    Serial.println("nextTrack");
    curTrack += 1;
    if(curTrack >= 4) {
      curTrack = 0;
    }
}

void nextLoop() {
  if(loopNum[curLoop] == 0 ) {
    Serial.println("nextLoop");
    curLoop += 1;
    if(curLoop >= 5) {
      curLoop = 0;
    }
  }
}

void prevTrack() {
    Serial.println("prevTrack");
    curTrack -= 1;
    if(curTrack <= -1) {
      curTrack = 3;
    }
}

void prevLoop() {
  if(loopNum[curLoop] == 0 ) {
    Serial.println("prevLoop");
    curLoop -= 1;
    if(curLoop <= -1) {
      curLoop = 4;
    }
  }
}

void selectLoop() {
   Serial.print("Loop #");
   Serial.print(curLoop + 1);
   Serial.println(" Selected");
   loopNum[curLoop] = 1;
}

void selectTrack() {
  Serial.print(filelist[curLoop][curTrack]);
  Serial.println(" Selected");
  trackNum[curTrack] = 1;
}

void deselectTrack() {
  Serial.print(filelist[curLoop][curTrack]);
  Serial.println(" Deselected");
  trackNum[curTrack] = 0;
}

void deselectLoop() {
  Serial.print("Loop #");
  Serial.print(curLoop +1);
  Serial.println(" Deselected");
  for(size_t i = 0; i < sizeof(trackNum)/sizeof(int); i++) {
    trackNum[i] = 0;
  }
  loopNum[curLoop] = 0;
}
