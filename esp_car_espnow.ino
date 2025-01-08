/////////////////////////////////////////////////////////// PACKAGES / LIBRARIES

#include <esp_car.h>
#include <Arducam_Mega.h>

/////////////////////////////////////////////////////////// CONSTANTS

const uint8_t espControllerAddress[] = {0x2C, 0xBC, 0xBB, 0x4B, 0xF6, 0x68};

const uint8_t LED_R_PIN = 32;
const uint8_t LED_G_PIN = 33;
const uint8_t LED_B_PIN = 25;
const uint8_t SPEAKER_CHAN0_PIN = 13;
const uint8_t SPEAKER_CHAN1_PIN = 12;
const uint8_t SPEAKER_CHAN2_PIN = 14;
const uint8_t SERVO_LEFT_PIN = 16;
const uint8_t SERVO_RIGHT_PIN = 15;
const uint8_t CAM_CS_PIN = 5;

/////////////////////////////////////////////////////////// OBJECTS

Arducam_Mega myCAM(CAM_CS_PIN);

/////////////////////////////////////////////////////////// STRUCTS

struct CamData {
  const uint8_t frameChunkStoreSize = 8;
  const uint8_t frameChunkReservedSize = 2;
  const uint8_t frameChunkSize = 128;

  uint8_t qualitySetQueue = 0;
  uint8_t brightnessSetQueue = 0;
  uint8_t contrastSetQueue = 0;

  uint8_t frameChunkNumber = 0;
  uint8_t lastByteRead = 0;
  uint8_t currentStoringIndex = 0;

  // Set in constructor
  uint8_t * frameChunkStored;

  CamData() {
    frameChunkStored = new uint8_t[frameChunkSize];
  }

  ~CamData() {
    delete[] frameChunkStored;
  }
};
CamData camData;

struct Car {
  int vroomTimer = 0;
  int speedScale = 0;  // -100 - 100
  int turnScale = 0;   // -100 - 100
  int leftMid = 300;
  int rightMid = 303;

  uint8_t ledR = 0;
  uint8_t ledG = 0;
  uint8_t ledB = 0;
};
Car car;

struct SoundChannel {
  uint8_t pin;

  short currentHz = -1;
  short setToHz = -1;

  uint8_t currentVelocity = -1;
  uint8_t setToVelocity = -1;

  int playTimer = 0;
};

struct SoundController {
  // For playing array sounds from Python program
  int currentSndIndex = -1;
  unsigned short currentSndBitIndex = 0;
  int currentSndTimer = 0;

  // Channel data
  uint8_t totalChannels = 3;
  SoundChannel ** channelList;
  SoundChannel channel0;
  SoundChannel channel1;
  SoundChannel channel2;

  SoundController() {
    channelList = new SoundChannel*[totalChannels];

    channel0.pin = SPEAKER_CHAN0_PIN;
    channelList[0] = &channel0;
    channel1.pin = SPEAKER_CHAN1_PIN;
    channelList[1] = &channel1;
    channel2.pin = SPEAKER_CHAN2_PIN;
    channelList[2] = &channel2;
  }

  ~SoundController() {
    delete[] channelList;
  }
};
SoundController sndControl;

// 8, 9, 9, 10 <- last 10 is actual 10
// hardware limit is 10hz (i think)
const unsigned short sndBitNoteFreqs[] = { 10, 10, 10, 10, 10, 11, 12, 12, 13, 14, 15, 15, 16, 17, 18, 19, 21, 22, 23, 24, 26, 28, 29, 31, 33, 35, 37, 39, 41, 44, 46, 49, 52, 55, 58, 62, 65, 69, 73, 78, 82, 87, 92, 98, 104, 110, 117, 123, 131, 139, 147, 156, 165, 175, 185, 196, 208, 220, 233, 247, 262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, 523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988, 1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976, 2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951, 4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902, 8372, 8870, 9397, 9956, 10548, 11175, 11840, 12544 };

