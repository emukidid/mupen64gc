#include "InputManager.h"

namespace menu {

Input::Input()
{
	PAD_Init();
	CONF_Init();
	WPAD_Init();
	WPAD_SetIdleTimeout(30);
	WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);
	WPAD_SetDataFormat(WPAD_CHAN_ALL, WPAD_FMT_BTNS_ACC_IR); 
//	VIDEO_SetPostRetraceCallback (PAD_ScanPads);
}

Input::~Input()
{
}

void Input::refreshInput()
{
	PAD_ScanPads();
	PAD_Read(gcPad);
	PAD_Clamp(gcPad);
	for (int i = WPAD_CHAN_0; i < WPAD_MAX_WIIMOTES; i++)
		wiiPadError[i] = WPAD_ReadEvent(i, &wiiPad[i]);
}

WPADData* Input::getWpad()
{
	return wiiPad;
}

PADStatus* Input::getPad()
{
	return gcPad;
}

} //namespace menu 
