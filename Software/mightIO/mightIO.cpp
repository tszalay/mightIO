/*
  mightIO.cpp - Arduino library for mightIO boosted analog IO board
*/

#include "mightIO.h"

#define VAL_SORT(a,b) { if ((a)>(b)) VAL_SWAP((a),(b)); }
#define VAL_SWAP(a,b) { int16_t temp=(a);(a)=(b);(b)=temp; }

int16_t opt_med5(int16_t * p)
{
    VAL_SORT(p[0],p[1]) ; VAL_SORT(p[3],p[4]) ; VAL_SORT(p[0],p[3]) ;
    VAL_SORT(p[1],p[4]) ; VAL_SORT(p[1],p[2]) ; VAL_SORT(p[2],p[3]) ;
    VAL_SORT(p[1],p[2]) ; return(p[2]) ;
}

int16_t opt_med7(int16_t * p)
{
    VAL_SORT(p[0], p[5]) ; VAL_SORT(p[0], p[3]) ; VAL_SORT(p[1], p[6]) ;
    VAL_SORT(p[2], p[4]) ; VAL_SORT(p[0], p[1]) ; VAL_SORT(p[3], p[5]) ;
    VAL_SORT(p[2], p[6]) ; VAL_SORT(p[2], p[3]) ; VAL_SORT(p[3], p[6]) ;
    VAL_SORT(p[4], p[5]) ; VAL_SORT(p[1], p[4]) ; VAL_SORT(p[1], p[3]) ;
    VAL_SORT(p[3], p[4]) ; return (p[3]) ;
}

// borrowed from the fantastic FastLED/lib8tion.h library (score MIT license!)
// note that this function operates only on unsigned ints! dealt with later in the code
uint16_t _scale16by8( int16_t i, uint8_t scale )
{
    uint16_t result = 0;
    asm volatile(
         // result.A = HighByte(i.A x j )
         "  mul %A[i], %[scale]                 \n\t"
         "  mov %A[result], r1                  \n\t"
         //"  clr %B[result]                      \n\t"

         // result.A-B += i.B x j
         "  mul %B[i], %[scale]                 \n\t"
         "  add %A[result], r0                  \n\t"
         "  adc %B[result], r1                  \n\t"

         // cleanup r1
         "  clr __zero_reg__                    \n\t"

         : [result] "+r" (result)
         : [i] "r" (i), [scale] "r" (scale)
         : "r0", "r1"
         );
    return result;
}

int16_t scale16by8(int16_t val, uint8_t scale)
{
    return (val>0) ? _scale16by8(val,scale) : -_scale16by8(-val,scale);
}

uint16_t scale16( uint16_t i, uint16_t scale )
{
    uint32_t result;
    asm volatile(
                 "  mul %A[i], %A[scale]                 \n\t"
                 "  movw %A[result], r0                   \n\t"
                 : [result] "=r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale)
                 : "r0", "r1"
                 );

    asm volatile(
                 "  mul %B[i], %B[scale]                 \n\t"
                 "  movw %C[result], r0                   \n\t"
                 : [result] "+r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale)
                 : "r0", "r1"
                 );

    const uint8_t  zero = 0;
    asm volatile(
                 "  mul %B[i], %A[scale]                 \n\t"

                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"

                 "  mul %A[i], %B[scale]                 \n\t"

                 "  add %B[result], r0                   \n\t"
                 "  adc %C[result], r1                   \n\t"
                 "  adc %D[result], %[zero]              \n\t"

                 "  clr r1                               \n\t"

                 : [result] "+r" (result)
                 : [i] "r" (i),
                   [scale] "r" (scale),
                   [zero] "r" (zero)
                 : "r0", "r1"
                 );

    result = result >> 16;
    return result;
}

// Initialize class and device addresses from IDs
mightIO::mightIO(uint8_t dacID, uint8_t adcID) : adcLastConfig(0)
{
  this->dacAddr = (DAC_BASE_ADDR | dacID);
  this->adcAddr = (ADC_BASE_ADDR | adcID);
}

// Or without addresses, default to 0 for both
mightIO::mightIO() : dacAddr(DAC_BASE_ADDR), adcAddr(ADC_BASE_ADDR)
{}


// Begin I2C and configure chips
void mightIO::begin()
{
  Wire.begin();
  this->dacSetVref();
  this->dacSetGain();
  this->adcSetup();
  this->defaultCalib();
}

// performance not super important here
void mightIO::defaultCalib()
{
  // DAC voltage divider
  uint8_t R1 = 22;
  uint8_t R2 = 100;
  
  // start it off at the Vref value, 2.5V
  uint32_t dac_off = 2500;
  // then do our handy calculation for offset (in mV)
  dac_off = (dac_off*R2)/R1;
  // and the scaling, *256 for the eventual division by 256 in scaling
  uint8_t dac_scl = (R1*256)/(R1+R2);
  
  for (int i=0; i<4; i++)
  {
    this->dac_scale[i] = dac_scl;
    this->dac_offset[i] = dac_off;
  }
  
  // now for the ADC
  R1 = 20;
  
  // vref is scaled down from 2.5V by resistors
  uint32_t adc_off = (2500*51)/61;
  // scale it by the resistors, assuming they divide nicely
  adc_off = (1+R2/R1)*adc_off;
  for (int i=0; i<4; i++)
  {
    this->adc_offset[i] = adc_off;
  }
}

