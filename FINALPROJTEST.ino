#include "DHT.h"
#include "Arduino.h"
#include <Keypad.h>
#include <TFT_HX8357.h> // Hardware-specific library
#include "SPI.h"
#include <SD.h>

/*******************************************************/

const int MISTERPUMPRELAY = 8;  // Mister Pump PIN 1
const int SOILPUMPRELAY = 9; // Soil Pump PIN 0
const int HEATRELAY = 5;  //  Heat PIN 5
const int FANRELAY = 7;   //  Fan PIN 7
const int PELTIERRELAY = 6;   //Cool PIN 6  
const int MISTERSOLENOIDRELAY = 2;  // Vavle To Water Plants PIN 2                
const int SOILSOLENOIDRELAY = 3;  // Vavle To Water Soil PIN 3     
const int LEDRELAY = 4;  // LED For Lighting PIN 4z

const int TEMPHUMIDITYREADING = A0;    // Temp/Humidity Sensor Reading PIN A0
const int MOISTUREREADING = A2;  // Moisture Sensor Reading PIN A2                   
const int UVLIGHTREADING = A1 ; //UV Lighting Reading PIN A1                    

float WANTEDTEMP = 0;  //User Input Temperature       
float WANTEDHUMIDITY = 0;  //User Input Humidity       
float WANTEDUV = 0;  //User Input UV value            
float WANTEDMOISTURE = 0;  // USER INPUT MOISTURE VALUE    

const int TEMPTOLERANCE = 2;  // Tolerance +-2 for conditions to execute    
const int HUMIDITYTOLERANCE = 5;
const int UVTOLERANCE  ;
const int SOILTOLERANCE = 5;

float tempReadingF; //Reading from sensor
float humidity; //Reading From Sensor
float lighting; //Reading From Sensor
float soilMoisture; //Reading From Sensor

File dataFile;
String currentFileName;
unsigned long lastTime = 0;
unsigned long fileStartTime = 0;
#define SD_CS_PIN 53 // Define the chip select pin for the SD card (should be 53 still)

/**********************************************************/

const byte ROWS = 4; // Four rows
const byte COLS = 4; // Four columns

char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {14, 15, 16, 17}; // Connect to the row pinouts of the keypad
byte colPins[COLS] = {18, 19, 20, 21}; // Connect to the column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

bool keypress = false; // Key press flag

/**************************************************************/
DHT dht22(TEMPHUMIDITYREADING, DHT22);

/**************************************************************/
//LCD

TFT_HX8357 tft = TFT_HX8357();       // Invoke custom library
#define CENTER 240
#define LEFT 1

//void init_SD();
void displayUserValues(); //call function that will show all user input values 
void Moisture(); // Calls Moisture Function
void Humidity(); // Calls Humidity Function
void Temperature(); // Calls Temperature Function
void UV(); // Calls UV Funtion

void setup() 
{
  Serial.begin(115200);
  tft.init();
  tft.setRotation(1);
  init_SD(); // Call SD initialization function
  dht22.begin(); // initialize the DHT22 sensor Temp And Humidity 
  pinMode(HEATRELAY, OUTPUT); // Set Heat Relat To Output
  pinMode(FANRELAY, OUTPUT);  //Set Fan Relay to Output
  pinMode(PELTIERRELAY, OUTPUT);  // Set Peltier Relay To Output
  pinMode(LEDRELAY, OUTPUT); // Set Heat Relat To Output
  pinMode(MISTERPUMPRELAY, OUTPUT); // Set Heat Relat To Output
  pinMode(SOILPUMPRELAY, OUTPUT); // Set Heat Relat To Output
  pinMode(MISTERSOLENOIDRELAY, OUTPUT); // Set Heat Relat To Output
  pinMode(SOILSOLENOIDRELAY, OUTPUT); // Set Heat Relat To Output
  digitalWrite(HEATRELAY, LOW); // Seat Heat Relay To Low
  digitalWrite(FANRELAY, LOW);  // Set Fan Relay To Low
  digitalWrite(PELTIERRELAY, LOW);  // Set Peltier Relay To Low
  digitalWrite(LEDRELAY, LOW); // Set LED Relat To Low
  digitalWrite(MISTERPUMPRELAY, LOW); // Set Mister Pump Relay To Low
  digitalWrite(SOILPUMPRELAY, LOW); // Set Soil Pump Relay To Low
  digitalWrite(MISTERSOLENOIDRELAY, LOW); // Set Mister Solenoid Valve Relay To Low
  digitalWrite(SOILSOLENOIDRELAY, LOW); //Set Soil Valve Solenoid Relay To Low
  digitalWrite(UVLIGHTREADING, INPUT);  // Sets UV Light Reading To Input
  digitalWrite(MOISTUREREADING, INPUT); // Sets Moisture Reading To Input
}

