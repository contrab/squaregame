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
#define EASY_DURATION 400
#define MEDIUM_DURATION 300
#define HARD_DURATION 150

// State Machine:
enum programState {
  STATE_INIT = 0,
  STATE_PLAY,
  STATE_RESULTS
};
enum programState state;

// Globals:

Adafruit_Trellis matrix = Adafruit_Trellis();
Adafruit_TrellisSet trellis =  Adafruit_TrellisSet(&matrix);

short id = NOT_SET;
int startTime = 0;
int duration = EASY_DURATION;
short pointsFor = 0;
short pointsAgainst = 0;

// Sound Effects:
onenote success_notes[] = {{NOTE_C5, 6}, {NOTE_E5, 12}};
MelodyPlayer success(SPEAKER_PIN, success_notes);
onenote miss_notes[] = {{NOTE_A2, 8}};
MelodyPlayer miss(SPEAKER_PIN, miss_notes);

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

  // No LED is set.
  id = NOT_SET;

  // TODO get input from user how hard/easy to make the game.
  duration = EASY_DURATION;

  // Reset the score.
  pointsFor = 0;
  pointsAgainst = 0;

  // Now play the game.
  state = STATE_PLAY;
}


/**
 * playGame handles STATE_PLAY. Handle game play events.
 */
void playGame() {
  if (NOT_SET == id) {
    // Assign an LED to illuminate
    id = random(BUTTON_COUNT);
    trellis.setLED(id);
    trellis.writeDisplay();
    startTime = millis();
  } else {
    if (millis() >= (startTime + duration)) {
      trellis.clrLED(id);
      trellis.writeDisplay();
      id = NOT_SET;
    }
  }

  // Get out of the game by having enough points for or against.
  if ((pointsFor > 10) || (pointsAgainst > 10)) {
    state = STATE_RESULTS;
  }
}


/**
 * showResults handles STATE_RESULTS. Display the score when the game is over.
 */
void showResults() {
  state = STATE_INIT;
}


/**
 * setup.
 * Initialize the random seed; the Trellis board; the melody player; the State.
 */
void setup() {
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
      initNextGame();
      break;

    case STATE_PLAY:
      playGame();
      break;

    case STATE_RESULTS:
      showResults();
      break;
  }
}

