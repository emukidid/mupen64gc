#include <math.h>
#include "MenuContext.h"
#include "FileBrowserFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"

extern "C" {
#include "../fileBrowser/fileBrowser.h"
#include "../fileBrowser/fileBrowser-libfat.h"
#include "../fileBrowser/fileBrowser-DVD.h"
#include "../fileBrowser/fileBrowser-CARD.h"
}

void Func_PrevPage();
void Func_NextPage();
void Func_ReturnFromFileBrowserFrame();
void Func_Select1();
void Func_Select2();
void Func_Select3();
void Func_Select4();
void Func_Select5();
void Func_Select6();
void Func_Select7();
void Func_Select8();
void Func_Select9();
void Func_Select10();

#define NUM_FRAME_BUTTONS 12
#define NUM_FILE_SLOTS 10
#define FRAME_BUTTONS fileBrowserFrameButtons
#define FRAME_STRINGS fileBrowserFrameStrings

static char FRAME_STRINGS[3][5] =
	{ "Prev",
	  "Next",
	  ""};


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
{ //	button	buttonString		x		y		width	height	Up	Dwn	Lft	Rt	clickFunc		returnFunc
	{	NULL,	FRAME_STRINGS[0],	 35.0,	220.0,	 70.0,	40.0,	-1,	-1,	-1,	 2,	Func_PrevPage,	Func_ReturnFromFileBrowserFrame }, // Prev
	{	NULL,	FRAME_STRINGS[1],	535.0,	220.0,	 70.0,	40.0,	-1,	-1,	 2,	-1,	Func_NextPage,	Func_ReturnFromFileBrowserFrame }, // Next
	{	NULL,	FRAME_STRINGS[2],	120.0,	 40.0,	400.0,	35.0,	11,	 3,	 0,	 1,	Func_Select1,	Func_ReturnFromFileBrowserFrame }, // File Button 1
	{	NULL,	FRAME_STRINGS[2],	120.0,	 80.0,	400.0,	35.0,	 2,	 4,	 0,	 1,	Func_Select2,	Func_ReturnFromFileBrowserFrame }, // File Button 2
	{	NULL,	FRAME_STRINGS[2],	120.0,	120.0,	400.0,	35.0,	 3,	 5,	 0,	 1,	Func_Select3,	Func_ReturnFromFileBrowserFrame }, // File Button 3
	{	NULL,	FRAME_STRINGS[2],	120.0,	160.0,	400.0,	35.0,	 4,	 6,	 0,	 1,	Func_Select4,	Func_ReturnFromFileBrowserFrame }, // File Button 4
	{	NULL,	FRAME_STRINGS[2],	120.0,	200.0,	400.0,	35.0,	 5,	 7,	 0,	 1,	Func_Select5,	Func_ReturnFromFileBrowserFrame }, // File Button 5
	{	NULL,	FRAME_STRINGS[2],	120.0,	240.0,	400.0,	35.0,	 6,	 8,	 0,	 1,	Func_Select6,	Func_ReturnFromFileBrowserFrame }, // File Button 6
	{	NULL,	FRAME_STRINGS[2],	120.0,	280.0,	400.0,	35.0,	 7,	 9,	 0,	 1,	Func_Select7,	Func_ReturnFromFileBrowserFrame }, // File Button 7
	{	NULL,	FRAME_STRINGS[2],	120.0,	320.0,	400.0,	35.0,	 8,	10,	 0,	 1,	Func_Select8,	Func_ReturnFromFileBrowserFrame }, // File Button 8
	{	NULL,	FRAME_STRINGS[2],	120.0,	360.0,	400.0,	35.0,	 9,	11,	 0,	 1,	Func_Select9,	Func_ReturnFromFileBrowserFrame }, // File Button 9
	{	NULL,	FRAME_STRINGS[2],	120.0,	400.0,	400.0,	35.0,	10,	 2,	 0,	 1,	Func_Select10,	Func_ReturnFromFileBrowserFrame }, // File Button 10
};

FileBrowserFrame::FileBrowserFrame()
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
	setDefaultFocus(FRAME_BUTTONS[2].button);
	setBackFunc(Func_ReturnFromFileBrowserFrame);
	setEnabled(true);

}

FileBrowserFrame::~FileBrowserFrame()
{
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

static fileBrowser_file* dir_entries;
static int				num_entries;
static int				current_page;
static int				max_page;
static char				feedback_string[36];

void fileBrowserFrame_OpenDirectory(fileBrowser_file* dir);
void fileBrowserFrame_Error();
void fileBrowserFrame_FillPage();
void fileBrowserFrame_LoadFile(int i);

void Func_PrevPage()
{
	if(current_page > 0) current_page -= 1;
	fileBrowserFrame_FillPage();
}

void Func_NextPage()
{
	if(current_page+1 < max_page) current_page +=1;
	fileBrowserFrame_FillPage();
}

extern MenuContext *pMenuContext;

void Func_ReturnFromFileBrowserFrame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_MAIN);
}

void Func_Select1() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 0); }

void Func_Select2() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 1); }

void Func_Select3() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 2); }

void Func_Select4() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 3); }

