
#include <Servo.h> 
#include <IOSControllerForHM10.h>
#include <SimpleDHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>


// Declarations for controlling gun mode
bool sentryMode = 0;

// Declarations for temperature sensors
// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
//int pinDHT11 = 9;
//SimpleDHT11 dht11;
//void senseTemperatureAndHumidity(); //Prototype for temperature sensor

//Declarations for gun control
#define FlyWheelPin 5
#define ServoPin 4
#define fullLoad 4
Servo myservo;  // create servo object to control firing servo
int startposition = 80;    // Setting the starting position of servo
int firingposition = 10;    // Setting the firing poistion of servo
bool Firing = 0;           // The current firing state
int load=fullLoad;              // Current rounds
bool Flywheelon = 0;       // Current flywheel state
void fireGun();

//Declarations for bluetooth control
char c;
#define CONNECTIONPIN   2


// Declarations for turntable control
#define Left 1
#define Right 0
#define TurnTableServoPin 12
Servo turnTableServo; // create servo for turntable
bool turnTableDirection = Right;
int turnTablePosition = 90; // Set initial turntable position
int turnTableSpeed = 10; // Speed at which turntable will move in degrees
int maxLeft = 0;
int maxRight = 180;
void sweep();  // Prototype for function to sweep in auto mode

//Declaration for sentryMode
//void detect();
//int pirPin = 14; // Input for HC-S501
//int pirValue; // Place to store read PIR Value


//Declarations for lcd
LiquidCrystal_I2C lcd(0x27,20,4);  // set the LCD address to 0x27 for a 20 chars and 4 line display
char roundsText[20];

/*
*
* Prototypes of IOSController’s callbacks
*
*
*/
void doWork();
void doSync(char *variable);
void processIncomingMessages(char *variable, char *value);
void processOutgoingMessages();
void processAlarms(char *variable);
void deviceConnected();
void deviceDisconnected();
/*
*
* IOSController Library initialization
*
*/
#ifdef ALARMS_SUPPORT 
  IOSControllerForHM10 iosController(&doWork,&doSync,&processIncomingMessages,&processOutgoingMessages,&processAlarms,&deviceConnected,&deviceDisconnected);
#else
  IOSControllerForHM10 iosController(&doWork,&doSync,&processIncomingMessages,&processOutgoingMessages,&deviceConnected,&deviceDisconnected);
#endif

// Main setup routine
void setup() {
  Serial.begin(9600);
  pinMode(FlyWheelPin,OUTPUT); //---set pin direction
  myservo.attach(ServoPin);  // attaches the servo pin to the servo object
  turnTableServo.attach(TurnTableServoPin); // attaches the turntable servo to the pin
  myservo.write(startposition);
  turnTableServo.write(turnTablePosition);
  digitalWrite(FlyWheelPin,HIGH); // off
  Serial.println("Fly wheel off");
  Flywheelon = 0;
//  senseTemperatureAndHumidity();
  iosController.writeMessage("Rounds Left",load);
//  pinMode(pirPin, INPUT);
  lcd.init();  //initialize the lcd
  lcd.backlight();  //open the backlight
  lcd.setCursor ( 0, 0 );            // go to the 2nd row
  lcd.print("   Waiting...    "); // pad string with spaces for centering
}


void loop()
{
  iosController.loop(500);
}

/**
*
*
* This function is called periodically and its equivalent to the standard loop() function
*
*/
void doWork() {
  
 Serial.print("doWork "); Serial.println();

  if (sentryMode)
  {
//   lcd.setCursor ( 0, 0 );            // go to the top left corner
//   lcd.print("    Sentry Mode    "); // write this string on the top row
    sweep();
//    detect();
  }
  else
  { 
     // No ammo so force to not fire
    if (load <= 0 && Firing)
    {
      Firing = 0;
      iosController.writeMessage("Fire",Firing);
    }

// If firing is enabled then fire gun
    if (Firing)
    {
      fireGun();
    }
    else
    // if firing not enabled swith off flywheel if it is on
    {
      if (Flywheelon)
      {
        digitalWrite(FlyWheelPin,HIGH); // off
        Serial.println("Fly wheel off");
        Flywheelon = 0;
        lcd.setCursor ( 0, 0 );            // go to the 2nd row
        lcd.print("   Waiting...    "); // pad string with spaces for centering
      }
    }
  }
}

/*
*
* This function is called when the ios device connects and needs to initialize the position of switches and knobs
*
*/
void doSync (char *variable) {
  
  Serial.print("doSync "); Serial.println(variable);
  
  }


