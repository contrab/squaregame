/**
 * Copyright 2017 Thaddeus Jaszek.
 */

#include <Wire.h>
#include <Adafruit_Trellis.h>
#include <MelodyPlayer.h>
#include "pitches.h"

// Pin Assignments:
#define SPEAKER_PIN 8
#define TREL_INT_PIN 2

// Parameters etc.
#define BUTTON_COUNT 16
#define NOT_SET -1
#define EASY_DURATION 650
#define MEDIUM_DURATION 550
#define HARD_DURATION 450

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


// Globals:

Adafruit_Trellis matrix = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix);

short id = NOT_SET;
bool chooseNext = true;
unsigned long startTime = 0;
unsigned long duration = EASY_DURATION;
short pointsFor = 0;
short pointsAgainst = 0;
unsigned long throttle = 0;

// Sound Effects:
onenote success_notes[] = {{NOTE_C5, 6}, {NOTE_E5, 12}};
MelodyPlayer success(SPEAKER_PIN, success_notes);
onenote miss_notes[] = {{NOTE_A2, 8}};
MelodyPlayer miss(SPEAKER_PIN, miss_notes);
onenote low_notes[] = {{NOTE_C4, 3}, {0, 8}};
MelodyPlayer low(SPEAKER_PIN, low_notes);
onenote high_notes[] = {{NOTE_C6, 2}, {0, 8}};
MelodyPlayer high(SPEAKER_PIN, high_notes);

// Functions:

/**
 * initNextGame handles STATE_INIT. Initialize for the next game.
 * 
 * TODO: Get input from user on difficulty level. For now, just set to EASY.
 */
void initNextGame() {
  // Ensure all the LEDs are out
  for (int i=0; i<BUTTON_COUNT; i++) {
    trellis.clrLED(i);
  }
  trellis.writeDisplay();

  // Reset sounds
  success.Pause();
  success.FromTheTop();
  miss.Pause();
  miss.FromTheTop();

  // No LED is set.
  id = NOT_SET;
  chooseNext = true;

  // TODO get input from user how hard/easy to make the game.
  duration = EASY_DURATION;

  // Reset the score.
  pointsFor = 0;
  pointsAgainst = 0;

  // Tell the game player to get ready.
  readyState = RDY_LT_1;
  state = STATE_GET_READY;
}


/**
 * getRead handles STATE_GET_READY. Show the user a countdown to game start.
 */
void getReady() {
  switch (readyState) {
    case RDY_LT_1:
      low.FromTheTop();
      low.Play();
      readyState = RDY_1;
      break;
    case RDY_1:
      low.Update();
      if (!low.IsPlaying()) {
        readyState = RDY_LT_2;
      }
      break;
    case RDY_LT_2:
      low.FromTheTop();
      low.Play();
      readyState = RDY_2;
      break;
    case RDY_2:
      low.Update();
      if (!low.IsPlaying()) {
        readyState = RDY_LT_3;
      }
      break;
    case RDY_LT_3:
      low.FromTheTop();
      low.Play();
      readyState = RDY_3;
      break;
    case RDY_3:
      low.Update();
      if (!low.IsPlaying()) {
        readyState = RDY_LT_4;
      }
      break;
    case RDY_LT_4:
      high.FromTheTop();
      high.Play();
      readyState = RDY_4;
      break;
    case RDY_4:
      high.Update();
      if (!high.IsPlaying()) {
        readyState = RDY_LT_1;
        state = STATE_PLAY;
      }
      break;
  }
}

/**
 * playGame handles STATE_PLAY. Handle game play events.
 */
void playGame() {
  delay(20);

  if (chooseNext) {
    short newId = NOT_SET;
    // Assign an LED to illuminate. Ensure that it's not the same as the last one.
    do {
      // Purposefully do more than there are buttons. Want there to be a decent chance
      // that no LED is turned on, to make it a little less predictable.
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
    if (millis() >= (startTime + duration)) {
      trellis.clrLED(id);
      trellis.writeDisplay();
      chooseNext = true;
    } else {
      // readSwitches returns true when a button is pushed or when a button is released.
      if (trellis.readSwitches()) {
        // A button was pressed. Which one?
        if (trellis.justPressed(id)) {
          // The correct one. Point awarded:
          Serial.print("+");
          success.Play();
          pointsFor++;
        } else {
          // Possibly a "just released" event, which we must ignore. So check all buttons
          // to see if they were pressed. If so, deduct a point. Otherwise, ignore this
          // event.
          for (int i=0; i<BUTTON_COUNT; i++) {
            if (trellis.justPressed(i))
              {
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
 */
void showResults() {
  // Wait for a button push to move on to the next game.
  if (trellis.readSwitches()) {
    delay(200);
    trellis.readSwitches();
    state = STATE_INIT;
  }
}


/**
 * setup.
 * Initialize the random seed; the Trellis board; the melody player; the State.
 */
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // INT pin requires a pullup
  pinMode(TREL_INT_PIN, INPUT);
  digitalWrite(TREL_INT_PIN, HIGH);

  // i2c address for Trellis
  trellis.begin(0x70);

  // Get ready to play a game
  state = STATE_INIT;
}


/**
 * Loop through the game's states.
 */
void loop() {
  switch (state) {
    case STATE_INIT:
      Serial.println("call initNextGame()");
      initNextGame();
      break;

    case STATE_GET_READY:
      getReady();
      break;

    case STATE_PLAY:
      playGame();
      break;

    case STATE_RESULTS:
      Serial.println("call showResults()");
      showResults();
      break;
  }
  success.Update();
  miss.Update();
}

