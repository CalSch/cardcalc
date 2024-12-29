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

#define PI 3.14159265358979323
#define E  2.71828182845904523

#define ANGLE_CONVERT (usingRadians ? 1 : (PI/180.0))

#define NUMBER_TYPE double

#define MENU_LINES 3
#define MENU_Y FONT_HEIGHT*4

// Format IDs
#define DECIMAL 0
#define HEX 1
#define BINARY 2
#define NUMBER_FORMATS 3 // remember to update this count after adding a new format
const char* NUMBER_FORMAT_NAMES[] = {"DEC","HEX","BIN"};

#define DECIMAL_FORMAT "%lf" // either "%f" or "%e", whichever is shortest
#define HEX_FORMAT "0x%X"

#define BINARY_DIGITS 16

#define CURRENT_NUMBER_FORMAT numberFormat==DECIMAL ? DECIMAL_FORMAT : (numberFormat==HEX ? HEX_FORMAT : "uh oh")
#define CURRENT_BASE numberFormat==DECIMAL ? 10 : (numberFormat==HEX ? 16 : (numberFormat == BINARY ? 2 : 0))

struct MenuItem {
  char* name;
  char* value;
  bool noValue;
};

std::vector<char> lastKeysPressed;

NUMBER_TYPE X = 0;
NUMBER_TYPE Y = 0;
NUMBER_TYPE Z = 0;
NUMBER_TYPE T = 0;

NUMBER_TYPE clipboard = 0;

bool decimalMode = false;
bool usingRadians = true;
int numberFormat = DECIMAL;

char chord = 0;

bool showingMenu = false;
MenuItem* menu;
int menuItems = 0;


int vectorFind(std::vector<char> v, char c) {
  for (int i=0;i<v.size();i++) {
    if (v[i]==c)
      return i;
  }
  return -1;
}

NUMBER_TYPE factorial(int n) {
  NUMBER_TYPE f = 1;
  for (int i=1; i<=n; ++i)
    f *= (NUMBER_TYPE)i;
  return f;
}


void printNumber(NUMBER_TYPE num, int y) {
  if (numberFormat == BINARY) {
    char text[BINARY_DIGITS+1];
    for (int i=0;i<BINARY_DIGITS+1;i++) {
      text[BINARY_DIGITS-i-1] = (((int)num >> i) & 1) ? '1' : '0';
    }
    int x = SCREEN_WIDTH-(BINARY_DIGITS*FONT_WIDTH);
    M5Cardputer.Display.drawString(text,x,y);
  } else {
    // find length of string
    int len = snprintf(NULL, 0, CURRENT_NUMBER_FORMAT, (numberFormat == HEX ? (int)num : num)); // convert number to int if in hex mode
    // actually get the string now
    char* text = (char*)malloc(len + 1);
    snprintf(text, len + 1, CURRENT_NUMBER_FORMAT, (numberFormat == HEX ? (int)num : num));
    
    int x = SCREEN_WIDTH-(len*FONT_WIDTH);
    M5Cardputer.Display.drawString(text,x,y);

    free(text);
  }
}

char* get_mode_string() {
  char* text = (char*)malloc(SCREEN_WIDTH/FONT_WIDTH);
  sprintf(text,"%s %s %s Chord: '%c'",
    decimalMode ? "D" : "W",
    usingRadians ? "RAD" : "DEG",
    NUMBER_FORMAT_NAMES[numberFormat],
    (chord!=0) ? chord : ' '
  );
  return text;
}

