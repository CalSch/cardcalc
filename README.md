# cardcalc
This is an RPN calculator for the M5 Cardputer. It's currently a work in progress.

## Build & Install
Clone the repo, open it in the Arduino IDE (you can follow this: https://docs.m5stack.com/en/arduino/m5cardputer/program), build it, and deploy it.  
Alternatively, you can download the .bin from the Releases page, then flash it or put it on the SD card if you have a launcher.

## todo
- [x] undo/redo
- [ ] co- trig (secant,cosecant,cotangent, + inverses)
- [ ] save settings to persistent storage
- [ ] help menu
- [ ] alerts (mode bar message that lasts for ~1 second)
- [ ] battery display
- [ ] better number input for decimals
- [ ] expression evaluator
    - [ ] graphing calculator?
- Refactoring
    - [ ] New name for `afterOperation()`
    - [ ] New function just for chord sub-keys
    - [ ] Organize functions
- Non-programming
    - [ ] Get a picture of it on the readme
    - [ ] Finish internal documentation

## Internal-workings (WIP)
This is intended to be the place where I explain the architecture of the project for newcomers (and my future self once come back to this project in 5 months and forget everything)  
Here's how most of it works:
- Display
    - Menus
        - [add stuff]
    - Mode bar
        - It's a bar on the bottom of the screen with dynamic text generated with `sprintf` inside `getModeString()`
        1. (W)hole/(D)ecimal mode:
            - this is a weird number input thing, see the Number input section for more
        2. RAD/DEG (angle format)
        3. DEC/HEX/BIN (number base)
        4. Chord: '*' (current chord, blank if none)
    - Number format
        - [add stuff]
- Input
    - Keyboard input
        1. It checks if the keyboard state changed
        2. It checks if any *new* keys were pressed (this has a weird workaround, using a list of already pressed keys)
        3. For each new key pressed, it checks what modifiers were pressed (ctrl,fn,opt,none) and runs the respective key press function (`onKeyPress(char c)`,`onCtrlKeyPress(char c)`, etc.)
        4. In the key press functions, it checks the key against the actions defined in `keys.h` and runs the respective action
    - Number input
        - When a number key is pressed (or capital A-F in HEX mode), X is shifted left by one digit (according to the current number base), then the number typed is added
        - When in 'Decimal mode' (not to be confused with the DEC number base) only the fractional part is affected, and digits are pushed from left-to-right, the reverse of how they are typically read
            - The whole 'Decimal mode' thing will hopefully be removed when proper number input is added, it's really weird to use
    - Key map
        - In `keys.h` there are `#define`s for every action on the calculator, and the key press functions should usually only use keys from these `#define`s, not hard-coded keys (common exceptions include 0-9 for numbers and A-F in HEX mode)
    - Chords
        - This is my solution to the not-enough-keys problem, by making certain keys (ex. `t`) open up menus where other keys (ex. `s`) can do different actions than normal (ex. `sin(x)`)
        - Chord starter keys are defined in `keys.h` and look like `CALC_KEY_CHORD_[THING]`
            - [Example logarithm key definition](#chord-keymap-definition)
        - Chord starter handling is in the `onKeyPress()` function,  inside the `else` **not** the `if (chord!=0)`
        - Chord sub-keys are defined in their own section in `keys.h` and look like `CALC_KEY_[THING]_[ACTION]`
        - Chord sub-key actions are in the `if (chord!=0)` section in `onKeyPress()`
- Math
    - The 4-variable stack is defined as `X`,`Y`,`Z`, and `T`
        - Which are all of the type `NUMBER_TYPE`, which can be changed if needed. I did this for future proofing, like if `long double`s are needed instead of regular `double`s
    - Operations (defined in key press functions) modify the stack variables
        - The `clearAll()`, `shiftDown()`, and `shiftUp()` functions can be used to modify the stack
        - Operations that take two arguments and have one output (eg. adding, and most 2-var math operations) should use `afterOperation()` (the name could use some work) at the end to shift things down, which erases Y and doesn't affect X (because it's the new result)
        - [Example power operation](#power-operation)
    - Angle modes
        - [add stuff]
- Undo/Redo
    - `HistoryItem`
    - [add stuff]

### Code examples:
#### Chord keymap definition
```c++
// In keys.h

// ...

// Chord starters
#define CALC_KEY_CHORD_LOG       'l'
// ...

// Chord sub-keys
#define CALC_KEY_LOG_10      'l'
#define CALC_KEY_LOG_2       '2'
#define CALC_KEY_LOG_X       'x'
#define CALC_KEY_LOG_NATURAL 'e'
// ...
```

#### Power operation
`X -> Y^X`
```c++
// In cardcalc.ino
void onKeyPress(char key) {
    // ...
    if (chord != 0) {
        // ...
    } else {
        // ...
        switch (key) {
            // ...
            case CALC_KEY_POWER:
                X = pow(Y, X);
                afterOperation();
                break;
            // ...
        }
        // ...
    }
    // ...
}
```