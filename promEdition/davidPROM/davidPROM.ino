#include <Adafruit_NeoPixel.h>
#include "SPI.h" // Comment out this line if using Trinket or Gemma

#ifdef __AVR_ATtiny85__
#include <avr/power.h>

#endif

#include <EEPROM.h>

int ADDRESS = 52;

struct PixelData {
  // 0-255
  uint8_t r;
  uint8_t g;
  uint8_t b;
};


int cycleTicks = 300;
uint8_t numModes = 7;


uint8_t currentMode;
uint32_t tick;
int numleds = 22 + 23 + 24 + 23 + 22;
struct PixelData* data;
uint64_t before = 0;
uint64_t now = 0;
const int dataPin  = 6;


Adafruit_NeoPixel strip = Adafruit_NeoPixel(114, dataPin, NEO_RGB + NEO_KHZ800);

//put some Modes here
struct PixelData getColor(uint32_t n) {
  n = n % (256*6);
  switch ((n/256)) {
  case 0:
    return PixelData{255,n,0};
    break;
  case 1:
    return PixelData{511-n,255,0};
    break;
  case 2:
    return PixelData{0,255,n-512};
    break;
  case 3:
    return PixelData{0,1023-n,255};
    break;
  case 4:
    return PixelData{n-1024,0,255};
    break;
  case 5:
    return PixelData{255,0,1535-n};
    break;
  default:
    return PixelData{0,0,0};
    break;
  }
}

uint8_t getLedIndex(uint8_t x, uint8_t y) {
  if (y >= (22 + x) || y >= (26-x)) {//quick bounds check
    return -1;
  }  

  switch (x) {
    case 0:
      return y;
    case 1:
      return 22 + 23 - y - 1;
    case 2:
      return 22 + 23 + y;
    case 3:
      return 22 + 23 + 24 + 23 - y - 1;
    case 4:
      return 22 + 23 + 24 + 23 + y;
    default:  
      return -1;
  }
}

void rainbowCycle(struct PixelData* d, int numleds, uint32_t tick, int speed) {
  PixelData c = getColor(tick*speed);

  for (int i = 0; i < numleds; i++) {
    d[i] = c;
  }
}

void rainbowCascade(struct PixelData* d, int numleds, uint32_t tick, int speed, int clamp) {
  int wheelAlong = (256*6)/clamp;
  int colorInd = (tick * speed) % (256*6); //start with an offset so it cascades
  struct PixelData c;
  for (int i = 0; i < 22; i++) { //first 22 rows (index 0-21) require no safety
    c = getColor(colorInd);
    colorInd += wheelAlong;

    for (int j = 0; j < 5; j++) {
      d[getLedIndex(j, i)] = c;
    }
  }
  //next rows 2 require some checking bounds
  c = getColor(colorInd);
  colorInd += wheelAlong;
  for (int j = 1; j < 4; j++) {
    d[getLedIndex(j,22)] = c;
  }

  c = getColor(colorInd);
  d[getLedIndex(2,23)] = c;
}

const uint8_t promBuffer[] = {0xe, 0xa, 0xe, 0x8, 0x8, 0x0, 0xe, 0xa, 0xc, 0xa, 0xa, 0x0, 0xe, 0xa, 0xa, 0xa, 0xe, 0x0, 0xa, 0xe, 0xa, 0xa, 0xa, 0x0};


void text(struct PixelData* d, int numleds, uint8_t* tb, int bs, uint32_t tick, PixelData c, PixelData bg, int direction, int speed) {
  //https://github.com/idispatch/raster-fonts/blob/master/font-5x8.c
  int offset;
  struct PixelData rain = getColor(tick*speed);
  rain.r = rain.r/3;
  rain.g = rain.g/3;
  rain.b = rain.b/3;
  if (bs > 25) {
    offset = tick % bs;
    if (direction < 0) {
      offset = bs - offset;
    }
  } else {
    offset = 0;
  }
  int index;

  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 5; j++) {
      index = getLedIndex(j,i);
      if (index < 0) {
        continue;
      }

      if ( tb[ (i + offset) % bs ] & (0x10 >> j) ) { //get the jth bit
        d[index] = c;
      } else {
        d[index] = rain;
      }
    }
  }
}

