#include <Arduino.h>
#include <EEPROM.h>

#define VERSION "1.0"

// Mirror motor values
#define MIRROR_POLL  3      // Seconds to check mirror state
#define MIRROR_MAX_POS 255  // Maximum position mirror should move

// GPIO
#define MIRROR_PIN_1 1      // First digital out pin for HBridge
#define MIRROR_PIN_2 2      // Second digital out pin for HBridge
#define MIRROR_BUTTON 3     // DigitalIN for button change

// EEPROM Values
#define MIRROR_POSIT 1 // Position of mirror, 0 is closed and N is how far it reached, with max_N 

/* ------------------------------------------ */
#define DEBUG 1 // Toggle to switch whether serial will be utilised for debugging
// end definitions
#ifdef DEBUG
    #define debug(x)   Serial.print (x)
    #define debugln(x) Serial.println (x)
#else
    #define debug(x)
    #define debugln(x)
#endif
/* ------------------------------------------ */

// 3 states for the mirrors.
// 0. Mirrors are closed.
// 1. Mirrors are open.
// 2. Mirrors were interrupted.
static uint8_t mirror_state;

// If eeprom fails, we can't continue
static bool eepromFailure   = false;

// Forward declaraton for linker
void moveMirrors(bool);
void updateEEPROM();

void setup() 
{
  #ifdef DEBUG
      Serial.begin(115200);
      Serial.print("Setup: Starting... ");
      Serial.print(__TIME__);
      Serial.print(" ");
      Serial.print(__DATE__);
      Serial.println();
  #endif

  debug("Setup() Version ");
  debugln(VERSION);

  pinMode(MIRROR_PIN_1, OUTPUT);
  pinMode(MIRROR_PIN_2, OUTPUT);

  pinMode(MIRROR_BUTTON, INPUT);

  delay(1000);

  if (!EEPROM.begin(64))
  {
    debugln("Setup() failed to initialise EEPROM"); 
    eepromFailure = true;
    return;
  }

  debugln("Setup() EEPROM setup");

  // Load mirror state from EEPROM
  mirror_state = EEPROM.read(MIRROR_POSIT);

  // Mirrors were interrupted either opening or closing, as such close them.
  if(mirror_state > 0 && mirror_state <= MIRROR_MAX_POS)
  {
    debugln("Setup() Mirrors were stuck, closing.");
    // Close the mirrors.
    moveMirrors(false);
  }
}

void loop() 
{

  // Declare once for use only within this function
  static uint16_t counter = 0;
  
  if(digitalRead(MIRROR_BUTTON) == HIGH && mirror_state <= MIRROR_MAX_POS)
  {
    moveMirrors(true);
  }
  // We need to close the mirrors
  else if(mirror_state >= 0)
  {
    moveMirrors(false);
  }

  // Our 100ms gap between iterations
  delay(100);
  // Let this naturally overflow
  counter++;

}

// Mirror Actions
void moveMirrors(bool open)
{
  bool moveCondition = open ? mirror_state < MIRROR_MAX_POS : mirror_state > 0;
  uint8_t valueShift = open ? 1 : -1;

  while(moveCondition)
  {
      debug("Setup() Moving mirrors, position=");
      debugln(mirror_state);
      digitalWrite(MIRROR_PIN_1, open);
      digitalWrite(MIRROR_PIN_2, !open);

      mirror_state += valueShift;
      updateEEPROM();

      delay(25);
  }

  digitalWrite(MIRROR_PIN_1, LOW);
  digitalWrite(MIRROR_PIN_2, LOW);

  updateEEPROM();
}

// EEPROM Actions
void updateEEPROM()
{
  EEPROM.write(MIRROR_POSIT, mirror_state);
  EEPROM.commit();
}

