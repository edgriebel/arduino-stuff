#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

#define BACKLIGHT_PIN     13

//LiquidCrystal_I2C lcd(0x3F);  // Set the LCD I2C address
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
LCD *myLcd = &lcd;

void setup()
{
  // Switch on the backlight
  // pinMode ( BACKLIGHT_PIN, OUTPUT );
  // digitalWrite ( BACKLIGHT_PIN, HIGH );
  myLcd->begin(20,4);               // initialize the lcd 

  myLcd->backlight();

  myLcd->home ();                   // go home
  myLcd->print("Hello, ARDUINO ");  
  myLcd->setCursor ( 0, 1 );        // go to the next line
  myLcd->print (" WORLD!  ");      
}

void loop()
{
  delay(1000);
  myLcd->noBacklight();
  
  delay(500);
  myLcd->backlight();
}
