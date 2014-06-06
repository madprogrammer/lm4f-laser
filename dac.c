#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/rom.h"

void laser_pwr(unsigned char on)
{
	if (on)
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_PIN_4);
	else
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_4, 0);
}

void dac_initialize()
{
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_1);

	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1); // SYNC
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_PIN_3); // LDAC
}

void dac_write_byte(unsigned char byte)
{
	for(unsigned char n = 0; n < 8; n++)
	{
		if(byte & 0x80)
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, GPIO_PIN_1); // SDIN
		else
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_1, 0);
		byte <<= 1;

		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, GPIO_PIN_2); // SCK
		GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_2, 0);
	}
}

void dac_write(unsigned short x, unsigned short y)
{
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0); // SYNC
	x &= ~(1 << 12);

	dac_write_byte(x >> 8);   // High byte
	dac_write_byte(x & 0xFF); // Low byte

	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, 0);

	y |= 1 << 12;
	dac_write_byte(y >> 8);   // High byte
	dac_write_byte(y & 0xFF); // Low byte

	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_PIN_1);
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, 0);
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_3, GPIO_PIN_3);
}
