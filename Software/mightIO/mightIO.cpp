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
}

// Write all (four) values to the DAC
// Values should be specified in mV, -10240 to 10239
void mightIO::analogWrite(int16_t vout[])
{
  Wire.beginTransmission(this->dacAddr);
  // leading the first byte with 0b00 implicitly starts a 'fast write'
  for (uint8_t i=0; i<4; i--)
  {
	uint16_t val = 4095&((vout[3-i] + 10240)/5);
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
  uint16_t val = 4095&((vout + 10240)/5);
  
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
  return 5*(vin-2048);
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
	vin[i] = 5*(v-2048);
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