#include <Wire.h> // Necessary for I2C communication
#include <LiquidCrystal.h> //LCD library
LiquidCrystal lcd(7,6,5,4,3,2); // initialize the LCD object

int accel = 0x53; // I2C address for this sensor (from data sheet)
float x, y, z, acc;

// declare flag for data storage
bool flag = false;

// define thresholds
int upper_threshold = 250;
int lower_threshold = 220;

int steps;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize serial communications
  Wire.beginTransmission(accel); // Start communicating with the device
  Wire.write(0x2D); // Enable measurement
  Wire.write(8); // Get sample measurement
  Wire.endTransmission();
  lcd.begin(16,2); //Initiate LCD in 16x2 configuration
}

void loop() {
  Wire.beginTransmission(accel);
  Wire.write(0x32); // Prepare to get readings for sensor (address from data sheet)
  Wire.endTransmission(false);
  Wire.requestFrom(accel, 6, true); // Get readings (2 readings per direction x 3 directions = 6 values)
  x = (Wire.read() | Wire.read() << 8); // Parse x values
  y = (Wire.read() | Wire.read() << 8); // Parse y values
  z = (Wire.read() | Wire.read() << 8); // Parse z values

  if (x > 32767) x -= 65536;
  if (y > 32767) y -= 65536;
  if (z > 32767) z -= 65536;

 // Serial.print("x = "); // Print values
 // Serial.print(x);
 // Serial.print(", y = ");
 // Serial.print(y);
 // Serial.print(", z = ");
 // Serial.println(z);

  acc = sqrt(pow(x,2) + pow(y,2) + pow(z,2)); // Calculate acceleration magnitude
 // Serial.println(acc);

  delay(200);

  if (acc > upper_threshold && flag == false) {
    steps += 1;
    flag = true;
  }

  if (acc < lower_threshold && flag == true) {
    steps += 1;
    flag = false;
  }

  Serial.println(steps);

  //prints the current step count on LCD
  lcd.setCursor(0,0);
  lcd.print("Steps:"); 
  lcd.setCursor(0,1);
  lcd.print(steps);
}