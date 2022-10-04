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

ledStruct led[22]; // Struct to hold all the LED structs that are generated for the standard LEDs

const uint8_t LED_START_PIN = 22; // offset to the start pin of all the sequential LED output pins
const uint8_t TOTAL_LEDS = 22; // Total LEDs in the list

// Enum of all of the names of the LEDS - in order of the pin mapping, as all the LEDs have sequential pin numbers.
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

enum LED_STATE // the states for each of the standard LED indicators
{
  LED_OFF = 0,
  LED_ON,
  LED_TOGGLE
};

// Helper method to set an LEDs state
void setLED(ledStruct &led, LED_STATE state)
{

  // Based on the provided state to set the LED to, set it's state flag in the appropriate LEDs struct.
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
  // Make the change to the LED live
  digitalWrite(led.pinNum, led.pinVal);
}

#define ledstatus 13 // Turns on the default onboard status LED

/* Switch Defines */

// Time the library uses for debouncing the inputs
#define DEBOUNCE_TIME 50

// Button Input Pins
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

// Setup all the buttons with the helper library
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

// Servo for the "Spindle"
Servo myservo;        // create servo object to control a servo
#define SERVO_STOP 89 // value the servo will not run at
int spindleSpeed;     // Create a variable for storing spindle speed - the servo valuef

// Overall running mode that the box is currently in
enum MODE
{
  WAITING_RESET,
  MANUAL_RUN,
  AUTO_RUN,
  ESTOP
};

MODE stateMode = WAITING_RESET; // Create Variable for storing mode 1 = Waiting for reset, 2 = Auto Run Mode, 3 = Manual Run Mode, 4 = E-stop event

uint8_t x_axis = 0;
uint8_t y_axis = 0;

unsigned long currentMillis; // Used to store the current millisecond value at the start of each loop to save referencing millis everywhere.

bool coolant_flash = false;


// Default Setup function
void setup()
{

  // Open serial up
  Serial.begin(115200);
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

  // For all of the conventional LEDs, init them into the array of LEDs
  for (int i = 0; i < TOTAL_LEDS; i++)
  {
    // Setup each LED, with it's sequential pinout being stored in it's struct
    led[i] = {i + LED_START_PIN, false};
  }

  pinMode(ledstatus, OUTPUT); // LED STATUS ON MEGA BOARD

  // Start observing all of the buttons
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
  setLED(led[POWER_LED], LED_ON); // Turn on Power light
  spindleSpeed = 0;               // Set spindle speed variable at zero

  // Seed the random number generator, using noisy analog signals from the unconnected analog pins
  randomSeed(analogRead(0) + analogRead(1));

  x_axis = random(0, 8);
  y_axis = random(0, 8);

  // Deal to the random chance of the coolant light coming on
  int coolant_random = random(0, 6);

  if (coolant_random == 5)
  {
    // If the coolant error needs to come on, set that light to start flashing here
    setLED(led[COOL_LED], LED_ON);
    coolant_flash = true;
  }
  
  // Set the servo to it's stop value, to hopefully stop spinning
  myservo.write(SERVO_STOP);

  // Create the first program to use
  generateRandomProgram();
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

    resetFlash = true;

    if (currentMillis - startMillis >= flash)
    { // test whether the period has elapsed
      // digitalWrite(led13, !digitalRead(led13));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(led[RST_LED], LED_TOGGLE);
      startMillis = currentMillis; // IMPORTANT to reset the time period start etc.
    }

    val = rst_but.wasPressed(); // if reset is pushed go to mode Autorun
    if (val == 1)
    {
      setAllResets();
      stateMode = MANUAL_RUN;
      setLED(led[RST_LED], LED_OFF); // Turn off Reset Led just in case
      resetFlash = false;
      generateRandomProgram();       // Generates a new random program.
    }
  }
  if (stateMode == MANUAL_RUN)
  { // Jog Mode
   manualSpindleSpeedCheck();
  }

  if (stateMode == AUTO_RUN)
  { // Auto Mode
    manualSpindleSpeedCheck();
    if (auto_jog_swt.isReleased())
    {
      stateMode = MANUAL_RUN; // If Mode switch is in Jog change modes
    }
  }

  if (cycle_start_but.wasPressed())
  {
    Serial.println("Cycle start pressed");
    if (stateMode == MANUAL_RUN)
    {
      print("Execute Single Block Should Happen");
      randProgram.executeSingle = true;
    }
    else if (stateMode == AUTO_RUN)
    {
      randProgram.executeAuto = true;
    }
  }

  
  

  // Call the program execution function, to run any steps requrired, if any.
  executeProgramAutomatically();
}


