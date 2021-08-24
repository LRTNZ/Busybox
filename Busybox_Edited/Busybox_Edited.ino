#include <Arduino.h>
#include <Servo.h>
#include <FastLED.h>
/*************RGB LED DEFINES**************/
#define LED_PIN     13
#define NUM_LEDS    16
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB fast_leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
/******************************************/

/* LED Pin Def */

typedef struct {
  uint8_t pinNum;
  bool pinVal;
} ledStruct;

ledStruct leds[22];

const uint8_t LED_START_PIN = 22;
const uint8_t TOTAL_LEDS = 22;

// Enum of all of the names of the LEDS
enum LED_NAME {
  WAIT_LED = 0,
  EDIT_BUT_LED,
  UP_BUT_LED,
  OPT_STOP_BUT_LED,
  ERR_LED,
  LOAD_60_LED,
  LOAD_100_LED,
  LOAD_40_LED,
  OVERTRAVEL_LED,
  POWER_LED,
  MACH_ZERO_LED,
  CYC_START_LED,
  RST_LED,
  COOL_LED,
  LOAD_80_LED,
  LOAD_20_LED,
  TOOL_CHANGE_LED,
  SINGLE_BLOCK_LED,
  DRY_RUN_LED,
  WORK_LED,
  PANIC_LED,
  FEED_HOLD_LED
};

enum LED_STATE {
  LED_OFF = 0,
  LED_ON,
  LED_TOGGLE
};

#define ledstatus 13

#define switch1 45
#define switch2 46
#define switch3 47
#define switch4 48
#define switch5 49
#define switch6 50
#define switch7 51
#define switch8 52
#define switch9 53
#define switch10 69
#define switch11 68
#define switch12 67

#define servozero 85

Servo myservo;  // create servo object to control a servo

int mode; //Create Variable for storing mode 1 = Waiting for reset, 2 = Auto Run Mode, 3 = Manual Run Mode, 4 = E-stop event
int spindlespeed; //Create a variable for storing spindle speed

unsigned long startMillis;  //two variables to hold timing for timing based decisions
unsigned long currentMillis;
const unsigned long flash = 500;  //standard flashing light period

