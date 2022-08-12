#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>

//initialisierung LCD Display
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // Set the LCD I2C address UNO

//timer einstellungen
unsigned long idleTimerMillis = 0;
unsigned long idlePeriod = 150000; // time in ms between idle messages or shutdown e.g. 180000

//Serielle ausgaben
bool debug = true;

//alles was man für den münzprüfer braucht
const int coinIntteruptPin = 0; // attach coinInt to interrupt pin 0 = digital pin 2 = digitalPinToInterrupt(2) . (interrupt pin 1 = digital pin 3 = digitalPinToInterrupt(3))
volatile int coinsCurrentValue = 0;
int coinsChange = 0; // a coin has been inserted flag
unsigned long currentMillis = 0;
unsigned long oldMillis = 0;
int pulsecount;

//Ports für die Releais
const int relayK1AuswurfTrommel = 3;
const int relayK2Schubladenverschluss = 4;
const int relayK3LedBeleuchtung = 5;

//Ports f+r die schalter
const int switchS1GrundstellungAuswurfTrommel = 6;
const int switchS2SchubladeVerschlossen = 7;
const int switchS3LetzteFlasche = 8;
const int switchS4BestellTaster = 9;

//andere konstanten
const int BOTTLE_PRICE = 100; //in Cent
const char* VERSION_STRING = "0.1 Beta";

//private functions
void initPorts();
void auswurfTrommelZurGrundstellung();
void writeTextOnLcd(const char* message);

void setup()
{
  //init der seriellen Ausgabe
  if (debug)
  {
    Serial.begin(19200); // start serial communication
  }

  //init Ports
  initPorts();

  //init LCD
  lcd.begin(20, 4);
  lcd.clear();
  lcd.backlight();
  writeTextOnLcd("Start...");

  if (debug)
  {
    Serial.println("Wait on...");
  }

  delay(2000);

  //Auswurf Trommel zur Grundstellung zurück fahren
  writeTextOnLcd("Trommel Reset");
  auswurfTrommelZurGrundstellung();

  if (debug)
  {
    Serial.println("Bereit");
  }

  //init Interrupt Pin für den Münzprüfer
  pulsecount = 0;
  attachInterrupt(coinIntteruptPin, coinInserted, RISING);

  delay(200);
  coinsCurrentValue = 0;
  delay(200);
}

void loop()
{

  //ausgabe wenn sich der Letzte Flaschen Schaler geändert hat
  if (debug)
  {
    static unsigned int lastStateLetzteFlasche = -1;

    if (lastStateLetzteFlasche != digitalRead(switchS3LetzteFlasche))
    {
      char buffer[100];
      sprintf(&buffer[0], "Flaschen Leer %i", digitalRead(switchS3LetzteFlasche));
      Serial.println(&buffer[0]);
      lastStateLetzteFlasche = digitalRead(switchS3LetzteFlasche);
    }

  }

  //wenn in main Loop können wir die Releais immer ausschalten
  digitalWrite(relayK3LedBeleuchtung, HIGH);
  digitalWrite(relayK2Schubladenverschluss, HIGH);
  digitalWrite(relayK1AuswurfTrommel, HIGH);

  //prüfung ob der letzte Flaschen schalter gechaltet wurde
  if (digitalRead(switchS3LetzteFlasche) == LOW)
  {
    //anzeige des guthabens
    int coinsBig = coinsCurrentValue / 100;
    //% = Modulo = gibt dir den rest wert einer diffusion = 1112 % 11 wäre dann 12 ;-)
    int coinsLess = coinsCurrentValue % 100;
    char buffer[20];
    sprintf(&buffer[0], "Guthaben %i,%i      ", coinsBig, coinsLess);
    writeTextOnLcd(&buffer[0]);

    //wenn der münzzähler mehr als 99Cent aingeworfen wurden und der bestelltaster betätigt wurde dann lege los
    if (coinsCurrentValue >= 100)
    {
      //Bestelltaster wurde betätigt
      if (digitalRead(switchS4BestellTaster) == LOW)
      {
        //flaschen preis vom guthaben abziehen
        coinsCurrentValue -= BOTTLE_PRICE;

        while (1)
        {
          //es könnte sein das der vortrinker die schublade nicht richtig verschlossen hat,
          //das sollte man abwarten
          if (digitalRead(switchS2SchubladeVerschlossen) == LOW)
          {

            //wenn die schublade richtigverschlossen ist können wir die auswurftrommel anwerfen bis sie eine volle drehung gemacht hat
            writeTextOnLcd("Bier ausgabe        ");
            digitalWrite(relayK1AuswurfTrommel, LOW);
            delay(500);

            //hier warten wir bis die trommel wieder in der grundeinstellung ist
            while (digitalRead(switchS1GrundstellungAuswurfTrommel) == HIGH)
            {
              //do so long to zero position
              if (debug)
              {
                Serial.println("Fahre Trommel bis null punkt");
              }
            }
            digitalWrite(relayK1AuswurfTrommel, HIGH);

            //licht an und schublade auf
            digitalWrite(relayK3LedBeleuchtung, LOW);
            digitalWrite(relayK2Schubladenverschluss, LOW);
            delay(500);

            //so lange warten bis die schublade wieder zu ist
            while (digitalRead(switchS2SchubladeVerschlossen) == HIGH)
            {
              digitalWrite(relayK2Schubladenverschluss, HIGH);
              if (debug)
              {
                Serial.println("warte so lange bis schublade wieder geschlossen wurde");
              }
            }
            digitalWrite(relayK3LedBeleuchtung, HIGH);

            writeTextOnLcd("Viel Spaß beim trinken");
            delay(1000);
            break;
          }
          else
          {
            writeTextOnLcd("Schublade schliessen");

            if (debug)
            {
              Serial.println("schublade ist nicht geschlossen");
            }
          }
        }

      }
    }
  }
  else
  {
    writeTextOnLcd("Leider leer :(      ");
  }

  if (coinsChange == 1)
  {
    coinsChange = 0; // unflag that a coin has been inserted
  }
}


