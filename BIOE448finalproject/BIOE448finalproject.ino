#include <Wire.h> // Necessary for I2C communication
#include <LiquidCrystal.h> //LCD library
#include <ArduinoBLE.h> //Arduino Bluetooth library
LiquidCrystal lcd(7,6,5,4,3,2); // initialize the LCD object
BLEService newService("180A"); // Creates the service
BLEByteCharacteristic readChar("2A58", BLERead); //initialize read characteristic
BLEByteCharacteristic writeChar("2A57", BLEWrite); //initializae write characteristic

//accelerometer initialize
int accel = 0x53; // I2C address for this sensor (from data sheet)
float x, y, z, acc;
float kcal;
// declare steps flag for data storage
bool flag = false;
// define step counting thresholds
int steps_upper_threshold = 250;
int steps_lower_threshold = 220;
int steps; // initialize steps variable

//buttons initialize
int SelectButton = A0; // select button pin
int CycleButton = A4; // cycle button pin

//variables
String heightOptions[] = {"<60in", "61-65in", "66-72in",">73in"}; //the height range options
String weightOptions[] = {"<100lbs", "101-130lbs", "131-160lbs", "161-190lbs", "191-220lbs", "221-250lbs", ">251lbs"}; //weight range options
int heightIndex = 0;
int weightIndex = 0;
String height = "";
String weight = "";
int inputState = 0; //0=idle, 1=selecting height, 2=selecting weight

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize serial communications
  Wire.beginTransmission(accel); // Start communicating with the device
  Wire.write(0x2D); // Enable measurement
  Wire.write(8); // Get sample measurement
  Wire.endTransmission();
  lcd.begin(16,2); //Initiate LCD in 16x2 configuration
  pinMode(SelectButton, INPUT_PULLUP);
  pinMode(CycleButton, INPUT_PULLUP);

//Bluetooth device setup
  BLE.begin();
  BLE.setLocalName("LePoookie");
  BLE.setAdvertisedService(newService);
  newService.addCharacteristic(readChar); 
  newService.addCharacteristic(writeChar);
  BLE.addService(newService);
  readChar.writeValue(0);
  writeChar.writeValue(0);
  BLE.advertise(); // Look for a Bluetooth Connection
  Serial.println("Bluetooth Device Active");

//button setup
  pinMode(A0,INPUT); //input for the Enter Button
  pinMode(A4,INPUT); //input for the Cycle Button
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

  if (acc > steps_upper_threshold && flag == false) {
    steps += 1;
    flag = true;
  }

  if (acc < steps_lower_threshold && flag == true) {
    steps += 1;
    flag = false;
  }

  //Serial.println(steps);

  //prints the current step count on LCD
  lcd.setCursor(0,0);
  lcd.print("Steps:"); 
  lcd.setCursor(6,0);
  lcd.print(steps);

  //Bluetooth functionality
  BLEDevice central = BLE.central(); //wait for a BLE central
  if (central) {  // if a central is connected to the peripheral
    readChar.writeValue(steps); //Calls the steps value for the central to read from the peripheral
  }

  //Calorie counting functionality
  //Button states (active HIGH)
  bool selectPressed = digitalRead(SelectButton) == HIGH;
  bool cyclePressed = digitalRead(CycleButton) == HIGH;
  if (inputState == 0 && selectPressed){
    //starts height selection
    lcd.setCursor(0,1);
    lcd.print("Height:");
    lcd.setCursor(8,1);
    lcd.print(heightOptions[heightIndex]); //displays height options
    inputState = 1; // selecting height state
    delay(200);
  }
  else if (inputState ==1) {
    if (cyclePressed) {
      heightIndex = (heightIndex +1) % (sizeof(heightOptions) / sizeof(heightOptions[0])); //goes to next height range option
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print(heightOptions[heightIndex]); //displays the next height range option
      delay(200);
    }
    if (selectPressed){
      height = heightOptions[heightIndex]; //saves the currently shown height range as the user's height
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Height saved");
      Serial.println(height);
      delay(2000);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Weight:");
      lcd.setCursor(8,1);
      lcd.print(weightOptions[weightIndex]);
      inputState = 2;
      delay(200);
    }
  }
  else if (inputState == 2){
    if (cyclePressed) {
      weightIndex = (weightIndex +1) % (sizeof(weightOptions) / sizeof(weightOptions[0])); //goes to next weight range option
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print(weightOptions[weightIndex]); //displays the next weight range option
      delay(200);
    }
    if (selectPressed) {
      weight = weightOptions[weightIndex]; //saves the currently shown weight range as the user's weight
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Weight saved");
      Serial.println(weight);
      delay(2000);
      lcd.setCursor(0,1);
      lcd.print("                ");
      inputState = 0; //resets flag to idle state
    }
  }
}