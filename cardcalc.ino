#include "M5Cardputer.h"
#include "keys.h"
#include <cstring>
#include <vector>
#include <ctype.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define FONT_WIDTH   8
#define FONT_HEIGHT 16
#define TEXT_COLS SCREEN_WIDTH/FONT_WIDTH
#define TEXT_HEIGHT SCREEN_HEIGHT/FONT_HEIGHT

#define MODE_STRING_SIZE 2

std::vector<char> lastKeysPressed;

float X = 0;
float Y = 1;
float Z = 20;
float T = 142;

bool decimalMode = false;

int vectorFind(std::vector<char>v, char c) {
  for (int i=0;i<v.size();i++) {
    if (v[i]==c)
      return i;
  }
  return -1;
}

void printNumber(float num, int y) {
  int len = snprintf(NULL, 0, "%f", num);
  char* text = (char*)malloc(len + 1);
  snprintf(text, len + 1, "%f", num);
  
  int x = SCREEN_WIDTH-(len*FONT_WIDTH);
  M5Cardputer.Display.drawString(text,x,y);

  free(text);

}

char* get_mode_string() {
  char* text = (char*)malloc(SCREEN_WIDTH/FONT_WIDTH);
  sprintf(text,"Mode: %c",
    decimalMode ? 'D' : 'd'
  );
  return text;
}

void updateScreen() {
  M5Cardputer.Display.clearDisplay();
  M5Cardputer.Display.setTextColor(GREEN);
  M5Cardputer.Display.drawString("T=",0,FONT_HEIGHT*0);
  printNumber(T,FONT_HEIGHT*0);
  M5Cardputer.Display.drawString("Z=",0,FONT_HEIGHT*1);
  printNumber(Z,FONT_HEIGHT*1);
  M5Cardputer.Display.drawString("Y=",0,FONT_HEIGHT*2);
  printNumber(Y,FONT_HEIGHT*2);
  M5Cardputer.Display.drawString("X=",0,FONT_HEIGHT*3);
  printNumber(X,FONT_HEIGHT*3);

  M5Cardputer.Display.fillRect(0,FONT_HEIGHT*4,SCREEN_WIDTH,1,GREEN);

  M5Cardputer.Display.fillRect(0,SCREEN_HEIGHT-FONT_HEIGHT,SCREEN_WIDTH,FONT_HEIGHT,GREEN);
  M5Cardputer.Display.setTextColor(BLACK);
  // M5Cardputer.Display.drawString(":)",0,SCREEN_HEIGHT-FONT_HEIGHT);
  M5Cardputer.Display.drawString(get_mode_string(),0,SCREEN_HEIGHT-FONT_HEIGHT);
  // Serial.print("mode: ");
  // Serial.println(get_mode_string());

}

void clearAll() {
  X=0;
  Y=0;
  Z=0;
  T=0;
}
void shiftDown() {
  X=Y;
  Y=Z;
  Z=T;
}
void shiftUp() {
  T=Z;
  Z=Y;
  Y=X;
  X=0;
}

// shiftDown() but different, T=0 and X isn't changed
void afterOperation() {
  Y=Z;
  Z=T;
  T=0;
  decimalMode = false;
}

void setup() {
  Serial.begin(921600);
  Serial.println("hi");
  auto cfg = M5.config();
  M5Cardputer.begin(cfg,true);

  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextColor(GREEN);
  // M5Cardputer.Display.setTextDatum(left);
  M5Cardputer.Display.setTextFont(&fonts::AsciiFont8x16);
  M5Cardputer.Display.setTextSize(1);
  // M5Cardputer.Display.drawString("hello world",0,0);

  updateScreen();
}

void onKeyPress(char key) {
  switch (key) {
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
    X *= 10; // shift to the right
    X += key - '0'; // add the value represented by the character (kinda weird)
    // todo: make this work with decimals
    break;
  case CALC_KEY_POP: // pop
    shiftDown();
    break;
  case CALC_KEY_ADD:
    X += Y;
    afterOperation();
    break;
  case CALC_KEY_SUBTRACT:
    X = Y - X;
    afterOperation();
    break;
  case CALC_KEY_MULTIPLY:
    X *= Y;
    afterOperation();
    break;
  case CALC_KEY_DIVIDE:
    X = Y / X;
    afterOperation();
    break;
  case CALC_KEY_POWER:
    X = pow(Y, X);
    afterOperation();
    break;
  case CALC_KEY_DECIMAL_TOGGLE:
    decimalMode = !decimalMode;
    break;
  case CALC_KEY_CLEAR:
    X = 0;
    break;
  case CALC_KEY_CLEAR_ALL:
    clearAll();
    break;
    
  };
}
void onEnterPress() {
  shiftUp();
}
void onDeletePress() {
  X = int(X/10); // todo: make this work with decimals
}

void loop() {
  // Serial.println("loop");
  M5Cardputer.update();
  // put your main code here, to run repeatedly:
  // print("hello world! ");
  if (M5Cardputer.Keyboard.isChange()) {
    if (M5Cardputer.Keyboard.isPressed()) {
      Keyboard_Class::KeysState status = M5Cardputer.Keyboard.keysState();

      for (char c : status.word) {
        int idx = vectorFind(lastKeysPressed, tolower(c));
        if (idx==-1) {
          onKeyPress(c);
          lastKeysPressed.push_back(c);
        }
      }
      if (status.enter) {
        onEnterPress();
      }
      if (status.del) {
        onDeletePress();
      }
      updateScreen();
    }
    for (int i=0;i<lastKeysPressed.size();i++) {
      if (!M5Cardputer.Keyboard.isKeyPressed(lastKeysPressed[i])) {
        lastKeysPressed.erase(lastKeysPressed.begin()+i);
      }
    }
  }
  // delay(600);
}