/**********************************************************/

unsigned long lastKeyPress = 0; // Time since the last key press
const unsigned long keypadDebounce = 200; // Debounce time in ms
unsigned long lastMeasurement = 0; // Variable to hold time since last measurement
const unsigned long measurementDelay = 2000; // Amount of time to delay the measurements in ms

void loop() {
  char key = keypad.getKey();
  unsigned long currentTime = millis();

  if (key) {
    // Time the debounce 
    if (currentTime - lastKeyPress >= keypadDebounce) {
      lastKeyPress = currentTime; // Update the time since last key press

      switch (key) {
        case 'A': // Press "A" on keypad to start selecting temperature
          WANTEDTEMP = getInput("Temperature (Â°F): ");
          break;
        case 'B': // Press "B" on keypad to start selecting humidity
          WANTEDHUMIDITY = getInput("Humidity (%): ");
          break;
        case 'C': // Press "C" on keypad to start selecting UV
          WANTEDUV = getInput("UV Index: ");
          break;
        case 'D': // Press "D" on keypad to start selecting soil moisture percentage
          WANTEDMOISTURE = getInput("Soil Moisture (%): ");
          break;
        case '#': // Press "#" on keypad (# to confirm and then again a second time to display all)
          displayUserValues(); // Call function that will show all user input values
          break;
      }
    }
  }


  if (currentTime - lastMeasurement >= measurementDelay){ // If the 2 seconds have passed since last measurement
    lastMeasurement = currentTime; // Reset last measurement time and call all functions ever 2 seconds
  
  // Check if all sensor values are set and only then will the system run
  if (WANTEDTEMP > 0 && WANTEDHUMIDITY > 0 && WANTEDUV > 0 && WANTEDMOISTURE > 0) {
    // Inside this loop will contain the functionality of the project
    Moisture(); // Calls Moisture Function
    Humidity(); // Calls Humidity Function
    Temperature(); // Calls Temperature Function
    UV(); // Calls UV Function


  if (newDayTest())
    { // Call boolean function for new day test and if it comes back true, update the file name
      updateFileName(); // Call function that updates file name
    }

    log_SD(); // Call data logging function to log sensor data

    //delay(1000);


  } else {
     tft.fillScreen(TFT_BLACK);
     tft.setTextColor(TFT_WHITE,TFT_RED);
     tft.setCursor(90,20,4);  
  
    tft.print("All values are not set by user");
  }
  }
  displayUserValues();
}

/**********************************************************/

void Temperature()  //Function
{
    tempReadingF = dht22.readTemperature(true);     // read temperature as Fahrenheit

    Serial.print("Temperature: ");  // Prints Comment
    Serial.print(tempReadingF); //Prints Value Of Reading
    Serial.println("°F"); //Prints Comment    
  
     if (tempReadingF  > WANTEDTEMP + TEMPTOLERANCE) //Condition If Wanted Temp + Tolerance Is Greater Than Reading Temp       

    {

    digitalWrite(FANRELAY, HIGH); // Sets Relay To High
    digitalWrite(PELTIERRELAY,HIGH); // Sets Peltier Relay To High 
    digitalWrite(HEATRELAY,LOW); // Sets Peltier Relay To High 

    Serial.print("FAN AND Peltier ON\n"); // Prints Comment
 
    }

      else if (tempReadingF  < WANTEDTEMP - TEMPTOLERANCE)  // Condition If Reading Temp Is Less Than Wanted Temp - Temp Tolerance

    {

    digitalWrite(FANRELAY, HIGH); // Sets Fan Relay To High
    digitalWrite(HEATRELAY,HIGH); // Sets Heat Realy To High
    digitalWrite(PELTIERRELAY,LOW); // Sets Peltier Relay To High 

    Serial.print("FAN AND HEAT ON!\n"); // Prints Comment
    
    }
   else
     { 

    digitalWrite(FANRELAY, LOW);   // Sets Fan Relay To Low
    digitalWrite(HEATRELAY, LOW); // Sets Heat Realy To High
    digitalWrite(PELTIERRELAY, LOW); // Sets Peltier Relay To High 
    Serial.print("FAN, HEAT, PELTIER OFF\n");  // Print The Comment
     }


}

