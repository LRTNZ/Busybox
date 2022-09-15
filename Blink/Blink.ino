#include <Servo.h>
#include <FastLED.h>
/*************RGB LED DEFINES**************/
#define LED_PIN     13
#define NUM_LEDS    16
#define BRIGHTNESS  64
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
/******************************************/

#define led1 22
#define led2 23
#define led3 24
#define led4 25
#define led5 26
#define led6 27
#define led7 28
#define led8 29
#define led9 30
#define led10 31
#define led11 32
#define led12 33
#define led13 34
#define led14 35
#define led15 36
#define led16 37
#define led17 38
#define led18 39
#define led19 40
#define led20 41
#define led21 42
#define led22 43

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

int mode; //Create Variable for storing mode 1 = Waiting for reset, 2 = Auto Run Mode, 3 = Maunal Run Mode, 4 = E-stop event
int spindlespeed; //Create a variable for storing spindle speed

unsigned long startMillis;  //two variables to hold timing for timing based decisions
unsigned long currentMillis;
const unsigned long flash = 500;  //standard flashing light period

void setup() {
  // put your setup code here, to run once:
  myservo.write(90); //Stop Servo
  myservo.attach(12);  // attaches the servo on pin 9 to the servo object
  /********************RGB LED SETUP*************/
  delay( 500 ); // power-up safety delay (was 3000)
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
    
    currentPalette = RainbowColors_p;
    currentBlending = LINEARBLEND;
  /**********************************************/
  pinMode(led1, OUTPUT); //Waiting Light
  pinMode(led2, OUTPUT); //Edit Button Light
  pinMode(led3, OUTPUT); //Spindle Up Button Light
  pinMode(led4, OUTPUT); //OPtional Stop Button Light
  pinMode(led5, OUTPUT); //Error Light
  pinMode(led6, OUTPUT); //60% Load Light 
  pinMode(led7, OUTPUT); //100% Load Light
  pinMode(led8, OUTPUT); //40% Load Light
  pinMode(led9, OUTPUT); //Over Travel Light
  pinMode(led10, OUTPUT); //Power Light
  pinMode(led11, OUTPUT); //Machine Zero Button Light
  pinMode(led12, OUTPUT); //Cycle Start Button Light
  pinMode(led13, OUTPUT); //Reset Button Light
  pinMode(led14, OUTPUT); //Coolant Light 
  pinMode(led15, OUTPUT); //80% Load Light
  pinMode(led16, OUTPUT); //20% Load Light
  pinMode(led17, OUTPUT); //Tool Changer Light
  pinMode(led18, OUTPUT); //Single Block Light
  pinMode(led19, OUTPUT); //Dry Run Light
  pinMode(led20, OUTPUT); //Work Light Light
  pinMode(led21, OUTPUT); //Panic Light
  pinMode(led22, OUTPUT); //Feed Hold Light
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
  digitalWrite(led10, 1); //Turn on Power light
  spindlespeed = 0;   //Set spindle speed variable at zero
  
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
      digitalWrite(led13, !digitalRead(led13));  //if so, change the state of the LED.  Uses a neat trick to change the state
      startMillis = currentMillis;  //IMPORTANT to reset the time period start etc.
    }
      
    val = digitalRead(switch5); //if reset is pushed go to mode 2
    if (val == 1){
      mode = 2;
      digitalWrite(led13, 0); // Turn off Reset Led just in case
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
      if(spindlespeed!=10){
        spindlespeed = spindlespeed + 2;
        delay(150); //bit of button debounce
      }
    }
    
    if(digitalRead(switch12)==1){  //Spindle -10% button
      if(spindlespeed!=0){
        spindlespeed = spindlespeed - 2;
        delay(150); //bit of button debounce
      }
    }

    if(spindlespeed==0){
      digitalWrite(led16, 0); //20% Light
      digitalWrite(led8, 0); //40% Light
      digitalWrite(led6, 0); //60% Light
      digitalWrite(led15, 0); //80% Light
      digitalWrite(led7, 0); //100% Light
    }
    else if(spindlespeed==2){
      digitalWrite(led16, 1); //20% Light
      digitalWrite(led8, 0); //40% Light
      digitalWrite(led6, 0); //60% Light
      digitalWrite(led15, 0); //80% Light
      digitalWrite(led7, 0); //100% Light
    }
    else if(spindlespeed==4){
      digitalWrite(led16, 1); //20% Light
      digitalWrite(led8, 1); //40% Light
      digitalWrite(led6, 0); //60% Light
      digitalWrite(led15, 0); //80% Light
      digitalWrite(led7, 0); //100% Light
    }
    else if(spindlespeed==6){
      digitalWrite(led16, 1); //20% Light
      digitalWrite(led8, 1); //40% Light
      digitalWrite(led6, 1); //60% Light
      digitalWrite(led15, 0); //80% Light
      digitalWrite(led7, 0); //100% Light
    }
    else if(spindlespeed==8){
      digitalWrite(led16, 1); //20% Light
      digitalWrite(led8, 1); //40% Light
      digitalWrite(led6, 1); //60% Light
      digitalWrite(led15, 1); //80% Light
      digitalWrite(led7, 0); //100% Light
    }
    if(spindlespeed==10){
      digitalWrite(led16, 1); //20% Light
      digitalWrite(led8, 1); //40% Light
      digitalWrite(led6, 1); //60% Light
      digitalWrite(led15, 1); //80% Light
      digitalWrite(led7, 1); //100% Light
    }
    
    if (currentMillis - startMillis >= flash){       //test whether the period has elapsed
      digitalWrite(led14, !digitalRead(led14));  //if so, change the state of the LED.  Uses a neat trick to change the state
      startMillis = currentMillis;  //IMPORTANT to reset the time period start etc.
    }
    
  }