struct SoundBit {
  uint8_t note;
  uint8_t velocity;
  unsigned short duration;
  unsigned short timeStart;
};
const SoundBit honkHonk[] = { { 76, 100, 96, 0 }, { 78, 100, 96, 96 }, { 76, 100, 96, 288 }, { 78, 100, 96, 384 } };
const SoundBit seaShanty[] = { { 69, 50, 48, 0 }, { 64, 50, 48, 192 }, { 62, 50, 48, 288 }, { 61, 50, 168, 384 }, { 61, 50, 48, 672 }, { 62, 50, 48, 768 }, { 64, 50, 48, 864 }, { 66, 50, 48, 960 }, { 68, 50, 48, 1056 }, { 64, 50, 240, 1152 }, { 66, 50, 48, 1536 }, { 64, 50, 48, 1728 }, { 62, 50, 48, 1824 }, { 61, 50, 48, 1920 }, { 61, 50, 48, 2112 }, { 59, 50, 48, 2304 }, { 61, 50, 48, 2496 }, { 62, 50, 240, 2688 }, { 69, 50, 48, 3072 }, { 64, 50, 48, 3264 }, { 62, 50, 48, 3360 }, { 61, 50, 144, 3456 }, { 61, 50, 48, 3840 }, { 62, 50, 48, 3936 }, { 64, 50, 48, 4032 }, { 66, 50, 48, 4128 }, { 62, 50, 144, 4224 }, { 66, 50, 48, 4608 }, { 64, 50, 48, 4704 }, { 62, 50, 48, 4800 }, { 61, 50, 48, 4896 }, { 59, 50, 48, 4992 }, { 61, 50, 48, 5088 }, { 62, 50, 48, 5184 }, { 66, 50, 48, 5280 }, { 64, 50, 48, 5376 }, { 62, 50, 48, 5472 }, { 61, 50, 48, 5568 }, { 59, 50, 48, 5664 }, { 57, 50, 240, 5760 }, { 61, 50, 48, 6144 }, { 45, 50, 96, 6144 }, { 59, 50, 48, 6240 }, { 61, 50, 48, 6336 }, { 57, 20, 48, 6336 }, { 61, 50, 48, 6528 }, { 40, 50, 96, 6528 }, { 61, 50, 48, 6720 }, { 57, 20, 48, 6720 }, { 59, 50, 48, 6816 }, { 61, 50, 48, 6912 }, { 45, 50, 96, 6912 }, { 59, 50, 48, 7104 }, { 57, 20, 48, 7104 }, { 61, 50, 240, 7296 }, { 33, 50, 96, 7296 }, { 40, 50, 96, 7488 }, { 57, 20, 48, 7488 }, { 57, 50, 48, 7680 }, { 45, 50, 96, 7680 }, { 59, 50, 48, 7776 }, { 61, 50, 48, 7872 }, { 57, 20, 48, 7872 }, { 62, 50, 48, 7968 }, { 61, 50, 48, 8064 }, { 40, 50, 96, 8064 }, { 59, 50, 48, 8256 }, { 57, 20, 48, 8256 }, { 61, 50, 240, 8448 }, { 45, 50, 384, 8448 }, { 57, 20, 48, 8640 }, { 61, 50, 48, 9216 }, { 59, 50, 48, 9312 }, { 61, 50, 48, 9408 }, { 61, 50, 48, 9600 }, { 62, 50, 48, 9792 }, { 61, 50, 48, 9888 }, { 59, 50, 48, 9984 }, { 64, 50, 48, 10176 }, { 61, 50, 144, 10368 }, { 61, 50, 48, 10752 }, { 59, 50, 48, 10848 }, { 61, 50, 48, 10944 }, { 62, 50, 48, 11040 }, { 61, 50, 48, 11136 }, { 57, 50, 48, 11328 }, { 61, 50, 240, 11520 }, { 64, 50, 48, 12288 }, { 62, 50, 48, 12384 }, { 64, 50, 48, 12480 }, { 64, 50, 48, 12672 }, { 64, 50, 48, 12864 }, { 62, 50, 48, 12960 }, { 64, 50, 48, 13056 }, { 66, 50, 48, 13248 }, { 64, 50, 240, 13440 }, { 68, 50, 48, 13824 }, { 68, 50, 48, 14016 }, { 66, 50, 48, 14112 }, { 64, 50, 48, 14208 }, { 62, 50, 48, 14400 }, { 64, 50, 336, 14592 }, { 61, 50, 48, 15360 }, { 59, 50, 48, 15456 }, { 61, 50, 48, 15552 }, { 61, 50, 48, 15744 }, { 59, 50, 48, 15936 }, { 61, 50, 48, 16032 }, { 62, 50, 48, 16128 }, { 59, 50, 48, 16320 }, { 61, 50, 144, 16512 }, { 61, 50, 48, 16896 }, { 59, 50, 48, 16992 }, { 61, 50, 48, 17088 }, { 62, 50, 48, 17184 }, { 61, 50, 48, 17280 }, { 57, 50, 144, 17472 }, { 45, 50, 144, 17664 }, { 82, 50, 48, 18672 } };

