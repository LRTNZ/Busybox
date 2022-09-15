## Planned Interactions


### Cycle Planning

- All the steps of the cycle are preplanned?
- All random chance events are generated at the beginning
- Create cycles as an object?

- Store actions/events in a stack form, and pop them off the list one by one


### Way to show machine movement

- Use addressable strips LEDS moving up and down, and lighting up either side to show "tool path"
- Increasing and decreasing in brightness on either side in a line to show L <-> movement


### When in auto mode:
 
 - 1/15 chance of an error in *auto mode*, requiring a *reset button* press
 - auto program run time is random between 30 and 45 seconds, with auto spindle speed
 - when *hold feed* pressed, *waiting* LED will illuminate until button is released
 - random chance of *waiting* + *tool change* LED occurring 1 - 3 times in a cycle. Requires the *cycle start* button to be pressed to clear. (*Cycle start* button will blink)
 - when *edit* pressed, followed by *opt stop* being pressed, it adds a stop either 1/3 or 2/3rds of the way into the program cycle.

### When in manual mode:

- Pressing *cycle start* turns on the *single block* LED and runs a 10 - 15 second program
- 1/10 chance of an error requiring a reset

### When in either mode: 

- Having spindle at *off*, and pressing *cycle start* will turn on the *dry run* LED - will run for the appropriate amount of time dependent on the mode
 - if *machine zero* button is not pressed before running when first started up, 1/6 chance of the machine turning on the *Over travel* LED and requiring a *reset* to unlock, followed by *machine zero* to fully satisfy.
 - if *machine zero* is pressed, 1/20 chance of it still erroring out when running 
 - 1/20 chance at startup of the coolant light coming on - Changed to be a 1/6 chance


 Optional stop starts - make opt stop led in button blink

 tool change turn off coolant

 jog mode - press cycle start in jog mode, and it does a random 2 - 3 instructions of the code.
 opt stop waits and blinks the opt stop button when waiting to be cleared
 wait just waits for cycle start to be pressed to carry on
 feed hold pauses execution in both modes
 edit mode lights up when enabled, off when not
 buttons pressed in edit toggle between states on press
 can add machine zero to actions to do (Starts with led bars at random lenghts that move back to zero)
 add random work light being on

 add panic when over travel or spindle switch turned off when running program

 if feed hold then edit is pressed, can add/remove opt stop to the program that is yet to run