if (mode == 3){ //Auto Mode
  if(digitalRead(switch7)==0){
      mode = 2; //If Mode switch is in Jog change modes
    }
}
  
//OLD EXAMPLE CODE FOR LED STRIPES
    //ChangePalettePeriodically();
    //startIndex = startIndex + 1; //motion speed
    //FillLEDsFromPaletteColors( startIndex);
    //FastLED.show();
    //FastLED.delay(1000 / UPDATES_PER_SECOND);

}

void FillLEDsFromPaletteColors( uint8_t colorIndex)
{
    uint8_t brightness = 255;
    
    for( int i = 0; i < NUM_LEDS; i++) {
        leds[i] = ColorFromPalette( currentPalette, colorIndex, brightness, currentBlending);
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



// Additionl notes on FastLED compact palettes:
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

void alllightson()
{
  digitalWrite(led1, 1);
  digitalWrite(led2, 1);
  digitalWrite(led3, 1);
  digitalWrite(led4, 1);
  digitalWrite(led5, 1);
  digitalWrite(led6, 1);
  digitalWrite(led7, 1);
  digitalWrite(led8, 1);
  digitalWrite(led9, 1);
  //digitalWrite(led10, 1); Dont do power light
  digitalWrite(led11, 1);
  digitalWrite(led12, 1);
  digitalWrite(led13, 1);
  digitalWrite(led14, 1);
  digitalWrite(led15, 1);
  digitalWrite(led16, 1);
  digitalWrite(led17, 1);
  digitalWrite(led18, 1);
  digitalWrite(led19, 1);
  digitalWrite(led20, 1);
  digitalWrite(led21, 1);
  digitalWrite(led22, 1); 
}

void alllightsoff()
{
  digitalWrite(led1, 0);
  digitalWrite(led2, 0);
  digitalWrite(led3, 0);
  digitalWrite(led4, 0);
  digitalWrite(led5, 0);
  digitalWrite(led6, 0);
  digitalWrite(led7, 0);
  digitalWrite(led8, 0);
  digitalWrite(led9, 0);
  //digitalWrite(led10, 0); Dont do power light
  digitalWrite(led11, 0);
  digitalWrite(led12, 0);
  digitalWrite(led13, 0);
  digitalWrite(led14, 0);
  digitalWrite(led15, 0);
  digitalWrite(led16, 0);
  digitalWrite(led17, 0);
  digitalWrite(led18, 0);
  digitalWrite(led19, 0);
  digitalWrite(led20, 0);
  digitalWrite(led21, 0);
  digitalWrite(led22, 0);
   
}

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