struct Sounds {
  const SoundBit * sound;
  unsigned short numElements;
  float bpm;  // bpm default on MIDI is 120
};
Sounds snds[] = {
  { honkHonk, sizeof(honkHonk) / sizeof(SoundBit), 120.0 },
  { seaShanty, sizeof(seaShanty) / sizeof(SoundBit), 90.0 }
};

/////////////////////////////////////////////////////////// FUNCTIONS

bool playSoundBit(int sndIndex, int bitIndex) {
  SoundBit sb = snds[sndIndex].sound[bitIndex];

  for (uint8_t i = 0; i < sndControl.totalChannels; i++) {
    SoundChannel * thisChannel = sndControl.channelList[i];
    if (thisChannel->setToHz == -1) {
      thisChannel->setToHz = sndBitNoteFreqs[sb.note];
      thisChannel->setToVelocity = sb.velocity;
      thisChannel->playTimer = sb.duration * 1000;
      return true;
    }
  }
  return false;
}

void updateSndChannels(unsigned long dt) {
  for (uint8_t i = 0; i < sndControl.totalChannels; i++) {
    SoundChannel * thisChannel = sndControl.channelList[i];

    // Update pwm hz
    if (thisChannel->currentHz != thisChannel->setToHz || thisChannel->currentVelocity != thisChannel->setToVelocity) {
      if (thisChannel->setToHz != -1) {
        float dutyCycle = lerp(0.0, 216.0, (float)thisChannel->setToVelocity / 127.0);
        ledcChangeFrequency(thisChannel->pin, thisChannel->setToHz, 8);
        ledcWrite(thisChannel->pin, (int)dutyCycle);
      }
      else {
        ledcWrite(thisChannel->pin, 0);
      }
      thisChannel->currentHz = thisChannel->setToHz;
      thisChannel->currentVelocity = thisChannel->setToVelocity;
    }

    // Update timer
    if (thisChannel->playTimer > 0) {
      thisChannel->playTimer -= dt;
      if (thisChannel->playTimer <= 0) {
        thisChannel->playTimer = 0;
        thisChannel->setToHz = -1;
      }
    }
  }
}

void updateSnd(unsigned long dt) {
  unsigned long sndModDt = dt;
  // Add midi snd processing
  if (sndControl.currentSndIndex >= 0) {
    Sounds currentSnd = snds[sndControl.currentSndIndex];
    sndModDt = (unsigned long)((float)dt * (currentSnd.bpm / 120.0));
    // Loop notes while their timestart is up
    while (currentSnd.sound[sndControl.currentSndBitIndex].timeStart * 1000 <= sndControl.currentSndTimer) {
      // Check if current snd bit index exceeds the max notes
      if (sndControl.currentSndBitIndex >= currentSnd.numElements) {
        // STOP THE SOUND
        sndControl.currentSndIndex = -1;
        sndControl.currentSndTimer = 0;
        sndControl.currentSndBitIndex = 0;
        break;
      }
      playSoundBit(sndControl.currentSndIndex, sndControl.currentSndBitIndex);
      sndControl.currentSndBitIndex++;
    }

    sndControl.currentSndTimer += sndModDt;
  }

  updateSndChannels(sndModDt);
}

void checkCamEditQueue() {
  if (camData.qualitySetQueue != 0) {
    IMAGE_QUALITY newQuality = static_cast<IMAGE_QUALITY>(camData.qualitySetQueue - 1);
    myCAM.setImageQuality(newQuality);
    camData.qualitySetQueue = 0;
  }
  if (camData.brightnessSetQueue != 0) {
    CAM_BRIGHTNESS_LEVEL newBrightness = static_cast<CAM_BRIGHTNESS_LEVEL>(camData.brightnessSetQueue - 1);
    myCAM.setBrightness(newBrightness);
    camData.brightnessSetQueue = 0;
  }
  if (camData.contrastSetQueue != 0) {
    CAM_CONTRAST_LEVEL newContrast = static_cast<CAM_CONTRAST_LEVEL>(camData.contrastSetQueue - 1);
    myCAM.setContrast(newContrast);
    camData.contrastSetQueue = 0;
  }
}