void setup() {

  Serial.begin(9600);
  Serial.println("Serial Working");
  // put your setup code here, to run once:
  myservo.write(90); //Stop Servo
  myservo.attach(12);  // attaches the servo on pin 9 to the servo object
  /********************RGB LED SETUP*************/
  delay( 500 ); // power-up safety delay (was 3000)
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(fast_leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
    
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
  /**********************************************/


  for(int i = 0; i <= TOTAL_LEDS; i++){
    leds[i] = { i + LED_START_PIN, false};
  }

  pinMode(ledstatus, OUTPUT); //LED STATUS ON MEGA BOARD

  pinMode(switch1, INPUT_PULLUP); //E-STOP (INVERTED!!!!!!NC SWITCH)
  pinMode(switch2, INPUT_PULLUP); //OP STOP SWITCH
  pinMode(switch3, INPUT_PULLUP); //CW SWITCH
  pinMode(switch4, INPUT_PULLUP); //CCW SWITCH
  pinMode(switch5, INPUT_PULLUP); //RESET SWITCH (INVERTED!!!!!!NC SWITCH)
  pinMode(switch6, INPUT_PULLUP); //EDIT BUTTON
  pinMode(switch7, INPUT_PULLUP); //AUTO JOG SWITCH
  pinMode(switch8, INPUT_PULLUP); //Machine Zero
  pinMode(switch9, INPUT_PULLUP); //Spindle -
  pinMode(switch10, INPUT_PULLUP); //FEED HOLD SWITCH (INVERTED!!!!!!NC SWITCH)
  pinMode(switch11, INPUT_PULLUP); //CYCLE START
  pinMode(switch12, INPUT_PULLUP); //Spindle +

  /* First Mode Setting wait for reset*/
  startMillis = millis();  //initial start time
  mode = 1;
  digitalWrite(leds[POWER_LED].pinNum, 1); //Turn on Power light
  spindlespeed = 0;   //Set spindle speed variable at zero
  
}

void setLED(ledStruct &led, LED_STATE state){

  switch (state){
    case LED_OFF:
      led.pinVal = 0;
      break;
    case LED_ON:
      led.pinVal = 1;
      break;
    case LED_TOGGLE:
      led.pinVal = !led.pinVal;
      break;
  }
  digitalWrite(led.pinNum, led.pinVal);
}

void loop() {
  // put your main code here, to run repeatedly:
  int val = 0;
  static uint8_t startIndex = 0; //used for LED stripes
  
  currentMillis = millis();
  if (digitalRead(switch1) == 1){     //E-stop pressed do red lights till unpressed
    myservo.write(90);
    while(digitalRead(switch1) == 1){
    SetupBlackPalette();         
    currentBlending = NOBLEND;
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
    alllightson();
    delay(500);
    alllightsoff();
    SetupRedPalette();         
    currentBlending = NOBLEND;
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
    delay(500);
          
    }
    alllightsoff(); //lights off after estop
    mode = 1; //back to waiting for reset
    SetupBlackPalette();         
    currentBlending = NOBLEND;
    FillLEDsFromPaletteColors( startIndex);
    FastLED.show();
  }
  
  if (mode == 1){  //Waiting for reset
    if (currentMillis - startMillis >= flash){       //test whether the period has elapsed
      //digitalWrite(led13, !digitalRead(led13));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(leds[RST_LED], LED_TOGGLE);
      startMillis = currentMillis;  //IMPORTANT to reset the time period start etc.
    }
      
    val = digitalRead(switch5); //if reset is pushed go to mode 2
    if (val == 1){
      mode = 2;
      setLED(leds[RST_LED], LED_OFF); // Turn off Reset Led just in case
    }
    
  }
  if (mode == 2){ //Jog Mode
    if(digitalRead(switch7)==1){
      mode = 3; //If Mode switch is in Auto change modes
    }
    //Update Spindle Control
    if(digitalRead(switch3)==0){ //CW Mode (switch is inverted)
      int spintemp = 90 - (spindlespeed * 5);
      myservo.write(spintemp);
      
    }
    else if(digitalRead(switch4)==0){ //CCW Mode (switch is inverted)
      int spintemp = 90 + (spindlespeed * 5);
      myservo.write(spintemp);
    }
    else {  //Stop spindle
    myservo.write(90);
    }

    if(digitalRead(switch9)==1){  //Spindle +10% button
      if(spindlespeed < 12){
        spindlespeed = spindlespeed + 2;
        delay(150); //bit of button debounce
      }
    }
    
    if(digitalRead(switch12)==1){  //Spindle -10% button
      if(spindlespeed > 0){
        spindlespeed = spindlespeed - 2;
        delay(150); //bit of button debounce
      }
    }

    // Calls the set spindle speed function
    setSpindleLEDs(spindlespeed);
    
    if (currentMillis - startMillis >= flash){       //test whether the period has elapsed
      //digitalWrite(led14, !digitalRead(led14));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(leds[COOL_LED], LED_TOGGLE);
      startMillis = currentMillis;  //IMPORTANT to reset the time period start etc.
    }
    
  }

if (mode == 3){ //Auto Mode
  if(digitalRead(switch7)==0){
      mode = 2; //If Mode switch is in Jog change modes
    }
}
  
}


// Array of the spindle speed LED enums, in the appropriate order to turn them on/off as required
LED_NAME spindleIndicators[] = {LOAD_20_LED, LOAD_40_LED, LOAD_60_LED, LOAD_80_LED, LOAD_100_LED};

void setSpindleLEDs(uint8_t speed){
  
  Serial.print("Set Spindle Speed: ");
  Serial.println(speed);
  // "Static" Preserves variable between function calls
  static uint8_t led_output = 0;
  static byte mask = 1;
  static uint8_t loop_count = 0;

  // Switch that sets the bitmask to turn on the required LEDs, based on the spindle speed.
  switch (speed){
    case 0:
      led_output = 0;
      break;
    case 2:
      led_output = 1;
      break;
    case 4:
      led_output = 3;
      break;
    case 6:
      led_output = 7;
      break;
    case 8:
      led_output = 15;
      break;
    case 10:
      led_output = 31;
      break;
    default:
      break;
  }

  // Use a bitmask to choose what LEDs to turn on and off for the spindle speed control
  mask = 0b00000001;
  for(loop_count = 0; loop_count < 5; loop_count ++){
    if(led_output & mask){
      setLED(leds[spindleIndicators[loop_count]], LED_ON);
    } else {
      setLED(leds[spindleIndicators[loop_count]], LED_OFF);
    }
    mask <<= 1;
  }
}

// Turn all the LEDS on
void alllightson()
{
  for(int i = 0; i < TOTAL_LEDS; i++){
    if(i != 9){ // Don't do power light
      setLED(leds[i], LED_ON);
    }
  }
}

// Turn all the LEDS off
void alllightsoff()
{
  for(int i = 0; i < TOTAL_LEDS; i++){
    if(i != 9){ // Don't do power light
      setLED(leds[i], LED_OFF);
    }
  }
}

// RANDOM FASTLED CODE?

//OLD EXAMPLE CODE FOR LED STRIPES
    //ChangePalettePeriodically();
    //startIndex = startIndex + 1; //motion speed
    //FillLEDsFromPaletteColors( startIndex);
    //FastLED.show();
    //FastLED.delay(1000 / UPDATES_PER_SECOND);

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; i++) {
        fast_leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
        colorIndex += 3;
    }
}