// Write all (four) values to the DAC
// Values should be specified in mV, roughly -10000 to 10000
void mightIO::analogWrite(int16_t vout[])
{
  Wire.beginTransmission(this->dacAddr);
  // leading the first byte with 0b00 implicitly starts a 'fast write'
  for (uint8_t i=0; i<4; i++)
  {
    uint16_t val = 4095&scale16by8(vout[3-i]+this->dac_offset[i],this->dac_scale[i]);
	// this is implicitly setting PD1,PD0 to 0 in upper bytes
	// which means normal operation
    Wire.write(highByte(val));
    Wire.write(lowByte(val));
  }
  Wire.endTransmission();
}

// Write a single value to the specified DAC channel (0...3)
void mightIO::analogWrite(uint8_t ch, int16_t vout)
{
  Wire.beginTransmission(this->dacAddr);
  
  uint16_t val = 4095&scale16by8(vout+this->dac_offset[ch],this->dac_scale[ch]);
  
  // reverse channel
  ch = 3-ch;
  // create a single-write command
  Wire.write(DAC_MULTIWRITE | (ch << 1));
  // and write the data, with Vref, PD, and gain set appropriately
  Wire.write(0b10010000 | highByte(val));
  Wire.write(lowByte(val));

  Wire.endTransmission();
}


// Read a single ADC channel
int16_t mightIO::analogRead(uint8_t ch)
{
  this->adcConfig(ch, true);
  
  Wire.requestFrom(this->adcAddr, (uint8_t)2);
  int16_t vin = (Wire.read() & 0xF) << 8;
  vin = vin | Wire.read();
  return this->adc_offset[ch] - 5*vin;
}

// Read ADC inputs in order, as many as specified
void mightIO::analogRead(int16_t vin[], uint8_t nchan)
{
  // set to scan to nchan-1
  this->adcConfig(nchan-1, false);
  // and read all of the bytes
  Wire.requestFrom((int)this->adcAddr,2*nchan);
  for (uint8_t i=0; i<nchan; i++)
  {
	int16_t v = (Wire.read() & 0xF) << 8;
	v = v | Wire.read();
	vin[i] = this->adc_offset[i] - 5*v;
  }
}
// or all of them, call with 4
void mightIO::analogRead(int16_t vin[])
{
  this->analogRead(vin,4);
}

// and some filtered ones
int16_t mightIO::analogReadFilter(uint8_t ch)
{
	int16_t vals[7];
	
	for (int i=0; i<7; i++)
		vals[i] = analogRead(ch);
		
	return opt_med7(vals);
}

//calibration functions
void mightIO::getCalibrationADC(uint8_t scale[], uint16_t offset[])
{
  memcpy(scale,this->adc_scale,4);
  memcpy(offset,this->adc_offset,8);
}
void mightIO::getCalibrationDAC(uint8_t scale[], uint16_t offset[])
{
  memcpy(scale,this->dac_scale,4);
  memcpy(offset,this->dac_offset,8);
}
void mightIO::setCalibrationADC(uint8_t scale[], uint16_t offset[])
{
  memcpy(this->adc_scale,scale,4);
  memcpy(this->adc_offset,offset,8);
}
void mightIO::setCalibrationDAC(uint8_t scale[], uint16_t offset[])
{
  memcpy(this->dac_scale,scale,4);
  memcpy(this->dac_offset,offset,8);
}

void mightIO::adjustCalibrationADC(int scale[], int offset[])
{
  for (int i=0; i<4; i++)
  {
    this->adc_scale[i] += scale[i];
    this->adc_offset[i] += offset[i];
  }
}

void mightIO::adjustCalibrationDAC(int scale[], int offset[])
{
  for (int i=0; i<4; i++)
  {
    this->dac_scale[i] += scale[i];
    this->dac_offset[i] += offset[i];
  }
}

// Private function to set config byte, does nothing if already set
inline void mightIO::adcConfig(uint8_t ch, bool scanone)
{
  uint8_t cfg = ADC_CFGSINGLE | (ch << 1) | (scanone?ADC_CFGSCANONE:0);
  // if same as previous, don't need to write
  if (cfg == this->adcLastConfig)
    return;
  
  // else write and save
  Wire.beginTransmission(this->adcAddr);
  Wire.write(cfg);
  Wire.endTransmission();
  
  this->adcLastConfig = cfg;
}

// Private function - write ADC setup bytes
void mightIO::adcSetup()
{
  Wire.beginTransmission(this->adcAddr);
  Wire.write(ADC_SETUPBYTE | ADC_SETSEL);
  Wire.endTransmission();
}

// Private function - set to use internal 2.048V reference
void mightIO::dacSetVref()
{
  Wire.beginTransmission(this->dacAddr);
  Wire.write(DAC_VREFWRITE | 0b1111);
  Wire.endTransmission();
}

// Private function - sets DAC gain to 2 (to go 0...4095)
void mightIO::dacSetGain()
{
  Wire.beginTransmission(this->dacAddr);
  Wire.write(DAC_GAINWRITE | 0b1111);
  //Wire.write(DAC_GAINWRITE | 0b0000); would set gain to 1
  Wire.endTransmission();
}