void updateLED(int r, int g, int b) {
  car.ledR = r;
  car.ledG = g;
  car.ledB = b;

  ledcWrite(LED_R_PIN, r);
  ledcWrite(LED_G_PIN, g);
  ledcWrite(LED_B_PIN, b);
}

/////////////////////////////////////////////////////////// SETUP AND LOOP (MAIN)

// Time variables for loop (SET AT END OF setup)
unsigned long oldTime = 0;
void setup() {
  ledcAttachChannel(LED_R_PIN, 490, 8, 15);
  ledcAttachChannel(LED_G_PIN, 490, 8, 14);
  ledcAttachChannel(LED_B_PIN, 490, 8, 13);
  // 12 attached 13 freq

  ledcAttachChannel(SERVO_RIGHT_PIN, 50, 12, 11);
  ledcAttachChannel(SERVO_LEFT_PIN, 50, 12, 10);

  ledcAttachChannel(SPEAKER_CHAN0_PIN, 490, 8, 8);
  ledcAttachChannel(SPEAKER_CHAN1_PIN, 490, 8, 6);
  ledcAttachChannel(SPEAKER_CHAN2_PIN, 490, 8, 4);

  // 200 - 300 - 400
  ledcWrite(SERVO_LEFT_PIN, 300);
  ledcWrite(SERVO_RIGHT_PIN, 300);
  
  myCAM.begin();
  myCAM.setImageQuality(LOW_QUALITY);

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(1000);

  // Get init ESP-NOW results
  char initEspNowPrintMsgBuf[50];
  bool initEspNowSuccess = initEspNow(espControllerAddress, OnPacketReceived, initEspNowPrintMsgBuf);

  // Print results and stop device setup if result code is an error
  Serial.println(initEspNowPrintMsgBuf);
  if (!initEspNowSuccess) {
    return;
  }
  
  updateLED(255, 255, 255);

  // SET LOOP TIME AT END OF setup
  oldTime = micros();
}

void loop() {
  unsigned long currentTime = micros();
  unsigned long dt = currentTime - oldTime;
  oldTime = currentTime;

  //Serial.println(dt);

  updateSnd(dt);
  
  int leftWrite = car.leftMid;
  int rightWrite = car.rightMid;
  if (car.vroomTimer > 0) {
    car.vroomTimer -= dt;
    if (car.vroomTimer <= 0) {
      car.vroomTimer = 0;
    } else {
      int speedVal = (int)lerp(-100.0, 100.0, (float)(car.speedScale + 100) / 200.0);
      int turnVal = 0;
      if (car.turnScale != 0) {
        turnVal = (int)lerp(2.0, 12.0, (float)abs(car.turnScale) / 100.0);
      }

      if (speedVal == 0) {
        int turnSpeedVal = (int)lerp(-100.0, 100.0, (float)(car.turnScale + 100) / 200.0);
        leftWrite += turnSpeedVal;
        rightWrite += turnSpeedVal;
      } else if (turnVal == 0) {
        leftWrite += speedVal;
        rightWrite -= speedVal;
      } else if (car.turnScale > 0) {
        leftWrite += speedVal;
        rightWrite -= speedVal / turnVal;
      } else if (car.turnScale < 0) {
        leftWrite += speedVal / turnVal;
        rightWrite -= speedVal;
      }
    }
  }
  ledcWrite(SERVO_LEFT_PIN, leftWrite);
  ledcWrite(SERVO_RIGHT_PIN, rightWrite);

  sendCameraFrame();
}

/////////////////////////////////////////////////////////// NETWORKING (ESP-NOW)

