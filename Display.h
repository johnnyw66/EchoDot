#ifndef _DISPLAY_H
#define _DISPLAY_H
// Display
#define DEFAULT_ROTATION 3

#define TFT_MISO    19      // NC
#define TFT_MOSI    18      // GREY
#define TFT_CLK     5       // PURPLE
#define TFT_DC      4       // BLUE
#define TFT_CS      21      // GREEN
#define TFT_RST     12      // YELLOW

void initDisplay(int rotation,int bl) ;
unsigned long testText(void) ;

void statusmessage(char *str) ;
void setBackLight(int val) ;
void displayMessage(char *str, int x, int y) ;
void displayMessage(char *str, int x, int y, int col) ;
void setTextSize(int sz) ;
void displayClear() ;

// Ada Fruit  TFT/SD ILI9340C Board Pins Order
//Left to Right Looking Down on Display
// BL - NC   (switch to ground) 
// SCK -  PURPLE
// MISO - NC
// MOSI - - GREY
// LCD CS -   GREEN
// SD CS      NC
// RESET      YELLOW
// DS         BLUE
// VIN        WHITE
// GND        BLACK

#endif