/**
*
*
* This function is called when a new message is received from the iOS device
*
*/
void processIncomingMessages(char *variable, char *value) {

  Serial.println("Got Input");
  Serial.println(variable);
  Serial.println(value);
  if ((strcmp(variable,"Fire")== 0) && (strcmp(value,"1") ==0)) {
      Firing = 1;
  }
 if ((strcmp(variable,"Fire")== 0) && (strcmp(value,"0") ==0)) {
      Firing = 0;
  }

 if ((strcmp(variable,"Reload")==0)&& (strcmp(value,"0") == 0)) {
      load = fullLoad;
      Serial.println("Reloading!");
      sprintf(roundsText, "Rounds : %i ", load);
      lcd.setCursor ( 0, 2 );            // go to the 2nd row
      lcd.print(roundsText); // pad string with spaces for centering
      iosController.writeMessage("Rounds Left",load);
 }

 if ((strcmp(variable,"Left")==0) && (strcmp(value,"1") ==0)){
      if (turnTablePosition > maxLeft) {
        turnTablePosition = turnTablePosition - turnTableSpeed;
        turnTableServo.write(turnTablePosition);
      }
      
 }

  if ((strcmp(variable,"Right")==0) && (strcmp(value,"1") ==0)){
      if (turnTablePosition < maxRight) {
        turnTablePosition = turnTablePosition + turnTableSpeed;
        turnTableServo.write(turnTablePosition);
      }
      
 }
}

/**
*
*
* This function is called periodically and messages can be sent to the iOS device
*
*/
void processOutgoingMessages()
{
}


/**
*
*
* This function is called when an Alarm is fired
*
*/
void processAlarms(char *alarm) {}


/**
*
*
* This function is called when the iOS device connects
*
*/
void deviceConnected() {
  digitalWrite(CONNECTIONPIN,HIGH);  
  Serial.println("Device connected");
  }


/**
*
*
* This function is called when the iOS device disconnects
*
*/
void deviceDisconnected() {
  
  digitalWrite(CONNECTIONPIN,LOW);
  Serial.println("Device disconnected");}

/**
*
*
* This function is called to sense temperature and humidity
*
*/
void senseTemperatureAndHumidity()
{
// start working...
  
  // read with raw sample data.
  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
//  if (dht11.read(pinDHT11, &temperature, &humidity, data)) {
//    Serial.print("Read DHT11 failed");
//    return;
//  }

// Write values back to controller
  iosController.writeMessage("Temperature",(int)temperature);
  iosController.writeMessage("Humidity",(int)humidity);

}

/**
*
*
* This function is called to sweep turret left and right
*
*/
void sweep()
{
    Serial.println("Sweep!");
//   Serial.print("Direction:"); Serial.println(turnTableDirection);
//    Serial.print("Position:"); Serial.println(turnTablePosition);
    
   
    if (turnTableDirection == Left)
    {
      if (turnTablePosition > maxLeft) // If sweep has not reached the maximum position keep turning else change direction
      {
        turnTablePosition = turnTablePosition - turnTableSpeed;
      }
      else
      {
        turnTableDirection = Right;
      }
    }
    else
    {
      if (turnTablePosition < maxRight)   // If sweep has not reached the maximum position keep turning else change direction
      {
        turnTablePosition = turnTablePosition + turnTableSpeed;
      }
      else
      {
        turnTableDirection = Left;
      }
    }
//    char sweepPosition[2];
//    char sweepText[20] = "Sweeping... ";
//    sprintf(sweepPosition, "%i", turnTablePosition);
//    strcat(sweepText,sweepPosition);
//    Serial.println(sweepText);
//    lcd.setCursor ( 0, 1 );            // go to the 2nd row
//    lcd.print(sweepText); // pad string with spaces for centering
    turnTableServo.write(turnTablePosition);
}

/**
*
*
* This function is called to fire the gun
*
*/
void fireGun()
{

    Serial.println("Fire Gun!");
    if (Flywheelon == 0)  /// if flywheel not on then switch it on
    {
        digitalWrite(FlyWheelPin,LOW); // on
        Serial.println("Fly wheel on");
        delay(500); // Delay to allow flywheel to spin up
        Flywheelon = 1;
    }

//---Move servo to firing position
    myservo.write(firingposition); // move firing pin forward
    Serial.print("Firing!!"); Serial.println(firingposition);
    lcd.setCursor ( 0, 0 );            // go to the 2nd row
    lcd.print("   Firing!   "); // pad string with spaces for centering
    delay(500);
    
//--Move servo back to start position
    myservo.write(startposition); // move firing pin back
    delay(500);
    load--;
    sprintf(roundsText, "Rounds : %i ", load);
    lcd.setCursor ( 0, 2 );            // go to the 2nd row
    lcd.print(roundsText); // pad string with spaces for centering
    iosController.writeMessage("Rounds Left",load);

}


/**
*
*
* This function is called to check IR sensor and react
*
*/

void detect()
{

  
}



