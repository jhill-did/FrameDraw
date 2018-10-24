#include <sys/mman.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <linux/fb.h>
#include <iostream>

#include <math.h>

uint32_t fillColor;
uint32_t strokeColor;
uint32_t clearColor;
struct fb_fix_screeninfo fixedInfo;
struct fb_var_screeninfo variableInfo;

inline uint32_t makeColoredPixel(uint8_t red, uint8_t green, uint8_t blue) {
	return (red << variableInfo.red.offset) | (green << variableInfo.green.offset) | (blue << variableInfo.blue.offset);
}

inline long getPixelLocation(long x, long y) {
	long location = (x + variableInfo.xoffset) * (variableInfo.bits_per_pixel / 8) + (y + variableInfo.yoffset) * fixedInfo.line_length;
}

void setPixelColor(uint8_t* frameBuffer, int x, int y, uint32_t color) {
	long location = getPixelLocation(x, y);
	*((uint32_t*)(frameBuffer + location)) = color;
}

bool inBounds(int x, int y) {
	return (x >= 0 || y >= 0 || x < variableInfo.xres || y < variableInfo.yres);
}

void fillRect(uint8_t* frameBuffer, long x, long y, int width, int height) {
	if (inBounds(x, y) == false || inBounds(x + width, y + height) == false) {
		// Out of bounds, do nothing.
		return;
	}
	
	for (int xPos = x; xPos < x + width; xPos += 1) {
		for (int yPos = y; yPos < y + height; yPos += 1) {
			setPixelColor(frameBuffer, xPos, yPos, fillColor);
		}
	}
}

template <typename T> inline constexpr
int sign(T value) {
	if (value > 0) return 1;
	if (value < 0) return -1;
	return 0;
}


void strokeLine(uint8_t* frameBuffer, int x1, int y1, int x2, int y2) {
	float rise = (float)y2 - (float)y1;
	float run = (float)x2 - (float)x1;

	// Special case to avoid division by zero draw a vertical line.
	if (run == 0) {
		for (int y = y1; y != y2; y += sign(rise)) {
			setPixelColor(frameBuffer, x1, y, strokeColor);
		}
		return;
	}	

  float slope = ((float)y2 - (float)y1) / ((float)x2 - (float)x1);

	// Determine which angle to draw from.
	if (slope <= 1.0f && slope >= -1.0f) {

		// Draw horizontal-ish line.
		int feedDirection = sign(run);
		for (int x = x1; x != x2; x += feedDirection) {
			float traversal = ((float)x - (float)x1) / (float)x2;
			int y = y1 + traversal * rise;
			setPixelColor(frameBuffer, x, y, strokeColor);	
		}
	} else {
		// Draw vertical-ish line.
		for (int y = y1; y != y2; y += sign(rise)) {
			float traversal = ((float)y - (float)y1) / (float)y2;
			int x = x1 + traversal * run;
			setPixelColor(frameBuffer, x, y, strokeColor);
		}
	}
}

void clearScreen(uint8_t* frameBuffer) {
	for (int x = 0; x < variableInfo.xres; x += 1) {
		for (int y = 0; y < variableInfo.yres; y += 1) {
			setPixelColor(frameBuffer, x, y, clearColor);
		}
	}
}

int main(int argc, char** argv) {
	int  frameBufferFile = open("/dev/fb0", O_RDWR);
	
	ioctl(frameBufferFile, FBIOGET_FSCREENINFO, &fixedInfo);

	// Get our initial variable info for our frame buffer.
	ioctl(frameBufferFile, FBIOGET_VSCREENINFO, &variableInfo);

	// Make sure our bits per pixel is something reasonable to do color with and write it to file.
	variableInfo.grayscale = 0;
	variableInfo.bits_per_pixel = 32;
	ioctl(frameBufferFile, FBIOPUT_VSCREENINFO, &variableInfo);

	// Pull it out to make sure our change stuck.
	ioctl(frameBufferFile, FBIOGET_VSCREENINFO, &variableInfo);

	long screenSize = variableInfo.yres_virtual * fixedInfo.line_length;
	
	// Initialize our framebuffer and default colors.
	uint8_t* frameBuffer = (uint8_t*)mmap(0, screenSize, PROT_READ | PROT_WRITE, MAP_SHARED, frameBufferFile, (off_t)0);
	clearColor = makeColoredPixel(0x00, 0x00, 0x00);
	fillColor = makeColoredPixel(0x00, 0x00, 0x00);
	strokeColor = makeColoredPixel(0xFF, 0xFF, 0x00);

	/*
	strokeLine(frameBuffer, 200, 200, 300, 150); // Right Up
	strokeLine(frameBuffer, 200, 200, 300, 250); // Right Down
	strokeLine(frameBuffer, 200, 200, 100, 150); // Left Up
	strokeLine(frameBuffer, 200, 200, 100, 250); // Left Down

	strokeColor = makeColoredPixel(0xFF, 0x00, 0xFF);
	strokeLine(frameBuffer, 200, 200, 200, 150);
	strokeLine(frameBuffer, 200, 200, 220, 250);

	strokeLine(frameBuffer, 500, 500, 100, 500);

	int startX = 500;
	int startY = 500;
	float radius = 100.0f;
	float angle = 30;
	while (true) {
		float radians = angle / 180.0f * M_PI;
		int endX = cosf(radians) * radius + startX;
		int endY = sinf(radians) * radius + startY;
		strokeLine(frameBuffer, startX, startY, endX, endY);
		angle += 0.001f;
		clearScreen(frameBuffer);
	}*/
	
	// Draw a color output demo
	/*
	int sampleCount = 25;
	int cellSize = 25;
	for (int x = 1; x < sampleCount; x += 1) {
		for (int y = 1; y < sampleCount; y += 1) {
			float hPercentage = (float)x / (float)sampleCount;
			float yPercentage = (float)y / (float)sampleCount;
			fillColor = makeColoredPixel(
				(uint8_t)((float)0xFF * (1.0f - hPercentage)),
				(uint8_t)((float)0xFF * hPercentage),
				(uint8_t)((float)0xFF * yPercentage)
			);
			int startX = 100 + (x * cellSize);
			int startY = 100 + (y * cellSize);
			fillRect(frameBuffer, startX, startY, cellSize, cellSize);
		}
	}
	*/

	for (int x = 0; x < variableInfo.xres; x += 1) {
		for (int y = 0; y < variableInfo.yres; y += 1) {
			float hPercentage = (float)x / (float)variableInfo.xres;
			float yPercentage = (float)y / (float)variableInfo.yres;
			fillColor = makeColoredPixel(
				(uint8_t)((float)0xFF * (1.0f - hPercentage)),
				(uint8_t)((float)0xFF * hPercentage),
				(uint8_t)((float)0xFF * yPercentage)
			);

			setPixelColor(frameBuffer, x, y, fillColor);
		}
	}
	
	return 0;
}
