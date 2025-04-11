#include <Wire.h> // Necessary for I2C communication
#include <LiquidCrystal.h> //LCD library
#include <ArduinoBLE.h> //Arduino Bluetooth library
LiquidCrystal lcd(7,6,5,4,3,2); // initialize the LCD object
BLEService newService("180A"); // Creates the service
BLEByteCharacteristic stepsChar("2A58", BLERead); //initialize steps characteristic
BLEByteCharacteristic writeChar("2A57", BLEWrite); //initializae write characteristic

//accelerometer initialize
int accel = 0x53; // I2C address for this sensor (from data sheet)
float x, y, z, acc;
//float kcal;
// declare steps flag for data storage
bool flag = false;
// define step counting thresholds
int steps_upper_threshold = 250;
int steps_lower_threshold = 220;
int steps; // initialize steps variable

//buttons initialize
int SelectButton = A0; // select button pin
int CycleButton = A4;  // cycle button pin

//Calorie variables
String heightOptions[] = {"<60in", "61-65in", "66-72in",">73in"}; //the height range options
String weightOptions[] = {"<100lbs", "101-130lbs", "131-160lbs", "161-190lbs", "191-220lbs", "221-250lbs", ">251lbs"}; //weight range options
//String stridelengths[] = {"24in", "26in", "29in", "31in"};  //the respective stride lengths for each height option. listed for reference
                                                              //stride length = distance back of one heel to the other during a step-in-stride
                                                              //these are averaged b/w men and women for our selected ranges.
int heightIndex = 0;
int weightIndex = 0;
String height = "";
String weight = "";
int inputState = 0; //calorie info flag: 0=idle(counting steps only), 1=selecting height, 2=selecting weight, 3=counting steps and calories
float MET = 3.8;  //MET for specific activity code 17190 of the 2024 Adult Compendium
                //MET defined as 1 kcal/(kg*hour). Roughly equivalent to energy cost of sitting quietly.
float walkingrate = 3.1;  // Miles per Hour. The midpoint of the range for the 17190 MET Activity. a constant.
                        // We assume the person is walking at a "moderate" pace, defined here as 3.1 MPH.
float kcal = 0; //initializes the cal variable which is the cal burned by the user.
float timetaken = 0; //initializes the time variable used to calculate cals burned
int userweight = 0; //initializes the user weight variable used in the cal burned calculations.
float KGuserweight = 0; //initializes the converted kilogram userweight for calories burned calculations.
int stridelength = 0; //initializes user stride length, for use in calories burned calculations

void setup() {
  Serial.begin(9600);
  Wire.begin(); // Initialize serial communications
  Wire.beginTransmission(accel); // Start communicating with the device
  Wire.write(0x2D); // Enable measurement
  Wire.write(8); // Get sample measurement
  Wire.endTransmission();
  lcd.begin(16,2); //Initiate LCD in 16x2 configuration

//Bluetooth device setup
  BLE.begin();
  BLE.setLocalName("LePoookie");
  BLE.setAdvertisedService(newService);
  newService.addCharacteristic(stepsChar);
  newService.addCharacteristic(writeChar);
  BLE.addService(newService);
  stepsChar.writeValue(0);
  writeChar.writeValue(0);
  BLE.advertise(); // Look for a Bluetooth Connection
  Serial.println("Bluetooth Device Active");

//button setup
  pinMode(A0,INPUT); //input for the Enter Button
  pinMode(A4,INPUT); //input for the Cycle Button
}

void loop() {
  //Button states (active = HIGH)
  bool selectPressed = digitalRead(SelectButton) == HIGH; //reads the Select Button's state
  bool cyclePressed = digitalRead(CycleButton) == HIGH;   //reads the Cycle Button's state
  
  if (inputState == 0 && selectPressed == LOW) {
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

    //Serial.println(steps); //debugger

    //prints the current step count on LCD
    lcd.setCursor(0,0);
    lcd.print("Steps:"); 
    lcd.setCursor(6,0);
    lcd.print(steps);
  }

  //Calorie counting functionality
  
  else if (inputState == 0 && selectPressed) {
    //starts height selection
    lcd.setCursor(0,1);
    lcd.print("Height:");
    lcd.setCursor(8,1);
    lcd.print(heightOptions[heightIndex]); //displays height options
    inputState = 1; // sets flag to selecting height state
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
      //calculate and store user stride length based on the selected height
      if (height == "<60in")        {stridelength = 24; }
      else if (height == "61-65in") {stridelength = 26; }
      else if (height == "66-72in") {stridelength = 29; }
      else if (height == ">73in")   {stridelength = 31; }
      //Serial.println("stridelength:"); //debugger to test if correct stride range is stored
      //Serial.println(stridelength); //debugger to test if correct stride range is stored
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Height saved");
      //Serial.println("Height:"); //debugger to test if correct height range is stored
      //Serial.println(height); //debugger to verify correct height range is stored
      delay(1500);
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Weight:");
      lcd.setCursor(8,1);
      lcd.print(weightOptions[weightIndex]);
      inputState = 2; // sets flag to selecting weight state
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
      //calculate and store user's weight in kilograms
      if (weight == "<100lbs")         {userweight = 85; }
      else if (weight == "101-130lbs") {userweight = 115; }
      else if (weight == "131-160lbs") {userweight = 145; }
      else if (weight == "161-190lbs") {userweight = 175; }
      else if (weight == "191-220lbs") {userweight = 205; }
      else if (weight == "221-250lbs") {userweight = 235; }
      else if (weight == ">251lbs")    {userweight = 265; }
      KGuserweight = userweight /2.205; //converts pounds (lbs) to kilograms (kg)
      lcd.setCursor(0,1);
      lcd.print("                ");
      lcd.setCursor(0,1);
      lcd.print("Weight saved");
      //Serial.println("userweight:"); //debugger to test if correct weight range is stored
      //Serial.println(userweight); //debugger to test if correct weight range is stored
      delay(1500);
      lcd.setCursor(0,1);
      lcd.print("                ");
      inputState = 3; //sets flag to counting steps and calories state
    }
  }
  else if (inputState ==3) {
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

    //Serial.println(steps); //debugger

    //prints the current step count on LCD
    lcd.setCursor(0,0);
    lcd.print("Steps:"); 
    lcd.setCursor(6,0);
    lcd.print(steps);
    //calculate calories
    timetaken = steps * stridelength / 12.0 / 5280.0 / walkingrate; //calculates the current time the user has been walking
    kcal = MET * KGuserweight * timetaken; // calculates the current calories burned by the user.
    //Serial.println("cal:" && cal);
    lcd.setCursor(0,1);
    lcd.print("kcal:");
    lcd.setCursor(5,1);
    lcd.print(kcal,2); //prints the current calories burned.
  }

  //Bluetooth functionality
  BLEDevice central = BLE.central(); //wait for a BLE central
  if (central) {  // if a central is connected to the peripheral
    stepsChar.writeValue(steps); //Calls the steps value for the central to read from the peripheral
  }

}