void text(struct PixelData* d, int numleds, uint8_t* tb, int bs, uint32_t tick, PixelData c, PixelData bg, int direction) {
  //https://github.com/idispatch/raster-fonts/blob/master/font-5x8.c
  int offset;

  if (bs > 25) {
    offset = tick % bs;
    if (direction < 0) {
      offset = bs - offset;
    }
  } else {
    offset = 0;
  }
  int index;

  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 5; j++) {
      index = getLedIndex(j,i);
      if (index < 0) {
        continue;
      }

      if ( tb[ (i + offset) % bs ] & (0x10 >> j) ) { //get the jth bit
        d[index] = c;
      } else {
        d[index] = bg;
      }
    }
  }
}


void textComposite(struct PixelData* d, int numleds, uint8_t* tb, int bs, uint32_t tick, PixelData c, PixelData bg, int direction, int speed) {
  //https://github.com/idispatch/raster-fonts/blob/master/font-5x8.c
  int offset;
  if (bs > 25) {
    offset = tick % bs;
    if (direction < 0) {
      offset = bs - offset;
    }
  } else {
    offset = 0;
  }
  int index;

  for (int i = 0; i < 25; i++) {
    for (int j = 0; j < 5; j++) {
      index = getLedIndex(j,i);
      if (index < 0) {
        continue;
      }

      if ( tb[ (i + offset) % bs ] & (0x10 >> j) ) { //get the jth bit
        d[index] = c;
      }
    }
  }
}







void tieCascade(struct PixelData* d, int numleds, uint32_t tick, PixelData c1, PixelData c2, PixelData c3) {
  if (numleds < 114) {
    return;
  }

  //slide colors based on TICK:
  struct PixelData swap;
  if (tick % 3 == 2) {
    swap = c1;
    c1 = c2;
    c2 = c3;
    c3 = swap;
  } else if (tick % 3 == 1) {
    swap = c1;
    c1 = c3;
    c3 = c2;
    c2 = swap;
  }

  //some logic for filling in the leds
  for (int i = 0; i <= 22; i++) {
    d[i] = c1;
  } 
  for (int i = 23; i <= 43; i++) {
    d[i] = c2;
  }
  d[44] = c1;
  d[45] = c1;
  d[46] = c2;
  for (int i = 47; i <= 66; i++) {
    d[i] = c3;
  }
  d[67] = c2;
  d[68] = c1;
  d[69] = c1;
  for (int i = 70; i <= 90; i++) {
    d[i] = c2;
  }
  for (int i = 91; i <= 113; i++) {
    d[i] = c1;
  }
}

void setup() {

  strip.begin();
  strip.show();
//  Serial.begin(9600);
//  Serial.print("start");

  
  tick = 0;
  //currentMode = 2;

  data = malloc(numleds * (sizeof(struct PixelData)));

//  randomSeed(analogRead(0));
//  currentMode = random(numModes);

  //currentMode = EEPROM.read(0) % numModes;
  //EEPROM.write(0, (currentMode + 1) % numModes);
  //currentMode = 6;

}

void updateLEDs(struct PixelData* d, int numleds) {
  for (int i = 0; i < numleds; i++) {
    strip.setPixelColor(i,strip.Color(
      d[i].g/2,
      d[i].r/2,
      d[i].b/2));
  }

  strip.show();
}


void loop() {
  before = millis();

  tick++;

  //handle set increment mode cycling
  //if (tick % cycleTicks == 0) {
  //  currentMode = (currentMode + 1) % numModes;
  //}
  
  /*switch (currentMode) {
    case 0:
      rainbowCycle(data, numleds, tick, 4);
      break;
    case 1:
      pixelChase(data, numleds, tick/4, PixelData{255,0,0}, PixelData{0,0,0});
      break;
    case 2:
      text(data, numleds, promBuffer, 54, tick/4, PixelData{255,255,255}, PixelData{0,0,255}, 1, 16);
      break;
    case 3:
      tieCascade(data, numleds, tick/16, PixelData{255,255,255}, PixelData{0,0,0}, PixelData{0,0,0});
      break;
    case 4:
      rainbowCascade(data, numleds, tick, 5, 100);
      textComposite(data, numleds, promBuffer, 54, tick/4, PixelData{255,255,255}, PixelData{255,0,255}, 1, 16);
      break;
    case 5:
      rainbowCascade(data, numleds, tick, 5, 100);
      break;    
    case 6:*/
      text(data, numleds, promBuffer, 24, tick/4, PixelData{255,255,255}, PixelData{0,0,255}, -1);
      /*break;
  }*/
  updateLEDs(data, numleds);

  now = millis();
  if (now - before < 16) {
    delay(16-(now-before)); //run at 60fps
  }
}
