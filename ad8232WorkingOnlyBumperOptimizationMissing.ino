//Arduino i2c libary
#include <Wire.h>
//Screen libaries
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --- Screen Settings and Analog Pin denifation ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 //no reset pin
#define EKG_PIN PA0      //A0 Pin as input

// Lead-Off pins defination
const int LO_PLUS_PIN = D8;    //LO+ pin
const int LO_MINUS_PIN = D9;   //LO- pin

// --- BPM Calculating Limits Settings ---
int beatThreshold = 800;                     //Limit point to be taken as a Pulse
unsigned long minTimeBetweenBeats = 400;    // Debounce delay for beat detection to avoid false triggers.

// --- Screen defination ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// 15 second counting veriables
const long bpmCalcInterval = 15000; // 15 Second in ms
unsigned long lastBpmCalcTime = 0;  // Last bpm calculation time
int beatCounter = 0;                // bpm counter in 15 sec
int displayBpm = 0;                 // Bpm record for secreen dispilaying

bool isInWarningState = false;    //Lead-off connection bool
unsigned long lastBeatTime = 0;   //Protection for debounce calculation
int lastPlotX = 0;                //X cordination for plotting
int lastPlotY = 0;                //Y cordination for plotting
int plotX = 0;                    //time interval for plotting (screen flowing)

void setup() {
  Serial.begin(115200); 
  pinMode(EKG_PIN, INPUT);        //Analog pin as input
  pinMode(LO_PLUS_PIN, INPUT);    //LO+ Digital input pin
  pinMode(LO_MINUS_PIN, INPUT);   //LO- Digital input pin
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { //System protection for screen working. If the screen adress wrong infinite loop
    while(true);
  }
  
  display.clearDisplay();
  lastPlotY = 30;               //Almost middle point to start screening
  display.display();            //Update the screen
  lastBpmCalcTime = millis();   //reset bpm time to now
}

void loop() {
  bool leadsAreOff = (digitalRead(LO_PLUS_PIN) == HIGH || digitalRead(LO_MINUS_PIN) == HIGH); //bool of the elektrods are connected or not

  if (leadsAreOff) {                //Electrod connection correction
    if (!isInWarningState) {        //Check warning state
      isInWarningState = true;      //Yes we are in warning state
      display.clearDisplay();
      display.setTextSize(1);
      display.setTextColor(SSD1306_WHITE);
      display.setCursor(15, 20);
      display.println("ELEKTROTLARI");
      display.setCursor(15, 35);
      display.println(" KONTROL EDIN");
      display.display();          //Setting screen warning state wrate text, cordinates and update screen
      
      displayBpm = 0;
      beatCounter = 0;
    }
  } 
  else { // Electrods are connected
    if (isInWarningState) {         //If we were previously in the warning state, this block resets everything for normal operation.
      isInWarningState = false;     // We are no longer in a warning state, so set the flag to false.
      display.clearDisplay();       //Clear screen
      lastBpmCalcTime = millis();   // Reset the 15-second BPM calculation timer.
      plotX = 0;                    //Reset plot positioning left
      lastPlotX = 0;
      beatCounter = 0;              // Reset the beat counter and the displayed BPM value to zero.
      displayBpm = 0;
    }

    // --- EKG Part ---
    unsigned long currentTime = millis();       // Get the current timestamp for this loop cycle.
    int sensorValue = analogRead(EKG_PIN);      // Read the raw analog value from the EKG sensor pin.
    Serial.println(sensorValue);                // Send the raw sensor value to the Serial Plotter for debugging.
    
    //  Detect peek point and increase bpm
    if (sensorValue > beatThreshold && currentTime - lastBeatTime > minTimeBetweenBeats) {
      beatCounter++;               // If the signal is high enough to be a pulse and bumper time passed then increment our count of bpm
      lastBeatTime = currentTime; // Record the new bpm time
    }

    // Check 15 second is passed
    if (currentTime - lastBpmCalcTime > bpmCalcInterval) { //If passed
      displayBpm = beatCounter * 4;     // Calculate new bpm for sceen(1 minute)
      beatCounter = 0;                  // Reset counter
      lastBpmCalcTime = currentTime;    // Reset time
    }
    
    // --- Plotting ---
    if (plotX >= SCREEN_WIDTH) {        // If the drawing position has reached the right edge of the screen
      plotX = 0;
      lastPlotX = 0;
      display.clearDisplay();           // reset the horizontal position to the left and clear the entire screen
    }
    //4085 - 2047 -1023
    int plotY = map(sensorValue, 0, 2048, 45, 5);                           // Map the raw sensor value (0-2048) to a vertical screen coordinate (45-5)
    display.drawLine(lastPlotX, lastPlotY, plotX, plotY, SSD1306_WHITE);    // Draw a line from the previous point to the new current point

    lastPlotX = plotX;        // Remember the current point's coordinates for the next line segment
    lastPlotY = plotY;

    // --- Calculate 15 second interval ---
    int elapsedSeconds = (currentTime - lastBpmCalcTime) / 1000;  // Calculate how many seconds have passed since the last BPM update

    display.fillRect(108, 0, 20, 8, SSD1306_BLACK);    // Clear the small area in the top-right corner where the countdown is displayed
    display.setTextSize(1);           // Print the new countdown value in the top-right corner
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(110, 0);
    display.print(elapsedSeconds);

    
    display.fillRect(0, 52, SCREEN_WIDTH, 12, SSD1306_BLACK);     // Clear the bottom area of the screen where the BPM value is shown
    display.setTextSize(1);                         // Print the current BPM value
    display.setCursor(35, 54);
    display.setTextColor(SSD1306_WHITE);
    display.print(displayBpm);
    display.print(" BPM");

    display.display();        //Update screen

    plotX++;                  // Move the horizontal drawing position one pixel
  }
  
  delay(5);                   // Wait for 5 milliseconds to stabilize the loop and analog readings
}