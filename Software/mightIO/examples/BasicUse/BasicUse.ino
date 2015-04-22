/* 
Basic use example for the mightIO board. For more information, see
http://github.com/tszalay/mightIO

Written by Tamas Szalay
*/

#include <Wire.h>
#include "mightIO.h"

// (0,0) are the base I2C addresses for the ADC and DAC chips
// try changing these if nothing works
mightIO mio = mightIO(0,0);

void setup()
{
  // initialize serial interface for print()
  //Serial.begin(9600);
  // initialize i2c interface and set registers
  mio.begin();
}


void loop()
{
  int vals[4] = {0,0,0,0};
  
  vals[0] = 1000;     // write 1V to channel 0
  vals[1] = -3500;    // and -3.5V to channel 1
  
  mio.analogWrite(vals);
  
  // now read from all 4 channels
  mio.analogRead(vals);
}