void sendCameraFrame() {
  // Check if new frame and no bytes stored
  if (camData.frameChunkNumber == 0 && camData.currentStoringIndex == camData.frameChunkReservedSize) {
    checkCamEditQueue();
    myCAM.takePicture(CAM_IMAGE_MODE_128X128, CAM_IMAGE_PIX_FMT_JPG);
  }

  // If cam has bytes
  if (myCAM.getReceivedLength()) {
    uint8_t amtOfBytesLeftToStore = camData.frameChunkSize - camData.currentStoringIndex;
    uint8_t amtBytesToYoink = (uint8_t)min((uint32_t)amtOfBytesLeftToStore, min((uint32_t)camData.frameChunkStoreSize, myCAM.getReceivedLength()));
    uint8_t yoinkedBytesBuf[amtBytesToYoink];

    myCAM.readBuff(yoinkedBytesBuf, amtBytesToYoink);

    // Check for frame end
    for (int i = 0; i < amtBytesToYoink; i++) {
      uint8_t thisByte = yoinkedBytesBuf[i];
      if (thisByte == 217 && camData.lastByteRead == 255) {
        // Frame is over, kill rest of bytes
        while (myCAM.getReceivedLength()) {
          myCAM.readByte();
        }
        break;
      }
      else {
        camData.lastByteRead = thisByte;
      }
    }

    memcpy(&camData.frameChunkStored[camData.currentStoringIndex], yoinkedBytesBuf, amtBytesToYoink);
    camData.currentStoringIndex += amtBytesToYoink;
  }

  // If stored frame chunk is full or if the bytes were/are cleared then send over what we got
  bool camEmpty = myCAM.getReceivedLength() == 0;
  if (camData.currentStoringIndex >= camData.frameChunkSize || camEmpty) {
    // Send it off into the wild
    esp_now_send(broadcastAddress, camData.frameChunkStored, camData.frameChunkSize);

    // Reset frame chunk storage
    memset(camData.frameChunkStored, 0, camData.frameChunkSize);
    // Set current storing index to the reserved packet slots
    camData.currentStoringIndex = camData.frameChunkReservedSize;

    // Start next frame chunk
    if (camEmpty) {
      camData.frameChunkNumber = 0;
    }
    else {
      camData.frameChunkNumber++;
    }

    // Set reserved slots
    camData.frameChunkStored[0] = 0;
    camData.frameChunkStored[1] = camData.frameChunkNumber;
  }
}

void updateCarValues(const uint8_t * packetData) {
  car.vroomTimer = (int)packetData[1] * 1000;
  car.speedScale = (int)packetData[2] - 100;
  car.turnScale = (int)packetData[3] - 100;
}

void updateRawHornValues(const uint8_t* packetData) {
  int timerToSet = (int)packetData[1] * 1000;

  float newHz = lerp(14.0, 70.0, (float)packetData[2] / 100.0);  // Square root of 200, 5000
  newHz *= newHz;

  for (uint8_t i = 0; i < sndControl.totalChannels; i++) {
    SoundChannel * thisChannel = sndControl.channelList[i];

    thisChannel->playTimer = timerToSet;
    thisChannel->setToHz = (short)(newHz * (i+1));
    thisChannel->setToVelocity = packetData[3];
  }
}

void updateConfigValues(const uint8_t * packetData) {
  switch (packetData[1]) {
    case 0:
      updateLED(packetData[2], car.ledG, car.ledB);
      break;
    case 1:
      updateLED(car.ledR, packetData[2], car.ledB);
      break;
    case 2:
      updateLED(car.ledR, car.ledG, packetData[2]);
      break;
    case 3:
      camData.qualitySetQueue = packetData[2] + 1;
      break;
    case 4:
      if (packetData[2] == 1) {
        camData.brightnessSetQueue = 2;
        camData.contrastSetQueue = 6;
      }
      else {
        camData.brightnessSetQueue = 1;
        camData.contrastSetQueue = 1;
      }
      break;
  }
}

void OnPacketReceived(const esp_now_recv_info_t * espInfo, const uint8_t * packet, int len) {
  // Check if packet from controller address
  for (int i = 0; i < 6; i++) {
    if (espInfo->src_addr[i] != espControllerAddress[i]) {
      return;
    }
  }

  switch (packet[0]) {
    case 0:
      updateCarValues(packet);
      break;
    case 1:
      updateRawHornValues(packet);
      break;
    case 2:
      if (sndControl.currentSndIndex < 0 && packet[1] < sizeof(snds) / sizeof(Sounds)) {
        sndControl.currentSndIndex = (int)packet[1];
      }
      break;
    case 3:
      updateConfigValues(packet);
      break;
  }
}