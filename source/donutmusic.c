#include <stdio.h>
#include <stdlib.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <aesndlib.h>
#include <gcmodplay.h>
#include <fat.h>

// include generated header
#include "donut_mod.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;
static MODPlay play;

//---------------------------------------------------------------------------------
int main(int argc, char **argv) {
//---------------------------------------------------------------------------------

	// Initialise the video system
	VIDEO_Init();

	// Initialise the attached controllers
	WPAD_Init();

	// Initialise the audio subsystem
	AESND_Init();

	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);

	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);

	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);

	// Make the display visible
	VIDEO_SetBlack(false);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// Initialise SD card

	if ( !fatInitDefault() ) {
		printf("Unable to initialise FAT filesystem, exiting");
		exit(0);
	}

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");

	MODPlay_Init(&play);
	MODPlay_SetMOD(&play,donut_mod);
	MODPlay_Start(&play);

	printf("Now playing: %s", (char *) &play.mod.name);

	printf("\nPress A to exit");

	while(1) {

		// Call WPAD_ScanPads each loop, this reads the latest controller states
		WPAD_ScanPads();

		// WPAD_ButtonsDown tells us which buttons were pressed in this loop
		// this is a "one shot" state which will not fire again until the button has been released
		u32 pressed = WPAD_ButtonsDown(0);

		if ( pressed & WPAD_BUTTON_RIGHT ) {
			printf("\e[1;1H\e[2J");
			printf("\x1b[2;0H");

			long mod_size;
			char* buffer;
			size_t result;

			FILE *f = fopen("/music/loop.mod", "rb");

			if (f == NULL) {
				fclose(f);
			} else {
				fseek(f, 0, SEEK_END);
				mod_size = ftell(f);
				rewind(f);
				// Allocate memory to contain the whole file:
				buffer = (char*) malloc(sizeof(char)*mod_size);
				if (buffer == NULL) {
					perror("Memory error\n");
				}
			}
			// Copy the file into the buffer:
			result = fread(buffer, 1, mod_size,f);
			if (result != mod_size) {
				perror("Reading error\n");
			}
			fclose(f);
			
			MODPlay_SetMOD(&play,buffer);
			MODPlay_Start(&play);

			printf("Now playing: %s\n", (char *) &play.mod.name);

			printf("Press A to exit");
		}

		// We return to the launcher application via exit
		if ( pressed & WPAD_BUTTON_A ) {
			MODPlay_Stop(&play);
			exit(0);
		}

		// Wait for the next frame
		VIDEO_WaitVSync();
	}

	return 0;
}
