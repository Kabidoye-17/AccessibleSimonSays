#include "pitches.h"
#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "";      // Replace with your WiFi name
const char* password = "";  // Replace with your WiFi password
const char* serverURL = "http://<IPV4>/game_result"; // Replace with computer IPV4 address


// set up LED pins
int redLEDPin = 25;
int yellowLEDPin = 12;
int greenLEDPin = 13;
int LEDPins[3] = {redLEDPin, yellowLEDPin, greenLEDPin};

// Segment pin definitions
const int segmentA = 17;
const int segmentB = 16;
const int segmentC = 26;
const int segmentD = 22;
const int segmentE = 4;
const int segmentF = 23;
const int segmentG = 18;

// set up buzzer pins
int buzzerPin = 14;

// set up buttons
int redButtonPin = 34;
int yellowButtonPin = 19;
int greenButtonPin = 35;
int onOffButtonPin = 21; // Off button pin

// set up button states
int redButtonState = 0;
int yellowButtonState = 0;
int greenButtonState = 0;
int onOffButtonState = 0;  // State for the off button

// Segment array and digit mappings
int segment[] = {segmentA, segmentB, segmentC, segmentD, segmentE, segmentF, segmentG};
int digit[10][7] = {
  {0, 0, 0, 0, 0, 0, 1}, // 0
  {1, 0, 0, 1, 1, 1, 1}, // 1
  {0, 0, 1, 0, 0, 1, 0}, // 2
  {0, 0, 0, 0, 1, 1, 0}, // 3
  {1, 0, 0, 1, 1, 0, 0}, // 4
  {0, 1, 0, 0, 1, 0, 0}, // 5
  {0, 1, 0, 0, 0, 0, 0}, // 6
  {0, 0, 0, 1, 1, 1, 1}, // 7
  {0, 0, 0, 0, 0, 0, 0}, // 8
  {0, 0, 0, 1, 1, 0, 0}  // 9
};

int pinOrder[9];
int userOrder[9];

// define game
int gameRound = 1;
int userInputCounter = 0;
int userIsCorrect = 1;
int gameState = 0;


int melody[] = {
  NOTE_E5, NOTE_G5, NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G6,
  NOTE_E6, NOTE_C6, NOTE_D6, NOTE_G5
};

int noteDurations[] = {
  8, 8, 8, 8, 8, 4, 
  8, 8, 8, 4
};

void victoryBuzz(){
   for (int thisNote = 0; thisNote < 10; thisNote++) {
    int noteDuration = 1000 / noteDurations[thisNote];
    tone(buzzerPin, melody[thisNote], noteDuration);
    delay(noteDuration * 1.3);
    noTone(buzzerPin);
  }
}

void displayDigit(int num) {
  Serial.println(num);
  for (int i = 0; i < 7; i++) {
    digitalWrite(segment[i], digit[num][i]);
  }
}

void setup() {

  pinMode(redLEDPin, OUTPUT);
  pinMode(yellowLEDPin, OUTPUT);
  pinMode(greenLEDPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(redButtonPin, INPUT);
  pinMode(redButtonPin, INPUT_PULLDOWN);
  pinMode(yellowButtonPin, INPUT);
  pinMode(yellowButtonPin, INPUT_PULLDOWN);
  pinMode(greenButtonPin, INPUT);
  pinMode(greenButtonPin, INPUT_PULLDOWN);
  pinMode(onOffButtonPin, INPUT);

  pinMode(segmentA, OUTPUT);
  pinMode(segmentB, OUTPUT);
  pinMode(segmentC, OUTPUT);
  pinMode(segmentD, OUTPUT);
  pinMode(segmentE, OUTPUT);
  pinMode(segmentF, OUTPUT);
  pinMode(segmentG, OUTPUT);

  displayDigit(0);

  Serial.begin(115200);

  WiFi.begin(ssid, password);

    Serial.print("Connecting to WiFi...");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nConnected to WiFi!");
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

  Serial.println("setup");
}
// define buzz methods 

void longBuzz(int ledPin) {
  if (ledPin == redLEDPin) {
    tone(buzzerPin, 300, 300);
    delay(600);
  } else if (ledPin == yellowLEDPin) {
    tone(buzzerPin, 450, 300);
    delay(600);
    noTone(buzzerPin);
  } else if (ledPin == greenLEDPin) {
    tone(buzzerPin, 600, 300);
    delay(600);
    noTone(buzzerPin);
  }
}

void shortBuzz(int ledPin) {
  if (ledPin == redLEDPin) {
    tone(buzzerPin, 300, 300);
    delay(400);
  } else if (ledPin == yellowLEDPin) {
    tone(buzzerPin, 450, 300);
    delay(400);
  } else if (ledPin == greenLEDPin) {
    tone(buzzerPin, 600, 300);
    delay(400);
  }
}

void correctBuzz() {
  tone(buzzerPin, 300, 300); 
  delay(300);                
  noTone(buzzerPin);    
  delay(10);                 
  tone(buzzerPin, 500, 300); 
  delay(300);  
  noTone(buzzerPin);
}


void incorrectBuzz() {
  tone(buzzerPin, 500, 300); 
  delay(100);                
  tone(buzzerPin, 300, 300); 
  delay(600);  
}

void startupBuzz() {
  tone(buzzerPin, 300, 200);  // Low note
  delay(250);
  tone(buzzerPin, 450, 200);  // Middle note
  delay(250);
  tone(buzzerPin, 600, 300);  // High note
  delay(500);
}

void shutdownBuzz() {
  tone(buzzerPin, 600, 200);  // High note
  delay(250);
  tone(buzzerPin, 450, 200);  // Middle note
  delay(250);
  tone(buzzerPin, 300, 300);  // Low note
  delay(500);
  noTone(buzzerPin);
}

void sendGameResult(int score) {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(serverURL);
        http.addHeader("Content-Type", "application/json");

        String jsonPayload = "{ \"score\":" + String(score) + "}";

        int httpResponseCode = http.POST(jsonPayload);
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);

        http.end();
    } else {
        Serial.println("WiFi not connected!");
    }
}

