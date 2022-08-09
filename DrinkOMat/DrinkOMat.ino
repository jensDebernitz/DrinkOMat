#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>


LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address UNO
unsigned long idleTimerMillis = 0;
// ############ specific configurations ####################

// max = number of buttons (default: 5; even with 1,2 or 3 rows of 5 compartments); if you want to have a button for each compartment increase up to 15 or more
// don't forget to look at EEPROM structure when changing value of max
const int max = 5; 
// number of rows (1 = 5 compartments, 2 = 10 compartments, 3 = 15 compartments)
// if maxrow <=2 the button #5 is used to start refill programm otherwise for open 3rd row during programming
const int maxrow = 1;

unsigned long idlePeriod = 150000; // time in ms between idle messages or shutdown e.g. 180000  

// debug modus: if you need 10 vendors and one additonal PIN (TX0 / Digital PIN 1 which should normally not be used) on UNO turn debug mode to 0 which disables serial 
// I use this pin to send a message to my smarthome system for any purchased product
bool debug = true;

// ########## INIT VALUES ##########
// #max
// #PREIS
int conveyorPrice[5] = {500, 500, 500, 500, 500}; // default price  
int conveyorItems[5] = {1, 1, 1, 1, 1};

// ########## INIT COIN ACCEPTOR ##########

const int coinInt = 0; // attach coinInt to interrupt pin 0 = digital pin 2 = digitalPinToInterrupt(2) . (interrupt pin 1 = digital pin 3 = digitalPinToInterrupt(3))

// set the coinsCurrentValue to a volatile float
// volatile as this variable changes any time the Interrupt is triggered
volatile int coinsCurrentValue = 0;
int coinsChange = 0; // a coin has been inserted flag

unsigned long currentMillis = 0;
unsigned long oldMillis = 0;

int pulsecount;

// const int relaisPin = xx; //
const int relays[5] = {3, 4, 5, 6, 7}; //uno: 5 or 10 boxes

const int selector[5] = {17, 16, 15, 14, 0 }; // input pins for selector buttons (A0=14, A1=15, A2=16, A3=17, Dig0=0) UNO

// PINS
const int configbutton = 13;   // 13 for Uno
const int refillbutton = 12;  // for Uno 12 (if only one row of compartments exists: maxrow = 1)
const int powersave_relais_pin = 11;  // for Uno 11

void setup() 
{
  if (debug) 
  {
    Serial.begin(9600); // start serial communication
    Serial.println("Max: ");
    Serial.println(max);
  }

 for (int index = 0; index < max; index++) 
 {
    pinMode(selector[index], INPUT_PULLUP);
 }
 
  pinMode(configbutton, INPUT_PULLUP);
  pinMode(refillbutton, INPUT_PULLUP);  

  lcd.begin(16, 2);
  lcd.print("Gude Drink-O-Mat");

  if(debug)
  {
    Serial.println("Wait on...");
  }

  delay(2000);

  attachInterrupt(coinInt, coinInserted, RISING);

  for (int index = 0; index < (max * maxrow); index++) 
  {
    digitalWrite(relays[index], HIGH); // Turn OFF
    pinMode(relays[index], OUTPUT);
  }

  delay(200);
  coinsCurrentValue = 0;
  lcd.print("Guthaben reset.");
  delay(200);

  if (debug) 
  {
    Serial.println("Bereit");
  }  

  lcd.clear();
  lcd.print("Bereit");
}

void loop() 
{
  // put your main code here, to run repeatedly:

}

// ########## COIN INSERT ##########

// This function is called by interrupt every time we receives a pulse from the coin acceptor
void coinInserted() 
{
  coinsChange = 1; // flag that there has been a coin inserted

  currentMillis = millis();
  int difference = currentMillis - oldMillis;

  //  Serial.print("difference: ");
  if (debug) 
  {
    Serial.println(difference); 
  } 
  oldMillis = currentMillis;
  pulsecount++;

// new coin? start to count from beginning....   
if (difference > 140 or difference < 50 ) 
{
    if (pulsecount > 1) 
    {
       pulsecount = 1;
     }
   }

  if (difference < 141) 
  {
    switch (pulsecount) 
    {
    case 2: coinsCurrentValue += 10;  
        break; 
    case 3: coinsCurrentValue += 10;  
        break; 
    case 4: coinsCurrentValue += 30;  
        break; 
    case 5: coinsCurrentValue += 50;  
        break; 
    case 6: coinsCurrentValue += 100;  
        break;         
    }
  }
  
  idleTimerMillis = millis();
  
  if (debug) 
  {    
    Serial.print ("(/coinsInserted)neuer Wert: ");
    Serial.println (  coinsCurrentValue);
  }
}
  