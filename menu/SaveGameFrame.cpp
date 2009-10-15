#include "MenuContext.h"
#include "SaveGameFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"


extern "C" {
#include "../gc_memory/Saves.h"
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-CARD.h"
#ifdef WII
#include "../fileBrowser/fileBrowser-WiiFS.h"
#endif
}

void Func_SaveGameCardA();
void Func_SaveGameCardB();
void Func_SaveGameSD();
void Func_SaveGameWiiFS();
void Func_ReturnFromSaveGameFrame();

#define NUM_FRAME_BUTTONS 4
#define FRAME_BUTTONS saveGameFrameButtons
#define FRAME_STRINGS saveGameFrameStrings

static char FRAME_STRINGS[4][22] =
	{ "Memory Card in Slot A",
	  "Memory Card in Slot B",
	  "SD Card",
	  "Wii Filesystem"};

struct ButtonInfo
{
	menu::Button	*button;
	char*			buttonString;
	float			x;
	float			y;
	float			width;
	float			height;
	int				focusUp;
	int				focusDown;
	int				focusLeft;
	int				focusRight;
	ButtonFunc		clickedFunc;
	ButtonFunc		returnFunc;
} FRAME_BUTTONS[NUM_FRAME_BUTTONS] =
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc			returnFunc
	{	NULL,	FRAME_STRINGS[0],	150.0,	 50.0,	340.0,	40.0,	 3,	 1,	-1,	-1,	Func_SaveGameCardA,	Func_ReturnFromSaveGameFrame }, // Save To Card A
	{	NULL,	FRAME_STRINGS[1],	150.0,	150.0,	340.0,	40.0,	 0,	 2,	-1,	-1,	Func_SaveGameCardB,	Func_ReturnFromSaveGameFrame }, // Save To Card B
	{	NULL,	FRAME_STRINGS[2],	150.0,	250.0,	340.0,	40.0,	 1,	 3,	-1,	-1,	Func_SaveGameSD,	Func_ReturnFromSaveGameFrame }, // Save To SD
	{	NULL,	FRAME_STRINGS[3],	150.0,	350.0,	340.0,	40.0,	 2,	 0,	-1,	-1,	Func_SaveGameWiiFS,	Func_ReturnFromSaveGameFrame }, // Save To Wii FS
};

SaveGameFrame::SaveGameFrame()
{
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(buttonImage, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		FRAME_BUTTONS[i].button->setFocusImage(buttonFocusImage);
		if (FRAME_BUTTONS[i].focusUp != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[i].focusUp].button);
		if (FRAME_BUTTONS[i].focusDown != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[i].focusDown].button);
		if (FRAME_BUTTONS[i].focusLeft != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusLeft].button);
		if (FRAME_BUTTONS[i].focusRight != -1) FRAME_BUTTONS[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, FRAME_BUTTONS[FRAME_BUTTONS[i].focusRight].button);
		FRAME_BUTTONS[i].button->setActive(true);
		if (FRAME_BUTTONS[i].clickedFunc) FRAME_BUTTONS[i].button->setClicked(FRAME_BUTTONS[i].clickedFunc);
		if (FRAME_BUTTONS[i].returnFunc) FRAME_BUTTONS[i].button->setReturn(FRAME_BUTTONS[i].returnFunc);
		add(FRAME_BUTTONS[i].button);
		menu::Cursor::getInstance().addComponent(this, FRAME_BUTTONS[i].button, FRAME_BUTTONS[i].x, 
												FRAME_BUTTONS[i].x+FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].y, 
												FRAME_BUTTONS[i].y+FRAME_BUTTONS[i].height);
	}
	setDefaultFocus(FRAME_BUTTONS[0].button);
	setEnabled(true);

}

SaveGameFrame::~SaveGameFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

void Func_SaveGameCardA()
{
	// Adjust saveFile pointers
//	saveFile_dir = (item_num%2) ? &saveDir_CARD_SlotB : &saveDir_CARD_SlotA;
	saveFile_dir       = &saveDir_CARD_SlotA;
	saveFile_readFile  = fileBrowser_CARD_readFile;
	saveFile_writeFile = fileBrowser_CARD_writeFile;
	saveFile_init      = fileBrowser_CARD_init;
	saveFile_deinit    = fileBrowser_CARD_deinit;
		
	// Try loading everything
	int result = 0;
	saveFile_init(saveFile_dir);
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);
		
//	return result ? "Saved game to memcard" : "Nothing to save";
}

void Func_SaveGameCardB()
{
	// Adjust saveFile pointers
//	saveFile_dir = (item_num%2) ? &saveDir_CARD_SlotB : &saveDir_CARD_SlotA;
	saveFile_dir       = &saveDir_CARD_SlotB;
	saveFile_readFile  = fileBrowser_CARD_readFile;
	saveFile_writeFile = fileBrowser_CARD_writeFile;
	saveFile_init      = fileBrowser_CARD_init;
	saveFile_deinit    = fileBrowser_CARD_deinit;
		
	// Try loading everything
	int result = 0;
	saveFile_init(saveFile_dir);
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);
		
//	return result ? "Saved game to memcard" : "Nothing to save";
}

void Func_SaveGameSD()
{
	// Adjust saveFile pointers
	saveFile_dir = &saveDir_libfat_Default;
	saveFile_readFile  = fileBrowser_libfat_readFile;
	saveFile_writeFile = fileBrowser_libfat_writeFile;
	saveFile_init      = fileBrowser_libfat_init;
	saveFile_deinit    = fileBrowser_libfat_deinit;
		
	// Try loading everything
	int result = 0;
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
		
//	return result ? "Saved game to SD card" : "Nothing to save";
}

void Func_SaveGameWiiFS()
{
	// Adjust saveFile pointers
	saveFile_dir       = &saveDir_WiiFS;
	saveFile_readFile  = fileBrowser_WiiFS_readFile;
	saveFile_writeFile = fileBrowser_WiiFS_writeFile;
	saveFile_init      = fileBrowser_WiiFS_init;
	saveFile_deinit    = fileBrowser_WiiFS_deinit;
		
	// Try loading everything
	int result = 0;
	saveFile_init(saveFile_dir);
	result += saveEeprom(saveFile_dir);
	result += saveSram(saveFile_dir);
	result += saveMempak(saveFile_dir);
	result += saveFlashram(saveFile_dir);
	saveFile_deinit(saveFile_dir);
		
//	return result ? "Saved game to filesystem" : "Nothing to save";
}

extern MenuContext *pMenuContext;

void Func_ReturnFromSaveGameFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}
