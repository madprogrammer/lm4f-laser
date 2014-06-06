#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "driverlib/timer.h"
#include "driverlib/uart.h"
#include "driverlib/rom.h"
#include "usblib/usblib.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdbulk.h"
#include "laser_structs.h"
#include "dac.h"

// The system tick rate expressed both as ticks per second and a period in ms.
#define SYSTICKS_PER_SECOND		100
#define SYSTICK_PERIOD_MS		(1000 / SYSTICKS_PER_SECOND)

// Size of two-channel sample
#define	SAMPLE_SIZE				4

// The global system tick counter.
volatile unsigned long g_ulSysTickCount = 0;

// Variables tracking transmit and receive counts.
volatile unsigned long g_ulTxCount = 0;
volatile unsigned long g_ulRxCount = 0;
volatile unsigned long g_ulPointsCount = 1;

// Global flag indicating that a USB configuration has been set.
static volatile tBoolean g_bUSBConfigured = false;

// Interrupt handler for the system tick counter.
void
SysTickIntHandler(void)
{
	g_ulSysTickCount++;
}

typedef struct
{
	unsigned x:12;
	unsigned rx:4;
	unsigned y:12;
	unsigned ry:4;
} sample_t;

static unsigned long
ReceiveDataFromHost(tUSBDBulkDevice *psDevice, unsigned char *pcData,
				  unsigned long ulNumBytes)
{
	unsigned long ulLoop;
	unsigned long ulReadIndex;

	ulLoop = ulNumBytes / SAMPLE_SIZE;

	// Set up to process the characters by directly accessing the USB buffers.
	ulReadIndex = (unsigned long)(pcData - g_pucUSBRxBuffer);

	while(ulLoop)
	{
		sample_t* sample = (sample_t*)&g_pucUSBRxBuffer[ulReadIndex];
		dac_write(sample->x, sample->y);

		if (sample->rx)
			laser_pwr(0);
		else
			laser_pwr(1);

		SysCtlDelay(1000);
		g_ulPointsCount++;

		ulReadIndex += SAMPLE_SIZE;
		ulReadIndex = (ulReadIndex == BULK_BUFFER_SIZE) ? 0 : ulReadIndex;

		ulLoop--;
	}

	return(ulNumBytes);
}

unsigned long
TxHandler(void *pvCBData, unsigned long ulEvent, unsigned long ulMsgValue,
		  void *pvMsgData)
{
	if(ulEvent == USB_EVENT_TX_COMPLETE)
		g_ulTxCount += ulMsgValue;

	return(0);
}

unsigned long
RxHandler(void *pvCBData, unsigned long ulEvent,
			   unsigned long ulMsgValue, void *pvMsgData)
{
	switch(ulEvent)
	{
		case USB_EVENT_CONNECTED:
		{
			g_bUSBConfigured = true;

			USBBufferFlush(&g_sTxBuffer);
			USBBufferFlush(&g_sRxBuffer);
			break;
		}
		case USB_EVENT_DISCONNECTED:
		{
			g_bUSBConfigured = false;
			break;
		}
		case USB_EVENT_RX_AVAILABLE:
		{
			tUSBDBulkDevice *psDevice;
			g_ulRxCount += ulMsgValue;
			psDevice = (tUSBDBulkDevice *)pvCBData;
			return(ReceiveDataFromHost(psDevice, pvMsgData, ulMsgValue));
		}
		case USB_EVENT_SUSPEND:
		case USB_EVENT_RESUME:
			break;
		default:
			break;
	}

	return(0);
}

int
main(void)
{
	unsigned long ulTxCount;
	unsigned long ulRxCount;
	unsigned long ulPointsCount;

	ROM_FPULazyStackingEnable();

	// Set the clocking to run from the PLL at 50MHz
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_2_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	// Enable the GPIO port that is used for the on-board LED.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

	// Enable the GPIO pins for the LED (PF2 & PF3).
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, GPIO_PIN_3|GPIO_PIN_2|GPIO_PIN_0);
	GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_0, 0);

	// Configure PC4
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	ROM_GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_4, GPIO_STRENGTH_8MA, GPIO_PIN_TYPE_STD_WPD);
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_4);

	// Not configured initially.
	g_bUSBConfigured = false;

	// Enable the GPIO peripheral used for USB, and configure the USB pins.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_GPIOPinTypeUSBAnalog(GPIO_PORTD_BASE, GPIO_PIN_4 | GPIO_PIN_5);

	// Enable the system tick.
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / SYSTICKS_PER_SECOND);
	ROM_SysTickIntEnable();
	ROM_SysTickEnable();

	// Initialize the transmit and receive buffers.
	USBBufferInit((tUSBBuffer *)&g_sTxBuffer);
	USBBufferInit((tUSBBuffer *)&g_sRxBuffer);

	// Set the USB stack mode to Device mode with VBUS monitoring.
	USBStackModeSet(0, USB_MODE_FORCE_DEVICE, 0);

	// Pass our device information to the USB library and place the device on the bus.
	USBDBulkInit(0, (tUSBDBulkDevice *)&g_sBulkDevice);

	// Clear our local byte counters.
	ulRxCount = 0;
	ulTxCount = 0;

	// Initialize DAC
	dac_initialize();
	laser_pwr(1);

	// Main application loop.
	while(1)
	{
		/*
		// See if any data has been transferred.
		if((ulTxCount != g_ulTxCount) || (ulRxCount != g_ulRxCount))
		{
			// Has there been any transmit traffic since we last checked?
			if(ulTxCount != g_ulTxCount)
			{
				// Toggle the Green LED.
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
				SysCtlDelay(800);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);

				// Take a snapshot of the latest transmit count.
				ulTxCount = g_ulTxCount;
			}

			// Has there been any receive traffic since we last checked?
			if(ulRxCount != g_ulRxCount)
			{
				// Toggle the Blue LED.
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, GPIO_PIN_2);
				SysCtlDelay(800);
				GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_2, 0);

				// Take a snapshot of the latest receive count.
				ulRxCount = g_ulRxCount;
			}
		}*/

		if (ulPointsCount + 20000 < g_ulPointsCount) {
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, GPIO_PIN_3);
			SysCtlDelay(100000);
			GPIOPinWrite(GPIO_PORTF_BASE, GPIO_PIN_3, 0);
			ulPointsCount = g_ulPointsCount;
		}
	}
}