// define how the game is to be ran
void runGame() {
  while (gameRound < 9) {
    // Display pattern
    int pinIndex = rand() % 3;
    pinOrder[gameRound] = LEDPins[pinIndex];
    
    for (int i = 0; i <= gameRound; i++) {
      int ledPin = pinOrder[i];
      digitalWrite(ledPin, HIGH);
      longBuzz(ledPin);
      digitalWrite(ledPin, LOW);
      delay(800);
    }

    // Get User Input
    userInputCounter = 0;

    while (userInputCounter <= gameRound) {
      redButtonState = digitalRead(redButtonPin);
      yellowButtonState = digitalRead(yellowButtonPin);
      greenButtonState = digitalRead(greenButtonPin);
      onOffButtonState = digitalRead(onOffButtonPin);

      if (onOffButtonState == HIGH) {
        Serial.println("Game stopped by user.");
        shutdownBuzz();
        sendGameResult(gameRound);
        displayDigit(0);
        userIsCorrect = 0;
        gameRound = 10;
        gameState = 0;
        break;
      }

      // RED BUTTON
      if (redButtonState == HIGH) {
        digitalWrite(redLEDPin, HIGH);
        userOrder[userInputCounter] = redLEDPin;
        shortBuzz(redLEDPin);
        delay(200);
        digitalWrite(redLEDPin, LOW);
        delay(50);
        if (userOrder[userInputCounter] != pinOrder[userInputCounter]) {
          Serial.println("WRONG!");
          incorrectBuzz();
          shutdownBuzz();
          sendGameResult(gameRound);
          displayDigit(0);
          userIsCorrect = 0;
          gameRound = 10;
          gameState = 0;
          break;
        }
        userInputCounter++;
      }

      // YELLOW BUTTON
      if (yellowButtonState == HIGH) {
        digitalWrite(yellowLEDPin, HIGH);
        userOrder[userInputCounter] = yellowLEDPin;
        shortBuzz(yellowLEDPin);
        delay(200);
        digitalWrite(yellowLEDPin, LOW);
        delay(50);
        if (userOrder[userInputCounter] != pinOrder[userInputCounter]) {
          Serial.println("WRONG!");
          incorrectBuzz();
          shutdownBuzz();
          sendGameResult(gameRound);
          displayDigit(0);

          userIsCorrect = 0;
          gameRound = 10;
          gameState = 0;
          break;
        }
        userInputCounter++;
      }

      // GREEN BUTTON
      if (greenButtonState == HIGH) {
        digitalWrite(greenLEDPin, HIGH);
        userOrder[userInputCounter] = greenLEDPin;
        shortBuzz(greenLEDPin);
        delay(200);
        digitalWrite(greenLEDPin, LOW);
        delay(50);
        if (userOrder[userInputCounter] != pinOrder[userInputCounter]) {
          Serial.println("WRONG!");
          incorrectBuzz();
          shutdownBuzz();
          sendGameResult(gameRound);
          displayDigit(0);
          userIsCorrect = 0;
          gameRound = 10;
          gameState = 0;
          break;
        }
        userInputCounter++;
      }
    }

    // Correct guess feedback
    if (userIsCorrect) {
      gameRound++;
      correctBuzz();
      displayDigit(gameRound);
      delay(2000);
    }
  }

  if (gameRound == 9 && userIsCorrect) {
    Serial.println("YOU WIN! 🎉");
    victoryBuzz();
    sendGameResult(gameRound);
    gameState = 0;
    delay(1000);
    displayDigit(0);

  }
}


void loop() {
  
  if (digitalRead(onOffButtonPin) == HIGH) {
    
    if (gameState == 0) { 
      gameRound = 0;
      gameState= 1;
      userIsCorrect = 1;
      userInputCounter = 0;
      memset(pinOrder, 0, sizeof(pinOrder));
      memset(userOrder, 0, sizeof(userOrder));
      Serial.println("Game started!");
      startupBuzz();
      delay(1000);
      runGame(); // Start the game
    } 
  }

}