void initMenu() {
  menu = {};
  menuItems=0;
}
void addMenuItem(MenuItem item) {
  menuItems++;
  MenuItem* newMenu = (MenuItem*)calloc(menuItems,sizeof(MenuItem));
  // memcpy(newMenu,menu,sizeof(menu));
  for (int i=0;i<menuItems-1;i++)
    newMenu[i]=menu[i];
  newMenu[menuItems-1] = item;
  menu = newMenu;
}
void addMenuItem(char c, char* str) {
  char *ptr = (char*)malloc(2*sizeof(char));
  ptr[0] = c;
  ptr[1] = '\0';
  addMenuItem((MenuItem){ptr,str,false});
}
void addMenuItem(char* name, char* value) {
  addMenuItem((MenuItem){name,value,false});
}
void addMenuItem(char* name) {
  addMenuItem((MenuItem){name,"",true});
}
void drawMenu() {
  Serial.println("drawing menu");
  M5Cardputer.Display.setTextColor(GREEN);
  int cols = ceil((float)menuItems/(float)MENU_LINES);

  int x=0;
  // Go through each column, find max name and value widths, and draw
  for (int col=0;col<cols;col++) {
    int nameWidth = 0;
    int valueWidth = 0;
    // Find maximum name and value widths
    for (int i=0;i<MENU_LINES;i++) {
      int idx = col*MENU_LINES + i;
      if (idx >= menuItems) continue;
      MenuItem item = menu[idx];
      if (item.noValue) {
        if (strlen(item.name)>(nameWidth+1+valueWidth+1))
          valueWidth = strlen(item.name);
        continue;
      }
      if (strlen(item.name) > nameWidth) nameWidth = strlen(item.name);
      if (strlen(item.value) > valueWidth) valueWidth = strlen(item.value);
    }
    // Now draw them
    for (int i=0;i<MENU_LINES;i++) {
      int idx = col*MENU_LINES + i;
      if (idx >= menuItems) continue;
      MenuItem item = menu[idx];
      M5Cardputer.Display.drawString(item.name,x*FONT_WIDTH,MENU_Y+i*FONT_HEIGHT);
      M5Cardputer.Display.drawString(item.value,(x+1+nameWidth)*FONT_WIDTH,MENU_Y+i*FONT_HEIGHT);
    }
    x += nameWidth + 1 + valueWidth + 1;
  }
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
  M5Cardputer.Display.drawString(get_mode_string(),1,SCREEN_HEIGHT-FONT_HEIGHT);
  // Serial.print("mode: ");
  // Serial.println(get_mode_string());
  if (showingMenu) drawMenu();
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
  if (chord != 0) { // Handle chords
    switch (chord) {
      case CALC_KEY_CHORD_CONSTANTS:
        switch (key) {
          case CALC_KEY_CONSTANT_PI:
            X = PI;
            break;
          case CALC_KEY_CONSTANT_E:
            X = E;
            break;
        };
        break;
      case CALC_KEY_CHORD_TRIG:
        switch (key) {
          case CALC_KEY_TRIG_SINE:
            X = sin(X*ANGLE_CONVERT);
            break;
          case CALC_KEY_TRIG_COSINE:
            X = cos(X*ANGLE_CONVERT);
            break;
          case CALC_KEY_TRIG_TANGENT:
            X = tan(X*ANGLE_CONVERT);
            break;
        }
        break;
      case CALC_KEY_CHORD_BITWISE:
        switch (key) {
          case CALC_KEY_BITWISE_AND:
            X = (int)Y & (int)X;
            afterOperation();
            break;
          case CALC_KEY_BITWISE_OR:
            X = (int)Y | (int)X;
            afterOperation();
            break;
          case CALC_KEY_BITWISE_XOR:
            X = (int)Y ^ (int)X;
            afterOperation();
            break;
          case CALC_KEY_BITWISE_NOT:
            X = ~((int)X);
            break;
          case CALC_KEY_BITWISE_SHIFT_LEFT:
            X = (int)X << 1;
            break;
          case CALC_KEY_BITWISE_SHIFT_RIGHT:
            X = (int)X >> 1;
            break;
        }
        break;
      case CALC_KEY_CHORD_LOG:
        switch (key) {
          case CALC_KEY_LOG_10:
            X = log10(X);
            break;
          case CALC_KEY_LOG_2:
            X = log2(X);
            break;
          case CALC_KEY_LOG_NATURAL:
            X = log(X);
            break;
          case CALC_KEY_LOG_X:
            X = log(Y) / log(X);
            afterOperation();
            break;
        }
        break;
      case CALC_KEY_CHORD_SETTINGS:
        switch (key) {
          case CALC_KEY_SETTINGS_BRIGHTNESS:
            M5Cardputer.Lcd.setBrightness(X);
            break;
        }
        break;
    }
    chord = 0;
    showingMenu = false;
  } else {
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
        if (!decimalMode) {
          NUMBER_TYPE whole = floor(X);
          X -= whole; // extract whole part of X
          whole *= CURRENT_BASE; // shift to the left
          whole += key - '0'; // add the value represented by the character (kinda weird)
          X += whole; // put the whole part back in X
        } else {
          NUMBER_TYPE fract = X - floor(X);
          Serial.print("fract: ");
          Serial.println(fract);
          X -= fract; // extract fractional part out of X
          fract /= 10; // shift to the right
          fract += (NUMBER_TYPE)(key - '0')/10.f; // add the value represented by the character (kinda weird)
          Serial.print("new fract: ");
          Serial.println(fract);
          X += fract; // put the fractional part back in X
        }
        // todo: make this work with decimals
        break;
      case CALC_KEY_POP:
        shiftDown();
        break;
      case CALC_KEY_SWAP: {
        NUMBER_TYPE temp = X;
        X = Y;
        Y = temp;
        break;
      }
      case CALC_KEY_RANDOM:
        X = (float)rand()/(float)RAND_MAX;
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
      case CALC_KEY_MODULUS:
        X = fmod(Y, X);
        afterOperation();
        break;
      case CALC_KEY_FACTORIAL:
        X = (NUMBER_TYPE)factorial((int)X);
        break;
      case CALC_KEY_NEGATE:
        X = -X;
        break;
      case CALC_KEY_DECIMAL_TOGGLE:
        decimalMode = !decimalMode;
        break;
      case CALC_KEY_ANGLE_TOGGLE:
        usingRadians = !usingRadians;
        break;
      case CALC_KEY_FORMAT_CYCLE:
        numberFormat = (numberFormat+1)%NUMBER_FORMATS;
        break;
      case CALC_KEY_CLEAR:
        X = 0;
        break;
      case CALC_KEY_CLEAR_ALL:
        clearAll();
        break;
      case CALC_KEY_CHORD_CONSTANTS:
        chord = key;
        showingMenu = true;
        initMenu();
        addMenuItem("Constants");
        addMenuItem(CALC_KEY_CONSTANT_PI,"pi");
        addMenuItem(CALC_KEY_CONSTANT_E,"e");
        break;
      case CALC_KEY_CHORD_TRIG:
        chord = key;
        showingMenu = true;
        initMenu();
        addMenuItem("Trigonometry");
        addMenuItem(CALC_KEY_TRIG_SINE,"sin");
        addMenuItem(CALC_KEY_TRIG_COSINE,"cos");
        addMenuItem(CALC_KEY_TRIG_TANGENT,"tan");
        break;
      case CALC_KEY_CHORD_BITWISE:
        chord = key;
        showingMenu = true;
        initMenu();
        addMenuItem("Bitwise");
        addMenuItem(CALC_KEY_BITWISE_AND,"and");
        addMenuItem(CALC_KEY_BITWISE_OR,"or");
        addMenuItem(CALC_KEY_BITWISE_XOR,"xor");
        addMenuItem(CALC_KEY_BITWISE_NOT,"not");
        addMenuItem(CALC_KEY_BITWISE_SHIFT_LEFT,"shift left");
        addMenuItem(CALC_KEY_BITWISE_SHIFT_RIGHT,"shift right");
        break;
      case CALC_KEY_CHORD_LOG:
        chord = key;
        showingMenu = true;
        initMenu();
        addMenuItem("Log");
        addMenuItem(CALC_KEY_LOG_10,"log(X,10)");
        addMenuItem(CALC_KEY_LOG_NATURAL,"ln(X)");
        addMenuItem(CALC_KEY_LOG_2,"log(X,2)");
        addMenuItem(CALC_KEY_LOG_X,"log(Y,X)");
        break;
      case CALC_KEY_CHORD_SETTINGS:
        chord = key;
        showingMenu = true;
        initMenu();
        addMenuItem("Settings");
        addMenuItem(CALC_KEY_SETTINGS_BRIGHTNESS,"brightness = X"); 
        break;
    };
  }
}

void onCtrlKeyPress(char key) {
  switch (key) {
    case CALC_KEY_COPY:
      clipboard = X;
      break;
    case CALC_KEY_PASTE:
      X = clipboard;
      break;
  }
}
void onFnKeyPress(char key) {
  
}
void onOptKeyPress(char key) {
  
}

void onEnterPress() {
  shiftUp();
}
void onDeletePress() {
  X = floor(X/10); // todo: make this work with decimals
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
          if (status.ctrl)
            onCtrlKeyPress(c);
          else if (status.fn)
            onFnKeyPress(c);
          else if (status.opt)
            onOptKeyPress(c);
          else
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
