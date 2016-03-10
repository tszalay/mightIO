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
  Serial.begin(9600);
  // initialize i2c interface and set registers
  mio.begin();

  // set some calibration offsets
  int dac_offs[4] = {95,23,0,0};
  mio.adjustOffsetDAC(dac_offs);
  int adc_offs[4] = {40,0,0,0};
  mio.adjustOffsetADC(adc_offs);
}

int val_square = 5000;
int input_0 = 0;

void loop()
{
  int vals[4] = {0,0,0,0};
  
  vals[0] = 1000;          // write 1V to channel 0
  vals[1] = -3500;         // and -3.5V to channel 1
  vals[2] = val_square;    // square wave to channel 2
  vals[3] = input_0;       // echo input 0 on channel 3
  
  mio.analogWrite(vals);

  delay(10);
  
  val_square = -val_square;
  
  // or directly write -8V to channel 3
  //mio.analogWrite(3,-8000);
  
  // how to read from all 4 channels
  //int vals_read[4];
  //mio.analogRead(vals_read);
  // vals_read is now whatever voltage is on AI0...AI3

  // or filtered, from channel 0
  input_0 = mio.analogReadFilter(0);

  Serial.print("Read values: ");
  Serial.println(input_0,DEC);
}