/**********************************************************/

void Humidity() //Function
{

    humidity  = dht22.readHumidity(); // read humidity

    Serial.print("Humidity: "); // Prints Comment
    Serial.print(humidity); // Prints Humidity Value
    Serial.print("%\n");  // Prints Comment

    if (humidity < WANTEDHUMIDITY - HUMIDITYTOLERANCE ) // Condition  

  {

    digitalWrite(MISTERPUMPRELAY, HIGH); // Turns Mister Pump Relay On
    digitalWrite(MISTERSOLENOIDRELAY, HIGH); //Turns Mistsr Solenoid On

    Serial.print("MISTER PUMP ON, MISTER SOLENOID ON\n");  // Prints Comment
  
  }
  else if (humidity > WANTEDHUMIDITY + HUMIDITYTOLERANCE )  //Condition
  {
    digitalWrite(MISTERPUMPRELAY, LOW); //Turns Mister Pump Relay OFF
    digitalWrite(MISTERSOLENOIDRELAY, LOW); //Turns Mister Solenoif OFF
    Serial.print("MISTER PUMP OFF, MISTER SOLENOID OFF\n"); // Prints Comment
  }

}

/**********************************************************/

void UV() //FUnction
{
float mV = uvSensor.read(); // Reads the UV sensor voltage value using library
lighting = uvSensor.index(mV); // Converts from mV to index using library

Serial.print("UV Sensor Reading: "); // Prints Comment
Serial.print(lighting);   // Prints Reading
Serial.print("\n");   // Prints Reading


  if(lighting < WANTEDUV)
  {
      digitalWrite(LEDRELAY, HIGH); // Set LED Relay To High
      Serial.print("LED Is ON\n");
  }

  else if (lighting >= WANTEDUV)
  {
    digitalWrite(LEDRELAY, LOW); // Sets LED Relay Off

    Serial.print("LED OFF\n");
  }


}

/**********************************************************/

void Moisture() //Function
{
  soilMoisture = analogRead(MOISTUREREADING);
  
  Serial.print(" Moisture Reading:");  // Printts Comment
  Serial.print(soilMoisture);  // Prints Moisture Reading
  Serial.print("\n");  // Prints Moisture Reading

   if (soilMoisture > WANTEDMOISTURE + SOILTOLERANCE )

    {
      digitalWrite(SOILSOLENOIDRELAY, LOW); // Sets Solenoid Relay On
     digitalWrite(SOILPUMPRELAY, LOW); // Sets Pump Relay On
      Serial.print (" do nothing\n");
    }
    else if(soilMoisture < WANTEDMOISTURE - SOILTOLERANCE)

  {
    digitalWrite(SOILSOLENOIDRELAY, HIGH); // Sets Solenoid Relay On
    digitalWrite(SOILPUMPRELAY, HIGH); // Sets Pump Relay On
    Serial.print("SOIL PUMP AND VALVE ON\n");
  }

}

/**********************************************************/

float getInput(String prompt) {
  Serial.println(prompt); 
  String input = ""; //string variable that is blank and will be filled by keypad input
  char key;
  
  while (true) {
    key = keypad.getKey();
    if (key) {
      if (key == '#') { // Confirm input if "#" is pressed
        break;
      } else if (key == '*') { // Clear input if "*" is pressed
        input = ""; //string is cleared 
        Serial.println(prompt);
      } else { //if not those cases, add key to the string and continue getting user input
        input += key; //add key to the input string
        Serial.print(key); //print key that was pressed
      }
    }
  }
  Serial.println();
  return input.toFloat(); //Take the keypad input number as a string and convert it to a float for comparison with sensor readings
}

