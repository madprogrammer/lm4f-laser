#ifndef __DAC__H__
#define __DAC__H__

void dac_initialize();
void dac_write(unsigned short x, unsigned short y);
void laser_pwr(unsigned char on);

#endif
