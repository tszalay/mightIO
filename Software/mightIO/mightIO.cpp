/*
  mightIO.cpp - Arduino library for mightIO boosted analog IO board
*/

#include "mightIO.h"


// Initialize class and device addresses from IDs
mightIO::mightIO(uint8_t dacID, uint8_t adcID) : adcLastConfig(0)
{
  this->dacAddr = (DAC_BASE_ADDR | dacID);
  this->adcAddr = (ADC_BASE_ADDR | adcID);
}

// Or without addresses, default to 0 for both
mightIO::mightIO() : mightIO(0,0)
{}


// Begin I2C and configure chips
void mightIO::begin()
{
  Wire.begin();
  this->dacSetGain();
  this->dacSetVref();
  this->adcSetup();
}

// Write all (four) values to the DAC
// Values should be specified in mV, -10240 to 10239
void mightIO::analogWrite(int16_t vout[])
{
  Wire.beginTransmission(this->dacAddr);
  // leading the first byte with 0b00 implicitly starts a 'fast write'
  for (uint8_t i=0; i<4; i++)
  {
	uint16_t val = 4095&((vout[i] + 10240)/5);
	// this is implicitly setting PD1,PD0 to 0 in upper bytes
	// which means normal operation
    Wire.send(highByte(val));
    Wire.send(lowByte(val));
  }
  Wire.endTransmission();
}

// Write a single value to the specified DAC channel (0...3)
void mightIO::analogWrite(uint8_t ch, int16_t vout)
{
  Wire.beginTransmission(this->dacAddr);
  uint16_t val = 4095&((vout + 10240)/5);
  
  // create a single-write command
  Wire.send(DAC_MULTIWRITE | (ch << 1));
  // and write the data, with Vref, PD, and gain set appropriately
  Write.send(0b10010000 | highByte(val));
  Wire.send(lowByte(val));

  Wire.endTransmission();
}


// Read a single ADC channel
int16_t mightIO::analogRead(uint8_t ch)
{
  this->adcConfig(ch, true);
  
  Wire.requestFrom(this->adcAddr,2);
  int16_t vin = (Wire.read() & 0xF) << 8;
  vin = vin | Wire.read();
  return 5*(vin-2048);
}

// Read ADC inputs in order, as many as specified
void mightIO::analogRead(uint16_t vin[], uint8_t nchan)
{
  // set to scan to nchan-1
  this->adcConfig(nchan-1, false);
  // and read all of the bytes
  Wire.requestFrom(this->adcAddr,2*nchan);
  for (uint8_t i=0; i<nchan; i++)
  {
	int16_t v = (Wire.read() & 0xF) << 8;
	v = v | Wire.read();
	vin[i] = 5*(v-2048);
  }
}
// or all of them, call with 4
void mightIO::analogRead(uint16_t vin[])
{
  this->analogRead(vin,4);
}



// Private function to set config byte, does nothing if already set
inline void mightIO::adcConfig(uint8_t ch, bool scanone)
{
  uint8_t cfg = ADC_CFGSINGLE | (ch << 1) | (scanone?ADC_CFGSCANONE:0);
  // if same as previous, don't need to send
  if (cfg == this->adcLastConfig)
    return;
  
  // else send and save
  Wire.beginTransmission(this->adcAddr);
  Wire.send(cfg);
  Wire.endTransmission();
  
  this->adcLastConfig = cfg;
}

// Private function - write ADC setup bytes
void mightIO::adcSetup()
{
  Wire.beginTransmission(this->adcAddr);
  Wire.send(ADC_SETUPBYTE | ADC_SETSEL);
  Wire.endTransmission();
}

// Private function - set to use internal 2.048V reference
void mightIO::dacSetVref()
{
  Wire.beginTransmission(this->dacAddr);
  Wire.send(DAC_VREFWRITE | 0b1111); 
  Wire.endTransmission();
}

// Private function - sets DAC gain to 2 (to go 0...4095)
void mightIO::dacSetGain()
{
  Wire.beginTransmission(this->dacAddr);
  Wire.send(DAC_GAINWRITE | 0b1111);
  //Wire.send(DAC_GAINWRITE | 0b0000); would set gain to 1
  Wire.endTransmission();
}