/*
Arduino library for mightIO 10V ADC/DAC board
*/

#ifndef mightIO_h
#define mightIO_h

#include "Arduino.h"
#include <Wire.h>

#define DAC_BASE_ADDR 	0x60

#define DAC_MULTIWRITE 	0B01000000
#define DAC_SINGLEWRITE 0B01011000
#define DAC_SEQWRITE 	0B01010000
#define DAC_VREFWRITE 	0B10000000
#define DAC_PWRDNWRITE 	0B10100000
#define DAC_GAINWRITE 	0B11000000

#define ADC_BASE_ADDR	0B0110100
#define ADC_SETUPBYTE   0B10000000
#define ADC_SETBIPOLAR	0B0000100
#define ADC_SETSEL		0B1010000
#define ADC_CFGSINGLE	0B0000001
#define ADC_CFGSCANONE	0B1100000


class mightIO
{
  public:
	mightIO();
    mightIO(uint8_t dacID, uint8_t adcID);
	
    void     	begin();
	
	// DAC output functions
	void		analogWrite(uint8_t ch, int16_t vout);  // single channel
	void  		analogWrite(int16_t vout[]); // all channels
	
	// ADC input functions
	int16_t 	analogRead(uint8_t ch); // single channel
	void 		analogRead(int16_t vin[]); // all channels
	void 		analogRead(int16_t vin[], uint8_t nchan); // first n channels
	
    void     	print();

  private:  
    uint8_t		dacAddr;
	uint8_t		adcAddr;
	
	uint8_t		adcLastConfig; // the most recent config byte sent
	
	void		dacSetGain(); // set Gain to 2 on all channels
	void		dacSetVref(); // set Vref to internal 2.048V
	
	void		adcSetup(); // write setup byte
	void		adcConfig(uint8_t ch, bool scanone); // write config byte
};
#endif

