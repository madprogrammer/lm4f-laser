CFLAGS=-g -mthumb \
			-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 \
			-mfloat-abi=softfp -O2 -ffunction-sections \
			-fdata-sections -MD -std=c99 -Wall -pedantic \
			-DPART_LM4F120H5QR -Dgcc -DTARGET_IS_BLIZZARD_RA1 -c \
			-I$(HOME)/source/stellarisware \
			-I$(HOME)/source/stellarisware/usblib

LDFLAGS=-T $(TARGET).ld --entry ResetISR --gc-sections
TARGET=laser

all:
	arm-none-eabi-gcc $(CFLAGS) \
		startup_gcc.c \
		laser_structs.c \
		main.c \
		dac.c
	arm-none-eabi-ld $(LDFLAGS) -o a.out \
		startup_gcc.o \
		laser_structs.o \
		main.o \
		dac.o \
		~/source/stellarisware/usblib/gcc-cm4f/libusb-cm4f.a \
		~/source/stellarisware/driverlib/gcc-cm4f/libdriver-cm4f.a
	arm-none-eabi-objcopy -O binary a.out $(TARGET).bin

flash:
	lm4flash $(TARGET).bin

clean:
	rm -f a.out
	rm -f *.d *.o
