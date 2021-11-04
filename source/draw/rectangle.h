#include <citro2d.h>
#include <citro3d.h>
#include <3ds.h>
bool top = false;

int draw() {
    if(top) {
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0x68);
	u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	u32 clrBlue  = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
	C2D_TargetClear(top,clrBlue);
	C2D_SceneBegin(top);  
	C2D_DrawRectangle(400 - 50, 0, 0, 50, 50, clrWhite, clrWhite, clrWhite, clrWhite);
	C3D_FrameEnd(0);
    }else{
    C3D_RenderTarget* top = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	u32 clrClear = C2D_Color32(0xFF, 0xD8, 0xB0, 0x68);
	u32 clrWhite = C2D_Color32(0xFF, 0xFF, 0xFF, 0xFF);
	u32 clrBlue  = C2D_Color32(0x00, 0x00, 0xFF, 0xFF);
	C2D_TargetClear(top,clrBlue);
	C2D_SceneBegin(top);  
	C2D_DrawRectangle(400 - 50, 0, 0, 50, 50, clrWhite, clrWhite, clrWhite, clrWhite);
	C3D_FrameEnd(0);
    }
}