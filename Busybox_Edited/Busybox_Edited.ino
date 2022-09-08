#include <JC_Button.h>

#include <Arduino.h>
#include <Servo.h>
#include <FastLED.h>

/*************RGB LED DEFINES**************/
#define LED_PIN 13
#define NUM_LEDS 16
#define BRIGHTNESS 64
#define LED_TYPE WS2811
#define COLOR_ORDER GRB
CRGB fast_leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100

CRGBPalette16 currentPalette;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 myRedWhiteBluePalette_p PROGMEM;
/******************************************/

/* LED Pin Def */

typedef struct
{
  uint8_t pinNum;
  bool pinVal;
} ledStruct;

ledStruct leds[22];

const uint8_t LED_START_PIN = 22;
const uint8_t TOTAL_LEDS = 22;

// Enum of all of the names of the LEDS
enum LED_NAME
{
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

enum LED_STATE
{
  LED_OFF = 0,
  LED_ON,
  LED_TOGGLE
};

#define ledstatus 13

#define DEBOUNCE_TIME 25

#define ESTOP_SWT 45
#define OP_STOP_BUT 46
#define CW_SWT 47
#define CCW_SWT 48
#define RST_BUT 49
#define EDIT_BUT 50
#define AUTO_JOG_SWT 51
#define MACH_ZERO_BUT 52
#define SPINDLE_MINUS_BUT 53
#define FEED_HOLD_BUT 69
#define CYCLE_START_BUT 68
#define SPINDLE_PLUS_BUT 67

Button estop_swt(ESTOP_SWT, DEBOUNCE_TIME, true, false);                 // E-STOP (INVERTED!!!!!!NC SWITCH)
Button opstop_swt(OP_STOP_BUT, DEBOUNCE_TIME, true, false);              // OP STOP SWITCH
Button cw_swt(CW_SWT, DEBOUNCE_TIME, true, false);                       // CW SWITCH
Button ccw_swt(CCW_SWT, DEBOUNCE_TIME, true, false);                     // CCW SWITCH
Button rst_but(RST_BUT, DEBOUNCE_TIME, true, true);                      // RESET BUTTON (INVERTED!!!!!!NC SWITCH)
Button edit_but(EDIT_BUT, DEBOUNCE_TIME, true, false);                   // EDIT BUTTON
Button auto_jog_swt(AUTO_JOG_SWT, DEBOUNCE_TIME, true, false);           // AUTO JOG SWITCH
Button mach_zero_but(MACH_ZERO_BUT, DEBOUNCE_TIME, true, false);         // Machine Zero
Button spindle_minus_but(SPINDLE_MINUS_BUT, DEBOUNCE_TIME, true, false); // Spindle -
Button feed_hold_but(FEED_HOLD_BUT, DEBOUNCE_TIME, true, true);          // FEED HOLD SWITCH (INVERTED!!!!!!NC SWITCH)
Button cycle_start_but(CYCLE_START_BUT, DEBOUNCE_TIME, true, false);     // CYCLE START
Button spindle_plus_but(SPINDLE_PLUS_BUT, DEBOUNCE_TIME, true, false);   // Spindle +

Servo myservo;        // create servo object to control a servo
#define SERVO_STOP 89 // value the servo will not run at
int spindleSpeed;     // Create a variable for storing spindle speed - the servo valuef

enum MODE
{
  WAITING_RESET,
  MANUAL_RUN,
  AUTO_RUN,
  ESTOP
};

MODE stateMode = WAITING_RESET; // Create Variable for storing mode 1 = Waiting for reset, 2 = Auto Run Mode, 3 = Manual Run Mode, 4 = E-stop event

enum PROGRAM_STEPS
{
  END,
  WAITING,
  TOOL_CHANGE,
  OVERTRAVEL,
  OPTIONAL_STOP,
  SQUARE_MOVE,
  SIDEWAYS_MOVE,
  COOLANT,
  NUMBER_OF_STEPS
};

struct autoRandomProgram
{
  PROGRAM_STEPS programSteps[20];
  int spindleSpeed = 0;
  int currentStep = 0;
};

autoRandomProgram randProgram;


bool optStopDesired = false;


// Function to generate a random sequence of steps to be executed by the busybox
void generateRandomProgram()
{

  randProgram.spindleSpeed = random(2, 12);

  int lengthProgram = random(5, 20);

  int skipOptStopIndex = 0;

  if(optStopDesired){
    skipOptStopIndex = random(2, lengthProgram - 2);
    randProgram.programSteps[skipOptStopIndex] = OPTIONAL_STOP;
  }


  int count = 0;
  int previousInstruction = 255;

  // While the count of the number of steps is below the desired program length
  while(count < lengthProgram){
    int randInstruction = random(0, NUMBER_OF_STEPS);
    
    // If the current index already has an optional stop in it, increment the count to go to the next valid index
    if(randProgram.programSteps[count] == OPTIONAL_STOP){
      count ++;
    }

    // If the current instruction would be the same as the previous one
    if(randInstruction == (previousInstruction && (WAITING || TOOL_CHANGE || OVERTRAVEL || COOLANT))){
      continue;
    }

    // If the program would end to soon with something boring
    if((count < 3 && randInstruction == OVERTRAVEL) || (count < 5 && randInstruction == END)){
      continue;
    }

    

  }
}

unsigned long startMillis; // two variables to hold timing for timing based decisions
unsigned long currentMillis;
const unsigned long flash = 500; // standard flashing light period

bool coolant_flash = false;

void setup()
{

  Serial.begin(9600);
  Serial.println("Serial Working");

  // Deals to servo
  myservo.attach(12); // attaches the servo on pin 9 to the servo object
  myservo.write(95);  // Start servo running so it can be set to it's stop value later on to properly have it not spin.

  /********************RGB LED SETUP*************/
  delay(500); // power-up safety delay (was 3000)
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(fast_leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  currentPalette = RainbowColors_p;
  currentBlending = LINEARBLEND;
  /**********************************************/

  // For all of the conventional LEDs, init them in the array of LEDs
  for (int i = 0; i <= TOTAL_LEDS; i++)
  {
    leds[i] = {i + LED_START_PIN, false};
  }

  pinMode(ledstatus, OUTPUT); // LED STATUS ON MEGA BOARD

  estop_swt.begin();
  opstop_swt.begin();
  cw_swt.begin();
  ccw_swt.begin();
  rst_but.begin();
  edit_but.begin();
  auto_jog_swt.begin();
  mach_zero_but.begin();
  spindle_minus_but.begin();
  feed_hold_but.begin();
  cycle_start_but.begin();
  spindle_plus_but.begin();

  /* First Mode Setting wait for reset*/
  startMillis = millis(); // initial start time
  stateMode = WAITING_RESET;
  setLED(leds[POWER_LED], LED_ON); // Turn on Power light
  spindleSpeed = 0;                // Set spindle speed variable at zero

  // Deal to the random chance of the coolant light coming on

  // Seed the random number generator
  randomSeed(analogRead(0) + analogRead(1));

  long coolant_random = random(0, 6);

  if (coolant_random == 5)
  {
    // If the coolant error needs to come on, set that light to start flashing here
    setLED(leds[COOL_LED], LED_TOGGLE);
    coolant_flash = true;
  }
  Serial.print("Coolant number: ");
  Serial.println(coolant_random);

  myservo.write(SERVO_STOP);
}

void setLED(ledStruct &led, LED_STATE state)
{

  switch (state)
  {
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

void loop()
{

  // Update the read state of all of the defined buttons
  estop_swt.read();
  opstop_swt.read();
  cw_swt.read();
  ccw_swt.read();
  rst_but.read();
  edit_but.read();
  auto_jog_swt.read();
  mach_zero_but.read();
  spindle_minus_but.read();
  feed_hold_but.read();
  cycle_start_but.read();
  spindle_plus_but.read();

  int val = 0;
  static uint8_t startIndex = 0; // used for LED stripes

  currentMillis = millis();
  if (estop_swt.isPressed())
  { // E-stop pressed do red lights till unpressed
    myservo.write(SERVO_STOP);
    unsigned long local_timer = 0;
    bool lights_on = false;
    while (estop_swt.isPressed())
    {

      estop_swt.read();

      if ((local_timer + 500 < millis()) && !lights_on)
      {
        Serial.println("Estop pressed");
        alllightson();
        // control neopixels
        SetupBlackPalette();
        currentBlending = NOBLEND;
        FillLEDsFromPaletteColors(startIndex);
        FastLED.show();

        lights_on = true;
        local_timer = millis();
      }

      if ((local_timer + 500 < millis()) && lights_on)
      {
        alllightsoff();
        SetupRedPalette();
        currentBlending = NOBLEND;
        FillLEDsFromPaletteColors(startIndex);
        FastLED.show();

        lights_on = false;
        local_timer = millis();
      }
    }
    alllightsoff();            // lights off after estop
    stateMode = WAITING_RESET; // back to waiting for reset
    SetupBlackPalette();
    currentBlending = NOBLEND;
    FillLEDsFromPaletteColors(startIndex);
    FastLED.show();
  }

  if (stateMode == WAITING_RESET)
  { // Waiting for reset
    if (currentMillis - startMillis >= flash)
    { // test whether the period has elapsed
      // digitalWrite(led13, !digitalRead(led13));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(leds[RST_LED], LED_TOGGLE);
      startMillis = currentMillis; // IMPORTANT to reset the time period start etc.
    }

    val = rst_but.read(); // if reset is pushed go to mode Autorun
    if (val == 1)
    {
      stateMode = MANUAL_RUN;
      setLED(leds[RST_LED], LED_OFF); // Turn off Reset Led just in case
    }
  }
  if (stateMode == MANUAL_RUN)
  { // Jog Mode
    if (auto_jog_swt.isPressed())
    {
      stateMode = AUTO_RUN; // If Mode switch is in Auto change modes
    }
    // Update Spindle Control
    if (cw_swt.isPressed() && !ccw_swt.isPressed())
    { // CW Mode (switch is inverted - handled by library)   Need to check for the state of the other switch position, to be able to tell when we are in the stop positiong
      int spintemp = SERVO_STOP + (spindleSpeed * 5);
      // Serial.println("Spindle CW Rotate");
      myservo.write(spintemp);
    }
    else if (ccw_swt.isPressed() && !cw_swt.isPressed())
    { // CCW Mode (switch is inverted - handled by library)
      int spintemp = SERVO_STOP - (spindleSpeed * 5);
      // Serial.println("Servo CCW");
      myservo.write(spintemp);
    }
    else
    { // Stop spindle
      // Serial.println("Servo stop");
      myservo.write(SERVO_STOP);
    }

    if (spindle_plus_but.wasPressed())
    { // Spindle +10% button
      if (spindleSpeed < 12)
      {
        spindleSpeed = spindleSpeed + 2;
      }
    }

    if (spindle_minus_but.wasPressed())
    { // Spindle -10% button
      if (spindleSpeed > 0)
      {
        spindleSpeed = spindleSpeed - 2;
      }
    }

    // Calls the set spindle speed function
    setSpindleLEDs(spindleSpeed);
  }

  if (stateMode == AUTO_RUN)
  { // Auto Mode
    if (auto_jog_swt.isReleased())
    {
      stateMode = MANUAL_RUN; // If Mode switch is in Jog change modes
    }
  }

  if (coolant_flash && (stateMode != WAITING_RESET || stateMode != ESTOP))
  {
    // Blink coolant LED if needed
    if (currentMillis - startMillis >= flash)
    { // test whether the period has elapsed
      // digitalWrite(led14, !digitalRead(led14));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(leds[COOL_LED], LED_TOGGLE);
      startMillis = currentMillis; // IMPORTANT to reset the time period start etc.
    }
  }
}

// Array of the spindle speed LED enums, in the appropriate order to turn them on/off as required
LED_NAME spindleIndicators[] = {LOAD_20_LED, LOAD_40_LED, LOAD_60_LED, LOAD_80_LED, LOAD_100_LED};

void setSpindleLEDs(uint8_t speed)
{

  // Serial.print("Set Spindle Speed: ");
  // Serial.println(speed);
  //  "Static" Preserves variable between function calls
  static uint8_t led_output = 0;
  static byte mask = 1;
  static uint8_t loop_count = 0;

  // Switch that sets the bitmask to turn on the required LEDs, based on the spindle speed.
  switch (speed)
  {
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
  for (loop_count = 0; loop_count < 5; loop_count++)
  {
    if (led_output & mask)
    {
      setLED(leds[spindleIndicators[loop_count]], LED_ON);
    }
    else
    {
      setLED(leds[spindleIndicators[loop_count]], LED_OFF);
    }
    mask <<= 1;
  }
}

// Turn all the LEDS on
void alllightson()
{
  for (int i = 0; i < TOTAL_LEDS; i++)
  {
    if (i != 9)
    { // Don't do power light
      setLED(leds[i], LED_ON);
    }
  }
}

// Turn all the LEDS off
void alllightsoff()
{
  for (int i = 0; i < TOTAL_LEDS; i++)
  {
    if (i != 9)
    { // Don't do power light
      setLED(leds[i], LED_OFF);
    }
  }
}

// RANDOM FASTLED CODE?

// OLD EXAMPLE CODE FOR LED STRIPES
// ChangePalettePeriodically();
// startIndex = startIndex + 1; //motion speed
// FillLEDsFromPaletteColors( startIndex);
// FastLED.show();
// FastLED.delay(1000 / UPDATES_PER_SECOND);

void FillLEDsFromPaletteColors(uint8_t colorIndex)
{
  uint8_t brightness = 255;

  for (int i = 0; i < NUM_LEDS; i++)
  {
    fast_leds[i] = ColorFromPalette(currentPalette, colorIndex, brightness, currentBlending);
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

  if (lastSecond != secondHand)
  {
    lastSecond = secondHand;
    if (secondHand == 0)
    {
      currentPalette = RainbowColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 10)
    {
      currentPalette = RainbowStripeColors_p;
      currentBlending = NOBLEND;
    }
    if (secondHand == 15)
    {
      currentPalette = RainbowStripeColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 20)
    {
      SetupPurpleAndGreenPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 25)
    {
      SetupTotallyRandomPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 30)
    {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = NOBLEND;
    }
    if (secondHand == 35)
    {
      SetupBlackAndWhiteStripedPalette();
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 40)
    {
      currentPalette = CloudColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 45)
    {
      currentPalette = PartyColors_p;
      currentBlending = LINEARBLEND;
    }
    if (secondHand == 50)
    {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = NOBLEND;
    }
    if (secondHand == 55)
    {
      currentPalette = myRedWhiteBluePalette_p;
      currentBlending = LINEARBLEND;
    }
  }
}

// This function fills the palette with totally random colors.
void SetupTotallyRandomPalette()
{
  for (int i = 0; i < 16; i++)
  {
    currentPalette[i] = CHSV(random8(), 255, random8());
  }
}

// This function sets up a palette of black and white stripes,
// using code.  Since the palette is effectively an array of
// sixteen CRGB colors, the various fill_* functions can be used
// to set them up.
void SetupBlackAndWhiteStripedPalette()
{
  // 'black out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Black);
  // and set every fourth one to white.
  currentPalette[0] = CRGB::White;
  currentPalette[4] = CRGB::White;
  currentPalette[8] = CRGB::White;
  currentPalette[12] = CRGB::White;
}

// This function sets up a palette of purple and green stripes.
void SetupPurpleAndGreenPalette()
{
  CRGB purple = CHSV(HUE_PURPLE, 255, 255);
  CRGB green = CHSV(HUE_GREEN, 255, 255);
  CRGB black = CRGB::Black;

  currentPalette = CRGBPalette16(
      green, green, black, black,
      purple, purple, black, black,
      green, green, black, black,
      purple, purple, black, black);
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
        CRGB::Black};

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
  fill_solid(currentPalette, 16, CRGB::Black);
}

void SetupRedPalette()
{
  // 'red out' all 16 palette entries...
  fill_solid(currentPalette, 16, CRGB::Red);
}