void Func_Select5() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 4); }

void Func_Select6() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 5); }

void Func_Select7() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 6); }

void Func_Select8() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 7); }

void Func_Select9() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 8); }

void Func_Select10() { fileBrowserFrame_LoadFile((current_page*NUM_FILE_SLOTS) + 9); }


static char* filenameFromAbsPath(char* absPath)
{
	char* filename = absPath;
	// Here we want to extract from the absolute path
	//   just the filename
	// First we move the pointer all the way to the end
	//   of the the string so we can work our way back
	while( *filename ) ++filename;
	// Now, just move it back to the last '/' or the start
	//   of the string
	while( filename != absPath && (*(filename-1) != '\\' && *(filename-1) != '/'))
		--filename;
	return filename;
}

int loadROM(fileBrowser_file*);

void fileBrowserFrame_OpenDirectory(fileBrowser_file* dir)
{
	// Free the old menu stuff
//	if(menu_items){  free(menu_items);  menu_items  = NULL; }
	if(dir_entries){ free(dir_entries); dir_entries = NULL; }
	
	// Read the directories and return on error
	num_entries = romFile_readDir(dir, &dir_entries);
	if(num_entries <= 0)
	{ 
		if(dir_entries) free(dir_entries); 
		fileBrowserFrame_Error(); 
		return;
	}

	current_page = 0;
	max_page = (int)ceil((float)num_entries/NUM_FILE_SLOTS);
	fileBrowserFrame_FillPage();
}

void fileBrowserFrame_Error()
{
	//disable all buttons
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button->setActive(false);
	//set first entry to read 'error' and return to main menu
	strcpy(feedback_string,"An error occured");
	FRAME_BUTTONS[2].buttonString = feedback_string;
	FRAME_BUTTONS[2].button->setClicked(Func_ReturnFromFileBrowserFrame);
	FRAME_BUTTONS[2].button->setActive(true);
	for (int i = 1; i<NUM_FILE_SLOTS; i++)
		FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[2];
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
	menu::Focus::getInstance().clearPrimaryFocus();
}

void fileBrowserFrame_FillPage()
{
	//Restore next focus directions for top item in list
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_UP, FRAME_BUTTONS[FRAME_BUTTONS[2].focusUp].button);
	FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_DOWN, FRAME_BUTTONS[FRAME_BUTTONS[2].focusDown].button);

	//disable all buttons
	for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
		FRAME_BUTTONS[i].button->setActive(false);
	//set entries according to page
	for (int i = 0; i < NUM_FILE_SLOTS; i++)
	{
		if ((current_page*NUM_FILE_SLOTS) + i < num_entries)
		{
			FRAME_BUTTONS[i+2].buttonString = filenameFromAbsPath(dir_entries[i+(current_page*NUM_FILE_SLOTS)].name);
			FRAME_BUTTONS[i+2].button->setClicked(FRAME_BUTTONS[i+2].clickedFunc);
			FRAME_BUTTONS[i+2].button->setActive(true);
		}
		else
			FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[2];
	}
	if (!FRAME_BUTTONS[3].button->getActive())
	{ //NULL out up/down focus if there's only one item in list
		FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
	}
	if (current_page > 0) FRAME_BUTTONS[0].button->setActive(true);
	if (current_page+1 < max_page) FRAME_BUTTONS[1].button->setActive(true);
}

void fileBrowserFrame_LoadFile(int i)
{
	if(dir_entries[i].attr & FILE_BROWSER_ATTR_DIR){
		// Here we are 'recursing' into a subdirectory
		// We have to do a little dance here to avoid a dangling pointer
		fileBrowser_file* dir = (fileBrowser_file*)malloc( sizeof(fileBrowser_file) );
		memcpy(dir, dir_entries+i, sizeof(fileBrowser_file));
		fileBrowserFrame_OpenDirectory(dir);
		free(dir);
		menu::Focus::getInstance().clearPrimaryFocus();
	} else {
		// We must select this file
		int ret = loadROM( &dir_entries[i] );
		
		if(!ret){	// If the read succeeded.
			strcpy(feedback_string, "Loaded ");
			strncat(feedback_string, filenameFromAbsPath(dir_entries[i].name), 36-7);
		}
		else		// If not.
		{
			strcpy(feedback_string,"A read error occured");
		}

		//disable all buttons
		for (int i = 0; i < NUM_FRAME_BUTTONS; i++)
			FRAME_BUTTONS[i].button->setActive(false);
		//set first entry to report 'success'/'error' and return to main menu
		FRAME_BUTTONS[2].buttonString = feedback_string;
		FRAME_BUTTONS[2].button->setClicked(Func_ReturnFromFileBrowserFrame);
		FRAME_BUTTONS[2].button->setActive(true);
		FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_UP, NULL);
		FRAME_BUTTONS[2].button->setNextFocus(menu::Focus::DIRECTION_DOWN, NULL);
		for (int i = 1; i<NUM_FILE_SLOTS; i++)
			FRAME_BUTTONS[i+2].buttonString = FRAME_STRINGS[2];
		menu::Focus::getInstance().clearPrimaryFocus();
	}
}