//Text ausgabe auf dem LCD
void writeTextOnLcd(const char* message)
{
  lcd.setCursor(0, 0);
  lcd.print("*** Drink-O-MMat ***");
  lcd.setCursor(0, 1);
  char buffer[20];
  sprintf(&buffer[0], "Gude %s ", VERSION_STRING);
  lcd.print(&buffer[0]);
  lcd.setCursor(0, 2);
  lcd.print("Lust auf ein Bier?");
  lcd.setCursor(0, 3);
  lcd.print(message);
}

//Hilfsfunktion für die grundeinstellung der auswurftrommel
void auswurfTrommelZurGrundstellung()
{
  if (debug)
  {
    char buffer[100];
    sprintf(&buffer[0], "Reset Auswurftrommel %i", digitalRead(switchS1GrundstellungAuswurfTrommel));
    Serial.println(&buffer[0]);
  }

  while (digitalRead(switchS1GrundstellungAuswurfTrommel) == HIGH)
  {
    digitalWrite(relayK1AuswurfTrommel, LOW);
  }

  digitalWrite(relayK1AuswurfTrommel, HIGH);
}

//hilfsfunktion zur initialisieung der Ports
void initPorts()
{
  pinMode(switchS1GrundstellungAuswurfTrommel, INPUT_PULLUP);
  pinMode(switchS2SchubladeVerschlossen, INPUT_PULLUP);
  pinMode(switchS3LetzteFlasche, INPUT_PULLUP);
  pinMode(switchS4BestellTaster, INPUT_PULLUP);
  pinMode(2, INPUT_PULLUP);


  digitalWrite(relayK1AuswurfTrommel, HIGH);
  pinMode(relayK1AuswurfTrommel, OUTPUT);
  digitalWrite(relayK2Schubladenverschluss, HIGH);
  pinMode(relayK2Schubladenverschluss, OUTPUT);
  digitalWrite(relayK3LedBeleuchtung, HIGH);
  pinMode(relayK3LedBeleuchtung, OUTPUT);
}

//interrupt funktion zum zählen der Implulse vom Münzprüfer
void coinInserted()
{
  coinsChange = 1; // flag that there has been a coin inserted

  currentMillis = millis();
  int difference = currentMillis - oldMillis;

  if (debug)
  {
    char buffer[100];
    sprintf(&buffer[0], "Int PIN %i --> Different %i --> ", digitalRead(2), difference);
    Serial.print(&buffer[0]);
  }

  oldMillis = currentMillis;
  pulsecount++;

  // new coin? start to count from beginning....
  if (difference > 140 || difference < 50 )
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
      case 2: coinsCurrentValue += 20;
        break;
      case 3: coinsCurrentValue += 30;
        break;
      case 4: coinsCurrentValue += 50;
        break;
      case 5: coinsCurrentValue += 100;
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
