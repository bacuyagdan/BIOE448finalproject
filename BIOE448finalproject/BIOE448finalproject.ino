#include <Wire.h> // Necessary for I2C communication

int accel = 0x53; // I2C address for this sensor (from data sheet)
float x, y, z;

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize serial communications
  Wire.beginTransmission(accel); // Start communicating with the device
  Wire.write(0x2D); // Enable measurement
  Wire.write(8); // Get sample measurement
  Wire.endTransmission();
}

void loop() {
  Wire.beginTransmission(accel);
  Wire.write(0x32); // Prepare to get readings for sensor (address from data sheet)
  Wire.endTransmission(false);
  Wire.requestFrom(accel, 6, true); // Get readings (2 readings per direction x 3 directions = 6 values)
  x = (Wire.read() | Wire.read() << 8); // Parse x values
  y = (Wire.read() | Wire.read() << 8); // Parse y values
  y = (Wire.read() | Wire.read() << 8); // Parse z values

  Serial.print("x = "); // Print values
  Serial.print(x);
  Serial.print(", y = ");
  Serial.print(y);
  Serial.print(", z = ");
  Serial.println(z);
  delay(200);
}