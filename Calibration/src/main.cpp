// ******************************************************************************************************
// * This code is distributed under the GNU Public License v3                                           *
// * (see LICENSE file at root of repository for details)                                               *
// * This code is used to calibrate Open-Smart 3.2inch TFT LCD Shield                                   *
// * (probably can be used to calibrate other displays as well)                                         *
// ******************************************************************************************************
// * This code is modified from bytesnbits to work with the Open-Smart 3.2inch TFT                      *
// * Link: https://bytesnbits.co.uk/arduino-touchscreen-calibration-coding/#1606910525307-c15b5275-547b *
// ******************************************************************************************************
// * This code is written for the Arduino Uno and Open-Smart 3.2inch TFT LCD Shield                     *
// ******************************************************************************************************
// * Default Arduino libraries used:                                                                    *
// *  - Arduino.h                                                                                       *
// *  - math.h                                                                                          *
// * Libraries included:                                                                                *
// *  - MCUFRIEND_kbv (this library is probably modified for this specific display)                     *
// * Libraries needed to be installed:                                                                  *
// *  - Adafruit GFX (graphics)                                                                         *
// *  - Adafruit touchscreen (touchscreen driver)                                                       *
// ******************************************************************************************************
// * No aditional setup needed (appart from installing the shield obviously)                            *
// ******************************************************************************************************



#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <math.h>

// Colors
#define BLACK 0x0000
#define NAVY 0x000Fa
#define DARKGREEN 0x03E0
#define DARKCYAN 0x03EF
#define MAROON 0x7800
#define PURPLE 0x780F
#define OLIVE 0x7BE0
#define LIGHTGREY 0xC618
#define DARKGREY 0x7BEF
#define BLUE 0x001F
#define GREEN 0x07E0
#define CYAN 0x07FF
#define RED 0xF800
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define ORANGE 0xFD20
#define GREENYELLOW 0xAFE5
#define PINK 0xF81F

MCUFRIEND_kbv tft;

// Touch screen pins for ILI9320:
// ILI9320: YP=A2, XM=A3, YM=8, XP=9

uint8_t YP = A1; // must be an analog pin, use "An" notation!
uint8_t XM = A2; // must be an analog pin, use "An" notation!
uint8_t YM = 7;  // can be a digital pin
uint8_t XP = 6;  // can be a digital pin
uint8_t SwapXY = 0;

// calibration values
float xCalM = 0.0, yCalM = 0.0; // gradients
float xCalC = 0.0, yCalC = 0.0; // y axis crossing points

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);
TSPoint tp;

class TouchPoint
{
public:
  int16_t x;
  int16_t y;

  // default constructor
  TouchPoint()
  {
  }

  TouchPoint(int16_t xIn, int16_t yIn)
  {
    x = xIn;
    y = yIn;
  }
};

#define MINPRESSURE 100
#define MAXPRESSURE 1000

TouchPoint touchPoint;
int last_pressure = 0;
int total_pressure_samples_for_release = 0;
bool is_touching = false;

void detect_touch()
{
  tp = ts.getPoint();

  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  pinMode(XP, OUTPUT);
  pinMode(YM, OUTPUT);

  if (tp.z > MINPRESSURE && tp.z < MAXPRESSURE)
  {
    touchPoint.x = tp.x;
    touchPoint.y = tp.y;
    total_pressure_samples_for_release = 0;

    is_touching = true;
  }
  else
  {
    if (is_touching == true)
    {
      // sample last pressure, check if last pressure was 0.  We need to see 5 consecutive zeros to
      // confirm a release.
      total_pressure_samples_for_release++;

      if (tp.z == 0 && last_pressure == 0 && total_pressure_samples_for_release == 5)
      {
        // release!
        last_pressure = 0;
        total_pressure_samples_for_release = 0;
        is_touching = false;
      }
    }
  }
}

void calibrateTouchScreen()
{
  // Initialize variable for the touch point
  TSPoint p;
  // Initialize variables for the points coordinates
  int16_t x1, y1, x2, y2;

  // Make the screen black
  tft.fillScreen(BLACK);
  // Wait until user is not touching the screen
  detect_touch();
  while (is_touching)
  {
    detect_touch();
  }

  // Draw the first point  
  tft.drawFastHLine(10, 20, 20, RED);
  tft.drawFastVLine(20, 10, 20, RED);
  detect_touch();
  while (!is_touching)
  {
    detect_touch();
  }
  // This delay is so the stylus is well pressed (so that we get better reading)
  delay(50);
  // Get the first point' raw coordinates
  p = ts.getPoint();
  x1 = p.x;
  y1 = p.y;
  // Overwrite the first point with a black point
  tft.drawFastHLine(10, 20, 20, BLACK);
  tft.drawFastVLine(20, 10, 20, BLACK);
  delay(500);
  // Wait until user is not touching the screen
  detect_touch();
  while (is_touching)
  {
    detect_touch();
  }

  // Draw the second point
  tft.drawFastHLine(tft.width() - 30, tft.height() - 20, 20, RED);
  tft.drawFastVLine(tft.width() - 20, tft.height() - 30, 20, RED);
  // Wait until user is touching the screen
  detect_touch();
  while (!is_touching)
  {
    detect_touch();
  }
  // This delay is so the stylus is well pressed (so that we get better reading)
  delay(50);
  p = ts.getPoint();
  x2 = p.x;
  y2 = p.y;
  // Overwrite the second point with a black point
  tft.drawFastHLine(tft.width() - 30, tft.height() - 20, 20, BLACK);
  tft.drawFastVLine(tft.width() - 20, tft.height() - 30, 20, BLACK);

  // Calculate the distance between the two points (pythagoran theorem)
  int16_t xDist = tft.width() - 40;
  int16_t yDist = tft.height() - 40;

  // Calculate the gradients of the line
  // More about how this calculation works: https://bytesnbits.co.uk/arduino-touchscreen-calibration-coding/#1606910525307-c15b5275-547b (if it's not available, the year for internet archive is 2022)
  // translate in form pos = m x val + c
  // x
  // m = (x2 - x1) / xDist
  xCalM = (float)xDist / (float)(x2 - x1);
  xCalC = 20.0 - ((float)x1 * xCalM);

  // y
  // m = (y2 - y1) / yDist
  yCalM = (float)yDist / (float)(y2 - y1);
  yCalC = 20.0 - ((float)y1 * yCalM);

  Serial.print("x1 = ");
  Serial.print(x1);
  Serial.print(", y1 = ");
  Serial.print(y1);
  Serial.print(", x2 = ");
  Serial.print(x2);
  Serial.print(", y2 = ");
  Serial.println(y2);
  Serial.print("xCalM = ");
  Serial.print(xCalM);
  Serial.print(", xCalC = ");
  Serial.print(xCalC);
  Serial.print(", yCalM = ");
  Serial.print(yCalM);
  Serial.print(", yCalC = ");
  Serial.println(yCalC);
}

void setup()
{
  // Reset the display
  tft.reset();
  // Begin serial communication
  Serial.begin(9600);
  // Initialize the display
  // Get the ID of the display
  uint16_t ID = tft.readID();
  // Start the display with the ID
  tft.begin(ID);
  // Print the display's width and height (with default rotation)
  Serial.println("Display width: " + String(tft.width()));
  Serial.println("Display height: " + String(tft.height()));
  // Call the calibration function
  calibrateTouchScreen();
}


void loop()
{
}