/**********************************************************/

void displayUserValues() //Display all the user desired values (All the serial prints can be changed to LCD screen prints later)
{
  tft.setCursor(10,74,4);
  tft.print("Current: ");

// tft.drawCentreString("Desired conditions for container:", LEFT, 23,2);
  tft.setCursor(10,104);
  tft.print("Temp: ");
  tft.print(tempReadingF);
 // tft.print(" °F");
  
  tft.setCursor(10,134,4);
  tft.print("Humidity: ");
  tft.print(humidity);
 // Serial.println(" %");
  
  tft.setCursor(10,164,4);
  tft.print("UV Index: ");
  tft.print(lighting);
  //Serial.println();
  
  tft.setCursor(10,194,4);
  tft.print("Soil Moisture: ");
  tft.print(soilMoisture);
  //Serial.println(" %");
  //Serial.println("\n");

  tft.setCursor(250,74,4);
  tft.print("User Input:");
  
  tft.setCursor(250,104,4);
  tft.print("Temp: ");
  tft.print(WANTEDTEMP);
  
  tft.setCursor(250,134,4);
  tft.print("Humidity: ");
  tft.print(WANTEDHUMIDITY);

  tft.setCursor(250,164,4);
  tft.print("UV Index: ");
  tft.print(WANTEDUV);
  
  tft.setCursor(250,194,4);
  tft.print("Soil Moisture: ");
  tft.print(WANTEDMOISTURE);
}


/**********************************************************/

void updateFileName() 
{
  // Create a specific file name that reflects the day since start that the data is collected on
  String fileName = "data_"; // This will be the prefix of the file
  unsigned long currentMillis = millis();
 
  // Create a file name based on the number of days since the system started running
  unsigned long daysSinceStart = currentMillis / 86400000;  // Divide by number of milliseconds in a day (86400000)
  fileName += String(daysSinceStart) + ".txt"; // Add this in string for the file name and then finish with .txt to have a new file for each day


  // Close the previous file if open so that there will be no errors for an open file
  if (dataFile)
  {
    dataFile.close();
  }


  // Open new file with newly created file name for storing the next day's data
  dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile) 
  { // Include an error handling case
    Serial.println("Error opening file for writing!");
  } 
  else 
  { // If file is opened correctly, print that to the system and show the current file that is running
    Serial.print("Writing to the file named ");
    Serial.println(fileName);
  }
}

/**********************************************************/

void init_SD()
{
  // Initialize SD card
  if (!SD.begin(SD_CS_PIN)) 
    {
      Serial.println("SD card initialization failed!");
      return;
    }
      Serial.println("SD card initialized.");
      // Start the data logging process
     updateFileName(); // Create first file name on setup
      fileStartTime = millis();  // Set the start time for the current file
}

/**********************************************************/

void log_SD()
{
  // Write data to SD card file each loop using comma delimiters for purpose of Excel graphing
  if (dataFile) 
    {
      dataFile.print(tempReadingF); // Order of data in file is Temp, Humidity, Moisture, and then UV
      dataFile.print(",");
      dataFile.print(humidity);
      dataFile.print(",");
      dataFile.print(soilMoisture);
      dataFile.print(",");
      dataFile.println(lighting);
      dataFile.flush(); // Make sure data is written and saved to the card
    }   
     else 
    {
      Serial.println("Error writing to file!");
    }
}

/**********************************************************/

bool newDayTest() // Will need this function over many days because of limitation for millis() function (have to handle overflow case)
{
  unsigned long currentMillis = millis();
  // Test if 24 hours has passed with an extra logic case handling possible overflow (will happen after about 49 or 50 days according to reference page on millis function)
  return (currentMillis - fileStartTime >= 86400000) || (currentMillis < fileStartTime && (fileStartTime - currentMillis) >= 86400000);
}

