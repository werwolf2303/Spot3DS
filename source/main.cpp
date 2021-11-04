#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <citro2d.h>
#include "audio.h"
#include "sockets.h"

int keyA() {
    printf("Dummy");
	return 9;
}
int keyB() {

}
Thread threadId;
OggOpusFile *opusFile;
int keyX() {
	 // Setup LightEvent for synchronisation of audioThread
    LightEvent_Init(&s_event, RESET_ONESHOT);
    // Open the Opus audio file
    int error = 0;
    opusFile = op_open_file(PATH, &error);
    if(error) {
        return 0;
    }

    // Attempt audioInit
    if(!audioInit()) {
        return 0;
    }

    // Set the ndsp sound frame callback which signals our audioThread
    ndspSetCallback(audioCallback, NULL);

    // Spawn audio thread

    // Set the thread priority to the main thread's priority ...
    int32_t priority = 0x30;
    svcGetThreadPriority(&priority, CUR_THREAD_HANDLE);
    // ... then subtract 1, as lower number => higher actual priority ...
    priority -= 1;
    // ... finally, clamp it between 0x18 and 0x3F to guarantee that it's valid.
    priority = priority < 0x18 ? 0x18 : priority;
    priority = priority > 0x3F ? 0x3F : priority;

    // Start the thread, passing our opusFile as an argument.
    threadId = threadCreate(audioThread, opusFile,
                                         THREAD_STACK_SZ, priority,
                                         THREAD_AFFINITY, false);
    return 0;
}
int stopPlaying() {
    LightEvent_Signal(&s_event);

    // Free the audio thread
    threadJoin(threadId, UINT64_MAX);
    threadFree(threadId);

    // Cleanup audio things and de-init platform features
}
int keyY() {
	stopPlaying();
}
int keySelect() {
	sendURL("spot3231");
}
int keyStart() {
}
bool isCitra() {
    //if(osGet3DSliderState()==NULL) {
    //    return true;
    //}else{
    //    return false;
    //}
	//Error
	return false;
}
int drawThings() {
    
}

int main(int argc, char* argv[])
{
    gfxInitDefault();
	romfsInit();
    ndspInit();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
    //Check if the application runs on real hardware
    if(isCitra) {
        printf("ERROR: this homebrew application is on citra not functional");
    }else{
        //Enable speed
        osSetSpeedupEnable(true); //When this fails it is an old 3ds
    }
    printf("\nPress A to create http server");
	printf("\nPress B to draw rectangle");

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if(kDown & KEY_START) {
			break; // Return to homebrew menu
        }
        if(kDown & KEY_SELECT) {
            keySelect();
        }
        if(kDown & KEY_A) {
            keyA();
        }
        if(kDown & KEY_B) {
            keyB();
        }
        if(kDown & KEY_X) {
            keyX();
        }
        if(kDown & KEY_Y) {
            keyY();
        }
	}
    romfsExit();
	gfxExit();
	return 0;
}
