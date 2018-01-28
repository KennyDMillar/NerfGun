
#include <Servo.h> 
#include <IOSControllerForHM10.h>
#include <SimpleDHT.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif


// Declarations for dart sensor
#define SENSORPIN 4
int sensorState = 0;
int lastState=0;


//Declarations of power mode
bool powerOn = 0;
void turnPowerOn();
void turnPowerOff();

// Declarations for targeting light
#define TARGETLIGHTPIN 3
Adafruit_NeoPixel targetLight = Adafruit_NeoPixel(1, TARGETLIGHTPIN, NEO_GRB + NEO_KHZ800);

// Declarations for muzzle flash
#define MUZZLEFLASHPIN 7
Adafruit_NeoPixel muzzleFlash = Adafruit_NeoPixel(1, MUZZLEFLASHPIN, NEO_GRB + NEO_KHZ800);


// Declarations for controlling gun mode
bool sentryMode = 0;
bool safety = 0;
void safetyModeOn();
void safetyModeOff();

// Declarations for temperature sensors
// for DHT11, 
//      VCC: 5V or 3V
//      GND: GND
//      DATA: 2
int timer = 0;
int pinDHT11 = 9;
SimpleDHT11 dht11;
void senseTemperatureAndHumidity(); //Prototype for temperature sensor

//Declarations for gun control
#define FiringPin 11
#define fullLoad 25         // Maximum capacity of gun
bool Firing = 0;           // The current firing state
int load=fullLoad;              // Current rounds
void fireGun();
void standby();
void stopFiringGun();
void roundFired();
void reload();

//Declarations for bluetooth control
char c;
#define CONNECTIONPIN   2
void resetController();


// Declarations for turntable control
#define Left 1
#define Right 0
#define TurnTableServoPin 12
#define maxRight 60
#define maxLeft 120
#define centre 90
#define turnTableSpeed 10
Servo turnTableServo; // create servo for turntable
bool turnTableDirection = Right; // the direction the table is currently moving
int turnTablePosition = centre; // Set initial turntable position
void turn();
void sweep();  // Prototype for function to sweep in auto mode

// Declarations for neopixel lights
// Which pin on the Arduino is connected to the NeoPixels?
// On a Trinket or Gemma we suggest changing this to 1
#define PIN            6
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS      8
// When we setup the NeoPixel library, we tell it how many pixels, and which pin to use to send signals.
// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);
int delayval = 30; // delay for half a second
void lights(int colour);
void lightsOff();
void sweepLights();
void flash();
#define BLUE 1
#define RED 2
#define GREEN 3
#define AMBER 4

int colour = BLUE;
bool lightDirection = Left;
int currentLightPosition = 0;
#define maxLightLeft  0
#define maxLightRight 7


//Declaration for sentryMode
void detect();
void startSentryMode();
int pirPin = 8; // Input for HC-S501
int pirValue; // Place to store read PIR Value


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
  pinMode(pirPin, INPUT);
  pinMode(FiringPin, OUTPUT);
  digitalWrite(FiringPin,HIGH); // Ensure gun is turned off 

  pinMode(SENSORPIN, INPUT);     // Mode for dart sensor pin
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup
  
  lcd.init();  //initialize the lcd
  lcd.backlight();  //open the backlight
  
  pixels.begin(); // This initializes the NeoPixel library.
  targetLight.begin(); //
  muzzleFlash.begin(); //  
  
  turnPowerOff();  // Ensure device starts up in powered off mode
}


void loop()
{
  iosController.loop(200);
}

