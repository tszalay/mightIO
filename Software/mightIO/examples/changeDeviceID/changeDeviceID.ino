/*
  Writing programmable device id (0-7)
    Because of critical timing of LDAC latch during the address write and read, 
    the library use software I2C master library just for address read and writing.
    Included modified SoftI2cMaster library is required.
    Original library is from fat16lib , http://forums.adafruit.com/viewtopic.php?f=25&t=13722

  I2C CLOCK pin (SCL) need to connected to A5.
  I2C DATA pin (SDA) need to connected to A4.
  LDAC pin need to be connected to digital pin 8.

  This skech will check LDAC, I2C status first to check any possible problem.
  Just follow the direction on serial minitor
*/

/**********************************************
Change newDeviceID to new device ID you want to
***********************************************/
#define newDeviceID 0
//**********************************************


#include "SoftI2cMaster.h"
/*
  Change the pin, if you connect them other pins
*/
#define SCL_PIN 19 // A5
#define SDA_PIN 18 //A4
#define LDACpin 8


SoftI2cMaster i2c;

void setup()
{
  Serial.begin(9600);  // initialize serial interface for readAddress()
  guide();
  
  delay(5000);
  
  i2c.init(SCL_PIN, SDA_PIN); // initialize i2c interface
  
  checkLDAC();
  Serial.println("");
  delay(1000);
  
  pinMode(LDACpin, OUTPUT); // Set LDACpin to OUTPUT
  
  Serial.println("Now checking current device ID of mcp4728...");
  int current_address = readAddress(); // Read current Device ID 
  int scannedID = (current_address & 0B00001110) >> 1;
  Serial.print("Current device ID of MCP4728 = ");
  Serial.println(scannedID,DEC);
  Serial.println("");
  delay(1000);
  
  Serial.println("Checking communication to mcp4728...");
  int checki2c = i2c.start(current_address);
  i2c.stop();
  if (checki2c == 0){
    Serial.print("Sucessfully communicate with mcp4728");
  }
  else{
    Serial.print("Problem found on I2C with mcp4728");
  }
    
  Serial.println(checki2c,DEC);
  Serial.println("");
  delay(1000);
  
  Serial.println("Writing new device ID to mcp4728...");
  writeAddress(scannedID, newDeviceID);
  delay(100); // wait for EEPROM writing 
  Serial.println("");
  delay(1000);
  
  Serial.println("Checking changed device ID of mcp4728...");
  current_address = readAddress();
  scannedID = (current_address & 0B00001110) >> 1;
  if (newDeviceID == scannedID){
    Serial.print("Device ID of mcp4728 is sucessfully change to ");
    Serial.println(scannedID,DEC);
    Serial.print("Now for normal operation, please change back LDAC jumper from + to - ");
  }
  else{
    Serial.println("Device ID of mcp4728 is not matched with current ID");
    Serial.println("Please, check instruction and try again");
    Serial.print(scannedID,DEC);
  }
} 

void loop()
{
} 

void guide(){
  Serial.println("Changing Device ID of MCP2728 for multiple shield stacking");
  Serial.println("");
  Serial.println("Check following carefully !");
  Serial.println("  1.Just one RGB LED shield connected to Arduino");
  Serial.println("  2.Make sure A4, A5, GND pins are connected to Arduino");
  Serial.println("  3.Make sure LDAC jumpuer on the shield is changed from - to +");
  Serial.println("");
}

boolean checkLDAC(){
  boolean check_floating = false;
  int check2, check3;
  for(int i=0;i<10;i++){
    pinMode(LDACpin, OUTPUT);
    if(i<5) digitalWrite(LDACpin, LOW);
    if(i>=5) digitalWrite(LDACpin, HIGH);
    pinMode(LDACpin, INPUT); 
    check2 = digitalRead(LDACpin);
    if (i>0 && (check3 != check2)){
      check_floating = true;
    }
    check3 = check2;
  }
  if (check_floating == 0) {
    Serial.println("LDAC pin seem to be connected");
  }
  else {
    Serial.println("LDAC pin is not connected, Check LDAC jumper!");
  }
  return check_floating;
}

uint8_t readAddress() {
  digitalWrite(LDACpin, HIGH);
  i2c.start(0B00000000);
  i2c.ldacwrite(0B00001100, LDACpin); // modified command for LDAC pin latch
  i2c.restart(0B11000001);
  return i2c.read(false);
  i2c.stop();
}

void writeAddress(int oldAddress, int newAddress) {
  digitalWrite(LDACpin, HIGH);
  i2c.start(0B11000000 | (oldAddress << 1));
  i2c.ldacwrite(0B01100001 | (oldAddress << 2), LDACpin); // modified command for LDAC pin latch
  i2c.write(0B01100010 | (newAddress << 2));
  i2c.write(0B01100011 | (newAddress << 2));
  i2c.stop();
}