// Call to blink lights as required
void callBlinks(){

  static bool anyStandardBlinks = false;
  unsigned long startMillis; // two variables to hold timing for timing based decisions
  
  const unsigned long flash = 500; // standard flashing light period
  

  if (coolant_flash && (stateMode != WAITING_RESET || stateMode != ESTOP))
  {
    // Blink coolant LED if needed
    if (currentMillis - startMillis >= flash)
    { // test whether the period has elapsed
      setLED(led[COOL_LED], LED_TOGGLE);
      startMillis = currentMillis; // IMPORTANT to reset the time period start etc.
    }
  }

  if (currentMillis - startMillis >= flash)
    { // test whether the period has elapsed
      // digitalWrite(led13, !digitalRead(led13));  //if so, change the state of the LED.  Uses a neat trick to change the state
      setLED(led[RST_LED], LED_TOGGLE);
      startMillis = currentMillis; // IMPORTANT to reset the time period start etc.
    }


  if(anyStandardBlinks){
    startMillis = currentMillis;
  }

}



void manualSpindleSpeedCheck(){
   if (auto_jog_swt.isPressed())
    {
      stateMode = AUTO_RUN; // If Mode switch is in Auto change modes
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
}




/* 
   ######################################################
   ######################################################
   ####                                              ####
   ####     Start of internal "GCODE" program code   ####
   ####                                              ####
   ######################################################
   ###################################################### 
*/

   /*  What follows here is the majority of the code for this project. It is all the code that handles 
       generating code for the box to execute, as well as the code that then subsequently calls the different
       parts of it that need to be executed.
   */


/* 
   #############################################
   ##    Self Generated Program Generation    ##
   #############################################
*/       

// Enum that stores all the possible steps for a program to have.
enum PROGRAM_STEPS
{
  BLANK = 0,
  SQUARE_MOVE,
  LINE_MOVE,
  SIDEWAYS_MOVE,
  TOOL_CHANGE,
  OVERTRAVEL,
  COOLANT,
  WAITING,
  OPTIONAL_STOP,
  SPINDLE_OFF,
  SPINDLE_ON,
  END,
  NUMBER_OF_STEPS
};

// An instruction is stored as a single step/block in a program
struct instruction
{
  PROGRAM_STEPS command; // The command that the current instruction has
  int arg1; // Optional arguments to be used by an instruction
  int arg2;
  int arg3;
  int arg4;
  int arg5;
  bool argBool;
};

// The overall program structure
struct program
{
  instruction programSteps[35]; // Array to store all the instances of the instruction struct that are created
  int currentStep = 0; // The current execution step of the program
  unsigned long lastStepMillis = 0; // The time the last step of the current instruction command was executed - needed for multi step commands
  unsigned long nextStepDelay = 0; // The time the next step should be executed at
  bool executeAuto = false; // Currently in auto execution mode
  bool executeSingle = false; // In single block execution mode
  int totalLength = 0; // How many steps the current program was generated to have (Less than the maximum we can store)
  bool optStop = false; // Whether the program has an optional stop or not
  int optStop_index = 0; // Where the opt stop is located if it exists
};

// Where the randomly generated program is stored
program randProgram;

// If the user wants an optional stop to be added
bool optStopDesired = false;

// Function to generate a random sequence of steps to be executed by the busybox
void generateRandomProgram()
{

  // The program currently being generated
  program tempProgram;

  const int PROGRAM_MIN_LENGTH = 10; // Min length of the program to be generated
  const int PROGRAM_MAX_LENGTH = 30; // Keep below initialized length to avoid issues with out of bounds errors

  int lengthProgram = random(PROGRAM_MIN_LENGTH, PROGRAM_MAX_LENGTH); // Decide how long the program is to be
  tempProgram.totalLength = lengthProgram + 1; // The total length the program will be (Count starts at 0)

  int skipOptStopIndex = 0; // The location of the opt stop to be skipped

  // If the user wants an optional stop to be included
  if (optStopDesired)
  {
    skipOptStopIndex = random(3, lengthProgram - 2); // make sure the opt stop isn't at the start of the program
    tempProgram.programSteps[skipOptStopIndex] = instruction{OPTIONAL_STOP}; // put the opt stop into the program at the selected location
  }

  // Start populating the list of commands
  int count = 0;
  
  int randomToolSpindle = random(1,3);

  if(randomToolSpindle == 2){
    tempProgram.programSteps[0] = instruction{TOOL_CHANGE, randomSpindleSpeed()};  
  } else{
    tempProgram.programSteps[0] = instruction{SPINDLE_ON, randomSpindleSpeed()};
    if (1 == random(1, 3))
    {
      tempProgram.programSteps[count] = instruction{COOLANT};
      count++;
    } 
  }
  
  count++; // Inc count after taking up 1 or two places at the start of the code
  
  int previousInstruction = BLANK;

  //  While the count of the number of steps is below the desired program length
  while (count < lengthProgram)
  {
    randomSeed(analogRead(0) + analogRead(1) + analogRead(3));
    int randInstruction = random(1, 6); // Pick a valid auto generatable instruction to execute

    // If the current index already has an instruction in it, increment the count to go to the next valid index
    if (tempProgram.programSteps[count].command == BLANK)
    {
      count++;
      continue;
    }

    // If the current instruction would be the same as the previous one, skip this loop to avoid using the current number
    if (randInstruction == previousInstruction)
    {
      continue;
    }

    // If the program would end to soon with something boring, skip using the current number
    if ((count < 3 && randInstruction == OVERTRAVEL) || (count < PROGRAM_MIN_LENGTH && randInstruction == END))
    {
      continue;
    }

    tempProgram.programSteps[count].command = randInstruction;
    // If the random instruction is a tool change, generate a spindle speed to use for the tool that was changed to.
    if(randInstruction == TOOL_CHANGE){
      tempProgram.programSteps[count].arg1 = randomSpindleSpeed();
    }

    // Add fun stuff for movement commands
    if(randInstruction == SQUARE_MOVE || randInstruction == LINE_MOVE || randInstruction == SIDEWAYS_MOVE){
      bool tempDir = random(0,2);
      bool tempSide = random(0,2);
      uint8_t tempSpeed = random(80,250);
      uint8_t tempLoops = random(1, 8);
      
      switch (randInstruction){
        case SQUARE_MOVE:
          
      } 
    }


    // Keep track of the previous instruction to not do the same thing 30 times in a row
    previousInstruction = randInstruction;

    // Move to the next instruction generation
    count++;
  }

  // At the end of the program, turn off the spindle, and stop the program
  tempProgram.programSteps[count + 1].command = SPINDLE_OFF;
  tempProgram.programSteps[count + 2].command = END;

  // Store the new program
  randProgram = tempProgram;
  // Print out the program for debugging
  printRandomProgram();
}

// Prints out the stored computer generated program
void printRandomProgram()
{
  
  print("Printing Program");
  print("Steps in program: " + String(randProgram.totalLength));
  int count = 0;
  bool run = true;

  // For each of the steps in the program, print out the instruction
  while (run)
  {
    switch (randProgram.programSteps[count].command)
    {
    case END:
      print("end");
      run = false;
      break;
    case WAITING:
      print("Waiting");
      break;
    case TOOL_CHANGE:
      print("Tool Change");
      break;
    case OVERTRAVEL:
      print("Overtravel");
      break;
    case OPTIONAL_STOP:
      print("Optional Stop");
      break;
    case SQUARE_MOVE:
      print("Square Move");
      break;
    case LINE_MOVE:
      print("Line Move");
      break;
    case SIDEWAYS_MOVE:
      print("Sideways Move");
      break;
    case COOLANT:
      print("Coolant");
      break;
    case SPINDLE_OFF:
      print("Spindle Off");
      break;
    case SPINDLE_ON:
      print("Spindle On");
      print(String(randProgram.programSteps[count].arg1));
      break;
    }
    count++;
  }
}


/* 
   ############################################
   ##    Self Generated Program Execution    ##
   ############################################
*/  

instruction currentInstruction; // The current instruction in use (Global scope, so it can be viewed by subroutines as required)
bool finishedInstruction = false; // If the current instruction stored in current instruction has had all of it's steps executed correctly
bool currentlyMoving = false; // To be used to store whether the current instruction is a movement command

// Gets called each loop to execute queued instructions as required
void executeProgramAutomatically()
{

  static bool loadedInstruction = false;
  static bool programStart = true;

  // If the controller has been reset, we need to also reset all of the static variables
  if (resetsToDo[EXECUTE_RST])
  {
    finishedInstruction = false;
    loadedInstruction = false;
    programStart = true;
    resetsToDo[EXECUTE_RST] = false;
  }

  // if it is not time to perform the next step, don't do anything
  if (currentMillis < (randProgram.lastStepMillis + randProgram.nextStepDelay))
  {
    return;
  }

  // If an instruction has not been loaded to be run
  if (!loadedInstruction && !finishedInstruction)
  {
    print("Loaded Instruction");
    // If we are at the first step of the program, and need to load it
    if (randProgram.currentStep == 0 && programStart)
    {
      // Load the next instruction, and set the flag to say that we have not yet started this program
      currentInstruction = randProgram.programSteps[randProgram.currentStep];
      programStart = false;
    }
    // Otherwise if we are just needing to get the next instruction
    else
    {
      // Increment before fetching the instruction, otherwise the current step value will not truly be the current step when referenced elsewhere.
      randProgram.currentStep++;
      currentInstruction = randProgram.programSteps[randProgram.currentStep];
    }
    loadedInstruction = true;
    finishedInstruction = false;
    print("Loaded Instruction number:");
    print(String(randProgram.currentStep));
  }

  // if the single block flag is on
  if (randProgram.executeSingle)
  {
    print("Single step process should happen");
    setLED(led[SINGLE_BLOCK_LED], LED_ON);
  }

  // if it is not time to run a single step, and we are not in automatic mode return
  if (!randProgram.executeSingle && !randProgram.executeAuto)
  {
    // makes sure the single block LED gets turned off
    setLED(led[SINGLE_BLOCK_LED], LED_OFF);
    return;
  }

  // Switch based on the current instruction - let's us call the right instruction subroutine
  switch (currentInstruction.command)
  {

  case SPINDLE_ON:
    // Spindle on to the speed in arg 1
    setSpindleSpeed(currentInstruction.arg1);
    // Only instruction step so fire off the ending of the instruction
    oneShotEnd(750);
    break;
  case SPINDLE_OFF:
    setSpindleSpeed(0);
    oneShotEnd(1500);
    break;
  case TOOL_CHANGE:
    // Call the tool change process each time looped through, so it can step through all it's internal steps
    finishedInstruction = toolChange();
    break;
  case COOLANT:
   // Turn coolant on
    setCoolantState(true);
    // One shot instruction
    oneShotEnd(1000);
    break;
  case SQUARE_MOVE:
    break;
  case LINE_MOVE:
    break;
  case SIDEWAYS_MOVE:
    break;
  case END:
    print("End of program");

    // Resets the various values that need to be reset inside the program
    randProgram.executeAuto = false;
    randProgram.executeSingle = false;
    randProgram.currentStep = 0;
    randProgram.lastStepMillis = 0;
    randProgram.nextStepDelay = 0;
    // Reached the end of the current program
    resetsToDo[EXECUTE_RST] = true;
    break;
  default:
    print("defaulted");
    print(String(currentInstruction.command));
    finishedInstruction = true;
    break;
  }
  
  // If the instruction has finished in the current loop through the code
  if (finishedInstruction)
  {
    // Reset the flags saying we are currently running code
    loadedInstruction = false;
    currentlyMoving = false;
    finishedInstruction = false;
    // If it was a single instruction to execute, we need to set the flag to stop execution
    randProgram.executeSingle = false;
  }
}

// Function to deal with instructions with only one step
void oneShotEnd(int nextDelay)
{
  // Resets flags as needed
  randProgram.nextStepDelay = nextDelay;
  randProgram.lastStepMillis = currentMillis;
  finishedInstruction = true;
}


/* 
   ##############################################
   ##    Self Generated Program Subroutines    ##
   ##############################################
*/  


boolean squareMove(){
  static int step = 0;

  if(resetsToDo[SQUARE_RST]){
    step = 0;
    resetsToDo[SQUARE_RST] = false;
  }

  bool direction = currentInstruction.argBool1;
  // int speed
  // int loops
  return false;
}

boolean sidewaysMove(){
  static int step = 0;
  if(resetsToDo[SIDEWAYS_RST]){
    step = 0;
    resetsToDo[SIDEWAYS_RST] = false;
  }

  bool direction = currentInstruction.argBool1;
  // int speed
  // int loops
  return false;
}

boolean lineMove(){
  static int step = 0;
  if(resetsToDo[LINE_RST]){
    step = 0;
    resetsToDo[LINE_RST] = false;
  }
  bool direction = currentInstruction.argBool1;
  // Int Speed
  // int loops

  // Boolean side
  
  return false;
}

bool toolChange()
{

 // print("Tool Change Steps called");
  static int stepTool = 0;
 // print(String(stepTool));

  if (resetsToDo[TOOL_CHANGE_RST])
  {
    stepTool = 0;
    resetsToDo[TOOL_CHANGE_RST] = false;
  }

  // Actions to do at each step of the toolChange
  switch (stepTool)
  {
  case 0:
    setLED(led[TOOL_CHANGE_LED], LED_ON);
    stepTool++;
    randProgram.lastStepMillis = millis();
    randProgram.nextStepDelay = 250;
    break;
  case 1:
    setCoolantState(false);
    stepTool++;
    randProgram.lastStepMillis = millis();
    randProgram.nextStepDelay = 250;
    break;
  case 2:
    // Turn off spindle
    setSpindleSpeed(0);
    setCoolantState(false);
    stepTool++;
    randProgram.lastStepMillis = millis();
    randProgram.nextStepDelay = 1000;
    break;
  case 3:
     // Will randomly decide if this tool has coolant
     if(1 == random(1, 3)){
      setCoolantState(true);
      randProgram.lastStepMillis = millis();
      randProgram.nextStepDelay = 500;
     }
     stepTool++;
  case 4:
    setLED(led[TOOL_CHANGE_LED], LED_OFF);
    stepTool++;
    randProgram.lastStepMillis = millis();
    randProgram.nextStepDelay = 1000;
    break;
  case 5:
    setSpindleSpeed(currentInstruction.arg1);
    stepTool++;
    randProgram.lastStepMillis = millis();
    break;
  }

  // If we have reached the end of the sequence
  if (stepTool == 6)
  {
    stepTool = 0;
    return true;
  }

  return false;
}

// Turns the coolant on and off, dependant on the provided boolean state. Also handles the Coolant LED
bool setCoolantState(bool state)
{
  LED_STATE toSet = LED_OFF;
  if (state)
    toSet = LED_ON;
  setLED(led[COOL_LED], toSet);
  return true;
}

// Returns a random value to use for the spindle speed
uint8_t randomSpindleSpeed()
{
  return random(1, 10);
}

// Method to set the spindle servo rotation speed
void setSpindleSpeed(uint8_t speedPassed)
{
  print("Set spindle speed called");
  // Update Spindle Control
  if (cw_swt.isPressed() && !ccw_swt.isPressed())
  { // CW Mode (switch is inverted - handled by library)   Need to check for the state of the other switch position, to be able to tell when we are in the stop positiong
    int spintemp = SERVO_STOP + (speedPassed * 5);
    spindleSpeed = spintemp;
     Serial.println("Spindle CW Rotate");
    myservo.write(spintemp);
  }
  else if (ccw_swt.isPressed() && !cw_swt.isPressed())
  { // CCW Mode (switch is inverted - handled by library)
    int spintemp = SERVO_STOP - (speedPassed * 5);
    spindleSpeed = spintemp;
     Serial.println("Servo CCW");
    myservo.write(spintemp);
  }
  
  if(speedPassed == 0)
  { // Stop spindle
    Serial.println("Servo stop");
    myservo.write(SERVO_STOP);
  }

  // Calls the set spindle speed function
  setSpindleLEDs(spindleSpeed);
}

/* 
   ####################################################
   ##    Self Generated Program Subroutine Resets    ##
   ####################################################
*/ 

enum subRoutineResets // List of the subroutines that have static values that will need to be reset between each program run
{
  TOOL_CHANGE_RST,
  EXECUTE_RST,
  SQUARE_RST,
  LINE_RST,
  SIDEWAYS_RST,
  NUMBER_RSTS
};

bool resetsToDo[NUMBER_RSTS]; // Array of the resets that need to be done

// Set all the subroutines to update their internal static values as required when a reset is fired off
void setAllResets()
{
  for (uint8_t i = 0; i < NUMBER_RSTS; i++)
  {
    resetsToDo[i] = true;
  }
  generateRandomProgram(); // Regen the program to be used whenever the system is reset
}

/* 
   ######################################################
   ######################################################
   ####                                              ####
   ####     End of internal "GCODE" program code     ####
   ####                                              ####
   ######################################################
   ###################################################### 
*/


/* 
   ########################
   ##    LED Handling    ##
   ########################
*/  


// Array of the spindle speed LED enums, in the appropriate order to turn them on/off as required
LED_NAME spindleIndicators[] = {LOAD_20_LED, LOAD_40_LED, LOAD_60_LED, LOAD_80_LED, LOAD_100_LED};

void setSpindleLEDs(uint8_t speed)
{

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
      setLED(led[spindleIndicators[loop_count]], LED_ON);
    }
    else
    {
      setLED(led[spindleIndicators[loop_count]], LED_OFF);
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
      setLED(led[i], LED_ON);
    }
  }
}

// Turn all the LEDS off
void alllightsoff()
{
  print("All lights off");
  for (int i = 0; i < TOTAL_LEDS; i++)
  {
    if (i != 9)
    { // Don't do power light
      setLED(led[i], LED_OFF);
    }
  }
}


//Shorter print function helper method
void print(String toPrint)
{
  Serial.println(toPrint);
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