// There are several different palettes of colors demonstrated here.
//
// FastLED provides several 'preset' palettes: RainbowColors_p, RainbowStripeColors_p,
// OceanColors_p, CloudColors_p, LavaColors_p, ForestColors_p, and PartyColors_p.
//
// Additionally, you can manually define your own color palettes, or you can write
// code that creates color palettes on the fly.  All are shown here.

void ChangePalettePeriodically()
{
    uint8_t secondHand = (millis() / 1000) % 60;
    static uint8_t lastSecond = 99;
    
    if( lastSecond != secondHand) {
        lastSecond = secondHand;
        if( secondHand ==  0)  { currentPalette = RainbowColors_p;         currentBlending = LINEARBLEND; }
        if( secondHand == 10)  { currentPalette = RainbowStripeColors_p;   currentBlending = NOBLEND;  }
        if( secondHand == 15)  { currentPalette = RainbowStripeColors_p;   currentBlending = LINEARBLEND; }
        if( secondHand == 20)  { SetupPurpleAndGreenPalette();             currentBlending = LINEARBLEND; }
        if( secondHand == 25)  { SetupTotallyRandomPalette();              currentBlending = LINEARBLEND; }
        if( secondHand == 30)  { SetupBlackAndWhiteStripedPalette();       currentBlending = NOBLEND; }
        if( secondHand == 35)  { SetupBlackAndWhiteStripedPalette();       currentBlending = LINEARBLEND; }
        if( secondHand == 40)  { currentPalette = CloudColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 45)  { currentPalette = PartyColors_p;           currentBlending = LINEARBLEND; }
        if( secondHand == 50)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = NOBLEND;  }
        if( secondHand == 55)  { currentPalette = myRedWhiteBluePalette_p; currentBlending = LINEARBLEND; }
    }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
    for( int i = 0; i < 16; i++) {
        currentPalette[i] = CHSV( random8(), 255, random8());
    }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
    CRGB purple = CHSV( HUE_PURPLE, 255, 255);
    CRGB green  = CHSV( HUE_GREEN, 255, 255);
    CRGB black  = CRGB::Black;
    
    currentPalette = CRGBPalette16(
                                   green,  green,  black,  black,
                                   purple, purple, black,  black,
                                   green,  green,  black,  black,
                                   purple, purple, black,  black );
}


// This example shows how to set up a static color palette
// which is stored in PROGMEM (flash), which is almost always more
// plentiful than RAM.  A static PROGMEM palette like this
// takes up 64 bytes of flash.
const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM =
{
    CRGB::Red,
    CRGB::Gray, // 'white' is too bright compared to red and blue
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Black,
    
    CRGB::Red,
    CRGB::Red,
    CRGB::Gray,
    CRGB::Gray,
    CRGB::Blue,
    CRGB::Blue,
    CRGB::Black,
    CRGB::Black
};



// Additional notes on FastLED compact palettes:
//
// Normally, in computer graphics, the palette (or "color lookup table")
// has 256 entries, each containing a specific 24-bit RGB color.  You can then
// index into the color palette using a simple 8-bit (one byte) value.
// A 256-entry color palette takes up 768 bytes of RAM, which on Arduino
// is quite possibly "too many" bytes.
//
// FastLED does offer traditional 256-element palettes, for setups that
// can afford the 768-byte cost in RAM.
//
// However, FastLED also offers a compact alternative.  FastLED offers
// palettes that store 16 distinct entries, but can be accessed AS IF
// they actually have 256 entries; this is accomplished by interpolating
// between the 16 explicit entries to create fifteen intermediate palette
// entries between each pair.
//
// So for example, if you set the first two explicit entries of a compact 
// palette to Green (0,255,0) and Blue (0,0,255), and then retrieved 
// the first sixteen entries from the virtual palette (of 256), you'd get
// Green, followed by a smooth gradient from green-to-blue, and then Blue.



void SetupBlackPalette()
{
    // 'black out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Black);
     
}

void SetupRedPalette()
{
    // 'red out' all 16 palette entries...
    fill_solid( currentPalette, 16, CRGB::Red);
     
}
