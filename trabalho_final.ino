#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define INCREASE_BUTTON 38

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Magic constants for positions and size
const int FACE_X = 96;
const int FACE_Y = 40;
const int FACE_R = 20;
const int EYE_OFFSET_X = 7;
const int EYE_OFFSET_Y = 7;
const int EYE_R = 2;
const int MOUTH_R = 10;
int people_count = 0;
int onMenu = false;

// Helper function to draw arcs
void drawArc(int x, int y, int radius, int startAngle, int endAngle) {
  for (int angle = startAngle; angle <= endAngle; angle++) {
    float rad = angle * 3.14159 / 180.0;
    int px = x + radius * cos(rad);
    int py = y + radius * sin(rad);
    display.drawPixel(px, py, SSD1306_WHITE);
  }
}

void drawFace(int type=0) {
  display.drawCircle(FACE_X, FACE_Y, FACE_R, SSD1306_WHITE);
  display.fillCircle(FACE_X - EYE_OFFSET_X, FACE_Y - EYE_OFFSET_Y, EYE_R, SSD1306_WHITE);
  display.fillCircle(FACE_X + EYE_OFFSET_X, FACE_Y - EYE_OFFSET_Y, EYE_R, SSD1306_WHITE);

  if (type == 0) {
    display.drawLine(FACE_X - MOUTH_R, FACE_Y + 5, FACE_X + MOUTH_R, FACE_Y + 5, SSD1306_WHITE); // Straight mouth
  } else if (type == -1) {
    drawArc(FACE_X, FACE_Y + 10, MOUTH_R, 180, 360); // sad
  } else {
    drawArc(FACE_X, FACE_Y, MOUTH_R, 0, 180); // smile 
  }
}

void drawEnterScreen() {
  drawFace(1);
  display.setCursor(10, 5);
  display.print("Uma pessoa entrou!");
  display.setCursor(10, 20);
  display.print("Total: ");
  display.print(String(people_count));
  display.display();
}

void drawInfoScreen() {

}

void drawMaxCapacityScreen() {
  delay(50);
}

void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.clearDisplay();
  pinMode(INCREASE_BUTTON, INPUT_PULLUP);
}

void loop() {
  display.clearDisplay();
  display.display();

  if (digitalRead(INCREASE_BUTTON) == LOW) {
    people_count++;
    drawEnterScreen();
    delay(1000);
  } else if ((digitalRead(MENU_BUTTON) == LOW && !onMenu) || onMenu) {
    onMenu == true;

  } else {
    drawInfoScreen();
    delay(100);
  }

  delay(100);
}
