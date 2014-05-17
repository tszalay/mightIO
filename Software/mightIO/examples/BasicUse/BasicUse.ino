/* 
Basic use of Arduino library for MicroChip MCP4728 I2C D/A converter
For discussion and feedback, please go to http://arduino.cc/forum/index.php/topic,51842.0.html
*/

#include <Wire.h>
#include "mightIO.h"

mightIO mio = mightIO(1,0);


void setup()
{
  //Serial.begin(9600);  // initialize serial interface for print()
  mio.begin();  // initialize i2c interface
}


void loop()
{
  
}
