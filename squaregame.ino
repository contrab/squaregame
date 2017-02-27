/**
 * Copyright 2017 Thaddeus Jaszek.
 */

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <MelodyPlayer.h>
#include <Sparkle.h>

// Pin Assignments:
#define DIFFICULTY_SELECT_PIN  3
#define SPEAKER_PIN            8
#define LEFT_LED_RED          13
#define LEFT_LED_BLUE         12
#define LEFT_LED_YELLOW       11
#define LEFT_LED_GREEN        10
#define RIGHT_LED_RED          9
#define RIGHT_LED_BLUE         6
#define RIGHT_LED_YELLOW       5
#define RIGHT_LED_GREEN        4

// Parameters etc.
#define BUTTON_COUNT    16
#define NOT_SET         -1
#define EASY_DURATION  620
#define HARD_DURATION  390

// State Machine:
enum programState {
  STATE_INIT = 0,
  STATE_GET_READY,
  STATE_PLAY,
  STATE_RESULTS
};
enum programState state;

// Get ready, get set State Machine:
enum getReadyState {
  RDY_LT_1 = 1,
  RDY_1,
  RDY_LT_2,
  RDY_2,
  RDY_LT_3,
  RDY_3,
  RDY_LT_4,
  RDY_4
};
enum getReadyState readyState;

// Display Results state machine:
enum displayResultsState {
  DISP_INIT = 1,
  DISP_SHOW,
  DISP_WAIT
};
enum displayResultsState resultsState;


// Globals:
LedDef ledList[] = {LedDef(LEFT_LED_RED,     RED,    true, false),
                    LedDef(LEFT_LED_BLUE,    BLUE,   true, false),
                    LedDef(LEFT_LED_YELLOW,  YELLOW, true, true),
                    LedDef(LEFT_LED_GREEN,   GREEN,  true, true),
                    LedDef(RIGHT_LED_RED,    RED,    true, true),
                    LedDef(RIGHT_LED_BLUE,   BLUE,   true, true),
                    LedDef(RIGHT_LED_YELLOW, YELLOW, true, true),
                    LedDef(RIGHT_LED_GREEN,  GREEN,  true, false)};

Adafruit_Trellis matrix = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix);
Sparkle sparkle = Sparkle(ledList);

short id = NOT_SET;
bool chooseNext = true;
unsigned long startTime = 0;
unsigned long duration = EASY_DURATION;
short pointsFor = 0;
short pointsAgainst = 0;
unsigned long throttle = 0;
bool resultsFirstTime = true;

// Sound Effects:
onenote success_notes[] = {{NOTE_C5, 6}, {NOTE_E5, 12}};
MelodyPlayer success(SPEAKER_PIN, success_notes);
onenote miss_notes[] = {{NOTE_A2, 8}};
MelodyPlayer miss(SPEAKER_PIN, miss_notes);
onenote low_notes[] = {{NOTE_C4, 3}, {0, 8}};
MelodyPlayer low(SPEAKER_PIN, low_notes);
onenote high_notes[] = {{NOTE_C6, 2}, {0, 8}};
MelodyPlayer high(SPEAKER_PIN, high_notes);
onenote youwin_notes[] = {{0, 4}, {NOTE_C5, 4}, {NOTE_C5, 8}, {NOTE_C5, 8}, {NOTE_C5, 4},
                          {NOTE_E5, 4}, {NOTE_C5, 4}, {NOTE_E5, 4}, {NOTE_G5, 2}};
MelodyPlayer youwin(SPEAKER_PIN, youwin_notes);
onenote youlose_notes[] = {{0, 4}, {NOTE_C3, 2}, {NOTE_C3, 4}, {NOTE_C3, 4}, {NOTE_C3, 2},
                           {NOTE_DS3, 4}, {NOTE_D3, 4}, {NOTE_D3, 4}, {NOTE_C3, 4}, {NOTE_C3, 4}, {NOTE_B2, 4}, {NOTE_C3, 2}};
