#include "inc/hw_types.h"
#include "driverlib/usb.h"
#include "usblib/usblib.h"
#include "usblib/usb-ids.h"
#include "usblib/device/usbdevice.h"
#include "usblib/device/usbdbulk.h"
#include "laser_structs.h"

// The languages supported by this device.
const unsigned char g_pLangDescriptor[] =
{
    4,
    USB_DTYPE_STRING,
    USBShort(USB_LANG_EN_US)
};

// The manufacturer string.
const unsigned char g_pManufacturerString[] =
{
    (17 + 1) * 2,
    USB_DTYPE_STRING,
    'S', 0, 'e', 0, 'r', 0, 'g', 0, 'e', 0, 'y', 0, ' ', 0, 'A', 0, 'n', 0,
    'u', 0, 'f', 0, 'r', 0, 'i', 0, 'e', 0, 'n', 0, 'k', 0, 'o', 0,
};

// The product string.
const unsigned char g_pProductString[] =
{
    (16 + 1) * 2,
    USB_DTYPE_STRING,
    'L', 0, 'a', 0, 's', 0, 'e', 0, 'r', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0,
    't', 0, 'r', 0, 'o', 0, 'l', 0, 'l', 0, 'e', 0, 'r', 0,
};

// The serial number string.
const unsigned char g_pSerialNumberString[] =
{
    (8 + 1) * 2,
    USB_DTYPE_STRING,
    '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '0', 0, '1', 0
};

// The data interface description string.
const unsigned char g_pDataInterfaceString[] =
{
    (19 + 1) * 2,
    USB_DTYPE_STRING,
    'B', 0, 'u', 0, 'l', 0, 'k', 0, ' ', 0, 'D', 0, 'a', 0, 't', 0,
    'a', 0, ' ', 0, 'I', 0, 'n', 0, 't', 0, 'e', 0, 'r', 0, 'f', 0,
    'a', 0, 'c', 0, 'e', 0
};

// The configuration description string.
const unsigned char g_pConfigString[] =
{
    (23 + 1) * 2,
    USB_DTYPE_STRING,
    'B', 0, 'u', 0, 'l', 0, 'k', 0, ' ', 0, 'D', 0, 'a', 0, 't', 0,
    'a', 0, ' ', 0, 'C', 0, 'o', 0, 'n', 0, 'f', 0, 'i', 0, 'g', 0,
    'u', 0, 'r', 0, 'a', 0, 't', 0, 'i', 0, 'o', 0, 'n', 0
};

// The descriptor string table.
const unsigned char * const g_pStringDescriptors[] =
{
    g_pLangDescriptor,
    g_pManufacturerString,
    g_pProductString,
    g_pSerialNumberString,
    g_pDataInterfaceString,
    g_pConfigString
};

#define NUM_STRING_DESCRIPTORS (sizeof(g_pStringDescriptors) /                \
                                sizeof(unsigned char *))

tBulkInstance g_sBulkInstance;

extern const tUSBBuffer g_sTxBuffer;
extern const tUSBBuffer g_sRxBuffer;

const tUSBDBulkDevice g_sBulkDevice =
{
    USB_VID_STELLARIS,
    USB_PID_BULK,
    500,
    USB_CONF_ATTR_SELF_PWR,
    USBBufferEventCallback,
    (void *)&g_sRxBuffer,
    USBBufferEventCallback,
    (void *)&g_sTxBuffer,
    g_pStringDescriptors,
    NUM_STRING_DESCRIPTORS,
    &g_sBulkInstance
};

// Receive buffer (from the USB perspective).
unsigned char g_pucUSBRxBuffer[BULK_BUFFER_SIZE];
unsigned char g_pucRxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sRxBuffer =
{
    false,                           // This is a receive buffer.
    RxHandler,                       // pfnCallback
    (void *)&g_sBulkDevice,          // Callback data is our device pointer.
    USBDBulkPacketRead,              // pfnTransfer
    USBDBulkRxPacketAvailable,       // pfnAvailable
    (void *)&g_sBulkDevice,          // pvHandle
    g_pucUSBRxBuffer,                // pcBuffer
    BULK_BUFFER_SIZE,                // ulBufferSize
    g_pucRxBufferWorkspace           // pvWorkspace
};

// Transmit buffer (from the USB perspective).
unsigned char g_pucUSBTxBuffer[BULK_BUFFER_SIZE];
unsigned char g_pucTxBufferWorkspace[USB_BUFFER_WORKSPACE_SIZE];
const tUSBBuffer g_sTxBuffer =
{
    true,                            // This is a transmit buffer.
    TxHandler,                       // pfnCallback
    (void *)&g_sBulkDevice,          // Callback data is our device pointer.
    USBDBulkPacketWrite,             // pfnTransfer
    USBDBulkTxPacketAvailable,       // pfnAvailable
    (void *)&g_sBulkDevice,          // pvHandle
    g_pucUSBTxBuffer,                // pcBuffer
    BULK_BUFFER_SIZE,                // ulBufferSize
    g_pucTxBufferWorkspace           // pvWorkspace
};
