#include <3ds.h>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <cstdio>
u32 __stacksize__ =0x40000;


int main()
{
	gfxInitDefault();
	menu();
	consoleSelect(&top);
	hidInit();
	Result ret;
	httpcInit(0x1000);
	while (aptMainLoop()) {

		hidScanInput();
		hidTouchRead(&touch);

        u32 kDown=hidKeysDown();
		if (kDown & KEY_START) {
			break;
		}
		gfxFlushBuffers();
		gspWaitForVBlank();
		gfxSwapBuffers();
	}
httpcExit();	
hidExit();
gfxExit();
return 0;
}