/**
*
*
* This function is called periodically and its equivalent to the standard loop() function
*
*/
void doWork() {
  
 // Serial.print("doWork "); Serial.println();

  if (powerOn){
    senseTemperatureAndHumidity();
    
  if (sentryMode)
  {
    sweep();
    detect();
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

  if (powerOn == 0)    // If the power is off then only process power on command
  {
       if ((strcmp(variable,"Power")==0) && (strcmp(value,"1") ==0)){
       powerOn = 1;
       turnPowerOn();
    }
    else
    {
      resetController(); // If command other then power on then reset it
    }
  }
  else
  {

    if ((strcmp(variable, "Sentry Mode")== 0) && (strcmp(value,"1")==0)){
      startSentryMode();
    }
  
    if ((strcmp(variable, "Sentry Mode")== 0) && (strcmp(value,"0")==0)){
      sentryMode = 0;
      standby();
    }
    
     if ((strcmp(variable, "Safety")== 0) && (strcmp(value,"1")==0)){
      safetyModeOn();
    }
  
    if ((strcmp(variable, "Safety")== 0) && (strcmp(value,"0")==0)){
      safetyModeOff();
    }
    
    if ((strcmp(variable,"Fire")== 0) && (strcmp(value,"1") ==0)) {
        fireGun();
    }
    if ((strcmp(variable,"Fire")== 0) && (strcmp(value,"0") ==0)) {
        stopFiringGun();
    }

    if ((strcmp(variable,"Reload")==0)&& (strcmp(value,"1") == 0)) {
        reload();
    }

    if ((strcmp(variable,"Left")==0) && (strcmp(value,"1") ==0)){
          turnTablePosition = maxLeft;
          turn();
    }
    if ((strcmp(variable,"Right")==0) && (strcmp(value,"1") ==0)){
        turnTablePosition = maxRight;
        turn();
    }
 
   if ((strcmp(variable,"Centre")==0) && (strcmp(value,"1") ==0)){
        turnTablePosition = centre;
        turn();
        }

     if ((strcmp(variable,"Power")==0) && (strcmp(value,"0") ==0)){
       powerOn = 0;
       turnPowerOff();
    }

// Voice commands
     if (strcmp(variable,"$VC$")==0)
     {
        if (atoi(value) == 100)   // Power on
        {
            powerOn = 1;
            turnPowerOn();       
        }
        else if (atoi(value) == 200) // Power off
        {
          turnPowerOff();
        }
        else if (atoi(value) == 300) // Fire
        {
          fireGun();
        }
        else if (atoi(value) == 400) // Right
        {
          turnTablePosition = maxRight;
          turn();     
        }
        else if (atoi(value) == 500) // Centre
        {
          turnTablePosition = centre;
           turn();
        }
        else if (atoi(value) == 600) // Left
        {
          turnTablePosition = maxLeft;
          turn();
        }
        else if (atoi(value) == 700) // Sentry
        {
          
        }
        else if (atoi(value) == 800) // Manual
        {
          
        }
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
  Serial.println("Device disconnected");
  }
  
/**
*
*
* This function is called to sense temperature and humidity
*
*/
void senseTemperatureAndHumidity()
{
  
  // read with raw sample data.
  byte temperature = 0;
  byte humidity = 0;
  byte data[40] = {0};
  
 
//  Only sense at an interval
  if (timer == 20)
  {
//    Serial.print("Sensing temperature");
    timer = 0;
  if (dht11.read(pinDHT11, &temperature, &humidity, data)) {
//    Serial.print("Read DHT11 failed");
    return;
  }

// Write values back to controller
  iosController.writeMessage("Temperature",(int)temperature);
  iosController.writeMessage("Humidity",(int)humidity);

  char tempText[20];
   sprintf(tempText, "TEMP : %i C", (int)temperature);
   lcd.setCursor ( 0, 3 );            // go to the 3rd row
   lcd.print(tempText); 
  }
  timer ++;
}

/**
*
*
* This function is called to sweep turret left and right
*
*/
void sweep()
{

    if (turnTablePosition == centre)
    {
      if (turnTableDirection == Left)
      {
        turnTablePosition = maxLeft;
      }
      else
      {
        turnTablePosition = maxRight;
      }
      
    }
    else if (turnTablePosition == maxRight)
    {
      turnTablePosition = centre;
      turnTableDirection = Left;
    }
    else
    {
      turnTablePosition = centre;
      turnTableDirection = Right;
    }
    
//    turn();
//    delay(500);
    
    sweepLights();
}

/**
*
*
* This function is called to fire the gun
*
*/
void fireGun()
{
    if (load > 0)
    {
    
    Firing = 1;
 //   Serial.println("Fire Gun!");
    if (!safety){
      digitalWrite(FiringPin,LOW); // on
    }

    roundFired();
    }
   stopFiringGun();

    if (load > 10)
    {
   sprintf(roundsText, "ROUNDS : %i ", load);
    }
    else
    {
      sprintf(roundsText, "ROUNDS : %i CRITICAL", load);
    }
   lcd.setCursor ( 0, 2 );            // go to the 2nd row
   lcd.print(roundsText); // pad string with spaces for centering
   iosController.writeMessage("Rounds Left",load);

   if (load == 0)
   {
    iosController.writeMessage("Out Of Ammo", 1);
   }
}


//
// Function to count down rounds
//
void roundFired(){

 if (load == fullLoad)
 {
  delay(200); //delay to allow blank round to fire if new belt
 }
 
 int roundsToFire = 2; //Fire 2 rounds per volley
 int roundCount;
 for (roundCount = 1; roundCount <= roundsToFire; roundCount++)
{
   if (load <=0)
   {
    return;
   }
   delay(200); // delay for each shot

   load--; // Decrement load counter
   flash(); //Flash barrel lights

  }
}


//
// Function to reload ammo count
//

void reload(){
      load = fullLoad;
      sprintf(roundsText, "ROUNDS : %i        ", load);
      lcd.setCursor ( 0, 2 );            // go to the 2nd row
      lcd.print(roundsText); // pad string with spaces for centering
      iosController.writeMessage("Rounds Left",load);
      iosController.writeMessage("Out Of Ammo", 0);
}


//
// Function called to stop firing gun
//

void stopFiringGun()
{
    Firing = 0;
    digitalWrite(FiringPin,HIGH); // off
    iosController.writeMessage("Fire",Firing);  // Reset controller
}

/**
*
*
* This function is called to check IR sensor and react
*
*/

void detect()
{
      pirValue = digitalRead(pirPin);
      Serial.println("Detect PIR:");
      Serial.println(pirValue);
      Serial.println("\n");

      if (pirValue > 0)
      {
          fireGun();
          delay(1000);
    }
}

/**
*
*
* This function is called when entering standby
*
*/

void standby()
{

     // ensure gun is turned off
      stopFiringGun();
    // centre turret
      turnTablePosition = centre;
      turn();

   // write message to lcd
    lcd.setCursor ( 0, 0 );            // go to first row
    lcd.print("SYSTEM MODE: MANUAL"); // pad string with spaces for centering
    
    // write current rounds to controller
    iosController.writeMessage("Rounds Left",load);

  // set lights to blue
  lightsOff();
  colour = BLUE;
  lights(BLUE);

  targetLight.setPixelColor(0, targetLight.Color(24,202,230)); // Bright blue
  targetLight.show(); // This sends the updated pixel color to the hardware.
}


/**
*
*
* This function is called to turn turntable
* 
*
*/

void turn()
{
        turnTableServo.attach(TurnTableServoPin); // attaches the turntable servo to the pin
        delay(100);
        turnTableServo.write(turnTablePosition);
        delay(500);
        turnTableServo.detach(); // detach to avoid vibration

}


/**
*
*
* This function is called to turn on the lights
* 
*
*/

void lights(int colour)
{
  for(int i=0;i<NUMPIXELS;i++){

    if (colour == BLUE){
    pixels.setPixelColor(i, pixels.Color(24,202,230)); // Moderately bright blue color.
 //    pixels.setPixelColor(i, pixels.Color(50, 25, 1)); // Moderately bright copper color.

    }
    else if (colour == RED){
    pixels.setPixelColor(i, pixels.Color(255,140,0)); // Moderately bright red color.
    }
    else if (colour == GREEN){
    pixels.setPixelColor(i, pixels.Color(0,128,0)); // Moderately bright green color.
    }
    else if (colour == AMBER){
      pixels.setPixelColor(i, pixels.Color(255,191,0)); // Moderately bright amber color.
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(50); // Delay for a period of time (in milliseconds).

  }
}

//
//Function to flash the muzzle
//

void flash()
{
   muzzleFlash.setPixelColor(0, muzzleFlash.Color(255,255,255)); // Bright white
   muzzleFlash.show(); // This sends the updated pixel color to the hardware.
   delay(25);
   muzzleFlash.setPixelColor(0, 0, 0, 0, 0); // switch off light
   muzzleFlash.show();
}


//
// Function to switch off lights
//
void lightsOff()
{
for(int i=NUMPIXELS-1;i>-1;i--){

    pixels.setPixelColor(i, 0, 0, 0, 0); // switch off light
    pixels.show(); // This sends the updated pixel color to the hardware.
    delay(50); // Delay for a period of time (in milliseconds).
}
}

//
// Function to perform "Cylon" style sweeping lights
//

void sweepLights()
{
    Serial.println("sweepLights");
    Serial.println(currentLightPosition);
    
    pixels.setPixelColor(currentLightPosition, 0, 0, 0, 0); // turn off current light
    if (lightDirection == Left)
    {
      if (currentLightPosition > maxLightLeft) // If sweep has not reached the maximum position keep turning else change direction
      {
        currentLightPosition = currentLightPosition - 1;
      }
      else
      {
        lightDirection = Right;
      }
    }
    else
    {
      if (currentLightPosition < maxLightRight)   // If sweep has not reached the maximum position keep turning else change direction
      {
        currentLightPosition = currentLightPosition + 1;
      }
      else
      {
        lightDirection = Left;
      }
    }
    pixels.setPixelColor(currentLightPosition, 50, 0, 0);
    pixels.show();
}


//
// Function to go through power on cycle
//

void turnPowerOn()
{
    // write message to lcd
    lcd.setCursor ( 0, 0 );            // go to first row
    lcd.print("    POWERING UP    "); // pad string with spaces for centering
    lcd.backlight();  //open the backlight

    standby();
    safetyModeOff();
     
}


//
// Function to go through power down cycle
//

void turnPowerOff()
{
      powerOn = 0;
      sentryMode = 0;
      stopFiringGun();

     // centre turret
      turnTablePosition = centre;
      turn();
      
      resetController();
      targetLight.setPixelColor(0, targetLight.Color(0,0,0, 0)); // Turn light off
      targetLight.show(); // This sends the updated pixel color to the hardware.
      lightsOff();
      lcd.noBacklight();  //close the backlight
}


//
// Function to setup sentry mode
//

void startSentryMode(){
      sentryMode = 1;
      lcd.setCursor ( 0, 0 );            // go to the top left corner
      lcd.print("SYSTEM MODE: AUTO  "); // write this string on the top row
      lightsOff();
      colour = RED;
      currentLightPosition = 0;
      lightDirection = Left;
      sweepLights();
      targetLight.setPixelColor(0, targetLight.Color(250,0,0)); // Bright red
      targetLight.show(); // This sends the updated pixel color to the hardware.
}

//
// Function to turn safety mode on
//

void safetyModeOn(){
    safety = 1;
    lcd.setCursor ( 0, 1 );            // go to the 2nd row
    lcd.print("SAFETY: ON  "); // write this string
}

//
// Function to turn safety mode off
//

void safetyModeOff(){
 
    safety = 0;
    lcd.setCursor ( 0, 1 );            // go to the 2nd row
    lcd.print("SAFETY: OFF  "); // write this string
    iosController.writeMessage("Safety", safety);

}
//
// Function to reset controller
//

void resetController(){
      iosController.writeMessage("Fire",Firing);  // Reset firing controller
      iosController.writeMessage("Power", powerOn);
      iosController.writeMessage("Sentry Mode", sentryMode);
      iosController.writeMessage("Safety", safety);
}

