#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <libusb-1.0/libusb.h>
#include <iostream>
#include <string>
#include <vector>

#define MAGIC 0x41444C49

static inline uint16_t swapshort(uint16_t v) {
	return (v >> 8) | (v << 8);
}

float scale = 1.0;

typedef struct {
	uint32_t	magic;
	uint8_t		pad1[3];
	uint8_t		format;
	char		name[8];
	char		company[8];
	uint16_t 	count;
	uint16_t 	frameno;
	uint16_t 	framecount;
	uint8_t		scanner;
	uint8_t 	pad2;
} __attribute__((packed)) ilda_hdr;

#define BLANK 0x40
#define LAST  0x80

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t state;
	uint8_t color;
} __attribute__((packed)) icoord3d;

typedef struct coord3d {
	int16_t x;
	int16_t y;
	int16_t z;
	uint8_t state;

	coord3d(int16_t x, int16_t y, int16_t z, uint8_t state) : x(x), y(y), z(z), state(state) { }
} coord3d;

typedef struct {
	std::vector<coord3d> points;
	int position;
} frame;

frame rframe;
int subpos;
int divider = 1;

int loadildahdr(FILE *ild, ilda_hdr & hdr)
{
	if (fread(&hdr, sizeof(hdr), 1, ild) != 1) {
		std::cerr << "Error while reading header" << std::endl;
		return -1;
	}

	if (hdr.magic != MAGIC) {
		std::cerr << "Invalid magic" << std::endl;
		return -1;
	}

	if (hdr.format != 0) {
		fprintf(stderr, "Unsupported section type %d\n", hdr.format);
		return -1;
	}

	hdr.count = swapshort(hdr.count);
	hdr.frameno = swapshort(hdr.frameno);
	hdr.framecount = swapshort(hdr.framecount);
}

int loadild(const std::string & file, frame & frame)
{
	int i;
	FILE *ild = fopen(file.c_str(), "rb");

	if (!ild) {
		std::cerr << "Cannot open " << file << std::endl;
		return -1;
	}

	ilda_hdr hdr;
	loadildahdr(ild, hdr);

	for (int f = 0; f < hdr.framecount; f++)
	{
		std::cout << "Frame " << hdr.frameno << " of " << hdr.framecount << " " << hdr.count << " points" << std::endl;
		icoord3d *tmp = (icoord3d*)calloc(hdr.count, sizeof(icoord3d));

		if (fread(tmp, sizeof(icoord3d), hdr.count, ild) != hdr.count) {
			std::cerr << "Error while reading frame" << std::endl;
			return -1;
		}

		for(i = 0; i < hdr.count; i++) {
			coord3d point(swapshort(tmp[i].x), swapshort(tmp[i].y), swapshort(tmp[i].z), tmp[i].state);
			frame.points.push_back(point);
		}

		free(tmp);

		loadildahdr(ild, hdr);
	}

	fclose(ild);
	return 0;
}

short outBuffer[128];

int process()
{
	frame *frame = &rframe;

	short *sx = &outBuffer[0];
	short *sy = &outBuffer[1];

	for (int frm = 0; frm < 64; frm++) {
		struct coord3d *c = &frame->points[frame->position];

		*sx = 4095 - (2047 + (2048 * c->x / 32768)) * scale;
		*sy = (2047 + (2048 * c->y / 32768)) * scale;

		if(c->state & BLANK) {
			*sx |= 1 << 15;
		} else {
			*sx &= ~(1 << 15);
		}

		sx += 2;
		sy += 2;

		subpos++;
		if (subpos == divider) {
			subpos = 0;
			if (c->state & LAST)
				frame->position = 0;
			else
				frame->position = (frame->position + 1) % frame->points.size();
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	libusb_device_handle *dev;
	libusb_context *ctx = NULL;
	int ret, actual;

	ret = libusb_init(&ctx);
	if(ret < 0) {
		fprintf(stderr,"Couldn't initialize libusb\n");
		return EXIT_FAILURE;
	}

	libusb_set_debug(ctx, 3);

	dev = libusb_open_device_with_vid_pid(ctx, 0x1cbe, 0x0003);
	if(dev == NULL) {
		fprintf(stderr, "Cannot open device\n");
		return EXIT_FAILURE;
	}
	else
		printf("Device opened\n");

	if(libusb_kernel_driver_active(dev, 0) == 1) {
		fprintf(stderr, "Kernel driver active\n");
		libusb_detach_kernel_driver(dev, 0);
	}

	ret = libusb_claim_interface(dev, 0);
	if(ret < 0) {
		fprintf(stderr, "Couldn't claim interface\n");
		return EXIT_FAILURE;
	}

	// To maintain our sample rate
	struct timespec ts;
	ts.tv_sec  = 0;
	ts.tv_nsec = 2000000;

	memset(&rframe, 0, sizeof(frame));
	if (loadild(argv[1], rframe) < 0)
	{
		fprintf(stderr, "Failed to load ILDA\n");
		return EXIT_FAILURE;
	}

	while(1)
	{
		process();

		if(nanosleep(&ts, NULL) != 0)
			fprintf(stderr, "Nanosleep failed");
		ret = libusb_bulk_transfer(dev, (1 | LIBUSB_ENDPOINT_OUT), (unsigned char*)&outBuffer, 256, &actual, 0);
		if(ret != 0 || actual != 256)
			fprintf(stderr, "Write error\n");
	}

	libusb_release_interface(dev, 0);
	libusb_close(dev);
	libusb_exit(ctx);

	return 0;
}