MelodyPlayer youlose(SPEAKER_PIN, youlose_notes);

// Functions:

/**
 * clearAll clears all LEDs.
 */
 void clearAll() {
  for (int i=0; i<BUTTON_COUNT; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();
 }

/**
 * initNextGame handles STATE_INIT. Initialize for the next game.
 */
void initNextGame() {
  // Ensure all the LEDs are out
  clearAll();

  // Reset sounds
  success.Pause();
  success.FromTheTop();
  miss.Pause();
  miss.FromTheTop();
  youwin.Pause();
  youwin.FromTheTop();
  youlose.Pause();
  youlose.FromTheTop();

  // No LED is set.
  id = NOT_SET;
  chooseNext = true;

  // Get input from user how hard/easy to make the game.
  if (HIGH == digitalRead(DIFFICULTY_SELECT_PIN)) {
    duration = HARD_DURATION;
  } else {
    duration = EASY_DURATION;
  }

  // Reset the score.
  pointsFor = 0;
  pointsAgainst = 0;

  // Tell the game player to get ready.
  readyState = RDY_LT_1;
  resultsState = DISP_INIT;
  state = STATE_GET_READY;
}


/**
 * getRead handles STATE_GET_READY. Show the user a countdown to game start.
 * 
 * TODO: This could stand to be cleaned up: Repeated code put in a new function; magic 
 * numbers at least replaced with #define, etc.
 */
void getReady() {
  switch (readyState) {
    case RDY_LT_1:
      trellis.setLED(12);
      trellis.setLED(13);
      trellis.setLED(14);
      trellis.setLED(15);
      trellis.writeDisplay();
      sparkle.turnOnAllColor(RED);
      low.FromTheTop();
      low.Play();
      readyState = RDY_1;
      break;
    case RDY_1:
      low.Update();
      if (!low.IsPlaying()) {
        sparkle.allOff();
        readyState = RDY_LT_2;
      }
      break;
    case RDY_LT_2:
      trellis.setLED(8);
      trellis.setLED(9);
      trellis.setLED(10);
      trellis.setLED(11);
      trellis.writeDisplay();
      sparkle.turnOnAllColor(YELLOW);
      low.FromTheTop();
      low.Play();
      readyState = RDY_2;
      break;
    case RDY_2:
      low.Update();
      if (!low.IsPlaying()) {
        sparkle.allOff();
        readyState = RDY_LT_3;
      }
      break;
    case RDY_LT_3:
      trellis.setLED(4);
      trellis.setLED(5);
      trellis.setLED(6);
      trellis.setLED(7);
      trellis.writeDisplay();
      sparkle.turnOnAllColor(BLUE);
      low.FromTheTop();
      low.Play();
      readyState = RDY_3;
      break;
    case RDY_3:
      low.Update();
      if (!low.IsPlaying()) {
        sparkle.allOff();
        readyState = RDY_LT_4;
      }
      break;
    case RDY_LT_4:
      trellis.setLED(0);
      trellis.setLED(1);
      trellis.setLED(2);
      trellis.setLED(3);
      trellis.writeDisplay();
      sparkle.turnOnAllColor(GREEN);
      high.FromTheTop();
      high.Play();
      readyState = RDY_4;
      break;
    case RDY_4:
      high.Update();
      if (!high.IsPlaying()) {
        sparkle.allOff();
        clearAll();
        readyState = RDY_LT_1;
        state = STATE_PLAY;
      }
      break;
  }
}


/**
 * playGame handles STATE_PLAY. Handle game play events.
 * 
 * TODO: Break this into functions. Time limit the game. Make there be a hard game 
 * that is sneakier.
 */
void playGame() {
  delay(20);

  if (chooseNext) {
    // Time is up for the previous lit-up LED. Choose a new one.
    short newId = NOT_SET;

    // Assign an LED to illuminate. Ensure that it's not the same as the last one.
    do {
      // Purposefully do more than there are buttons. Want there to be a decent chance
      // that no LED is turned on, to make it a little less predictable. Numbers higher
      // than BUTTON_COUNT won't be reported as button pushes, so if no LED is illuminated
      // and a button is pushed, it will be correctly scored as a miss. Integer arithmetic
      // with the 1.5 factor is "close enough".
      newId = random(BUTTON_COUNT * 1.5);
    } while (newId == id);
    id = newId;
    chooseNext = false;
    // If the next one is a button, illuminate it.
    if (id < BUTTON_COUNT) {
      trellis.setLED(id);
      trellis.writeDisplay();
    }
    startTime = millis();
  } else {
    // Still on the currently lit-up LED. Is its time up?
    if (millis() >= (startTime + duration)) {
      // Yes. Next loop-around, choose a new LED to light up.
      trellis.clrLED(id);
      trellis.writeDisplay();
      chooseNext = true;
    } else {
      // The currently lit-up LED's time is not up. See if a button was pushed.
      // readSwitches returns true when a button is pushed or when a button is released.
      if (trellis.readSwitches()) {
        // A button was pressed. Which one?
        if (trellis.justPressed(id)) {
          // The correct one. Point awarded:
          Serial.print("+");
          success.Play();
          pointsFor++;
        } else {
          // Possibly a "just released" event, which we must ignore. So check all other
          // buttons to see if they were pressed. If so, deduct a point. Otherwise, ignore
          // this event.
          for (int i=0; i<BUTTON_COUNT; i++) {
            if (trellis.justPressed(i))
              {
                // Bad button pushed. Point deducted:
                Serial.print("-");
                miss.Play();
                pointsAgainst++;
              }
          }
        }
      }
    }
  }

  // Get out of the game by having enough points for or against.
  if ((pointsFor > 10) || (pointsAgainst > 10)) {
    Serial.print("points for: ");
    Serial.print(pointsFor);
    Serial.print(" points against: ");
    Serial.println(pointsAgainst);
    state = STATE_RESULTS;
  }
}


/**
 * showResults handles STATE_RESULTS. Display the score when the game is over.
 * 
 * TODO: make it display the score and wait for a button push.
 */
void showResults() {
  switch (resultsState) {
  case DISP_INIT:
    clearAll();
    if (pointsFor > pointsAgainst) {
      youwin.Play();
      sparkle.turnOnAllColor(GREEN);
    } else {
      youlose.Play();
      sparkle.turnOnAllColor(RED);
    }
    resultsState = DISP_SHOW;
    break;

  case DISP_SHOW:
    if (pointsFor > pointsAgainst) {
      youwin.Update();
      if (!youwin.IsPlaying()) {
        resultsState = DISP_WAIT;
      }
    } else {
      youlose.Update();
      if (!youlose.IsPlaying()) {
        resultsState = DISP_WAIT;
      }
    }
    break;

  case DISP_WAIT:
    // Wait for a button push to move on to the next game.
//    if (trellis.readSwitches()) {
//      for (int i=0; i<BUTTON_COUNT; i++) {
//        if (trellis.justPressed(i))
//          {
            sparkle.allOff();
            state = STATE_INIT;
//          }
//        }
//      trellis.readSwitches();
//    }
    break;
  }
}


/**
 * setup.
 * Initialize the random seed; the Trellis board; the melody player; the State.
 */
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // easy/hard selector
  pinMode(DIFFICULTY_SELECT_PIN, INPUT_PULLUP);

  // i2c address for Trellis
  trellis.begin(0x70);

  sparkle.initPins();

  // Get ready to play a game
  state = STATE_INIT;
}


/**
 * Loop through the game's states.
 */
void loop() {
  switch (state) {
    case STATE_INIT:
      initNextGame();
      break;

    case STATE_GET_READY:
      getReady();
      break;

    case STATE_PLAY:
      playGame();
      break;

    case STATE_RESULTS:
      showResults();
      break;
  }
  success.Update();
  miss.Update();
}

