#include "MenuContext.h"
#include "MainFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#include "../libgui/MessageBox.h"
#ifdef DEBUGON
# include <debug.h>
#endif
extern "C" {
#ifdef WII
#include <di/di.h>
#endif 
#include "../main/gc_dvd.h"
}
#include <ogc/dvd.h>

void Func_LoadROM();
void Func_LoadSave();
void Func_SaveGame();
void Func_StateManage();
void Func_SelectCPU();
void Func_ShowCredits();
void Func_StopDVD();
void Func_DevFeatures();
void Func_ControllerPaks();
void Func_ToggleAudio();
void Func_ExitToLoader();
void Func_RestartGame();
void Func_PlayGame();

#define NUM_MAIN_BUTTONS 13
#define FRAME_BUTTONS mainFrameButtons
#define FRAME_STRINGS mainFrameStrings

static char FRAME_STRINGS[14][20] =
	{ "Load ROM",
	  "Load Save File",
	  "Save Game",
	  "State Management",
	  "Select CPU Core",
	  "Show Credits",
	  "Stop/Swap DVD",
	  "Dev Features",
	  "Controller Paks",
	  "Toggle Audio (off)",
	  "Toggle Audio (on)",
	  "Exit to Loader",
	  "Restart Game",
	  "Play Game"};


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
} FRAME_BUTTONS[NUM_MAIN_BUTTONS] =
{ //	button	buttonString			x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	FRAME_STRINGS[0],	 45.0,	 50.0,	250.0,	40.0,	 6,	 1,	 7,	 7,	Func_LoadROM,			NULL }, // Load ROM
	{	NULL,	FRAME_STRINGS[1],	 45.0,	110.0,	250.0,	40.0,	 0,	 2,	 8,	 8,	Func_LoadSave,			NULL }, // Load Save File
	{	NULL,	FRAME_STRINGS[2],	 45.0,	170.0,	250.0,	40.0,	 1,	 3,	 9,	 9,	Func_SaveGame,			NULL }, // Save Game
	{	NULL,	FRAME_STRINGS[3],	 45.0,	230.0,	250.0,	40.0,	 2,	 4,	10,	10,	Func_StateManage,		NULL }, // State Management
	{	NULL,	FRAME_STRINGS[4],	 45.0,	290.0,	250.0,	40.0,	 3,	 5,	11,	11,	Func_SelectCPU,			NULL }, // Select CPU Core
	{	NULL,	FRAME_STRINGS[5],	 45.0,	350.0,	250.0,	40.0,	 4,	 6,	12,	12,	Func_ShowCredits,		NULL }, // Show Credits
	{	NULL,	FRAME_STRINGS[6],	 45.0,	410.0,	250.0,	40.0,	 5,	 0,	-1,	-1,	Func_StopDVD,			NULL }, // Stop/Swap DVD
	{	NULL,	FRAME_STRINGS[7],	345.0,	 50.0,	250.0,	40.0,	12,	 8,	 0,	 0,	Func_DevFeatures,		NULL }, // Dev Features
	{	NULL,	FRAME_STRINGS[8],	345.0,	110.0,	250.0,	40.0,	 7,	 9,	 1,	 1,	Func_ControllerPaks,	NULL }, // Controller Paks
	{	NULL,	FRAME_STRINGS[9],	345.0,	170.0,	250.0,	40.0,	 8,	10,	 2,	 2,	Func_ToggleAudio,		NULL }, // Toggle Audio
	{	NULL,	FRAME_STRINGS[11],	345.0,	230.0,	250.0,	40.0,	 9,	11,	 3,	 3,	Func_ExitToLoader,		NULL }, // Exit to Loader
	{	NULL,	FRAME_STRINGS[12],	345.0,	290.0,	250.0,	40.0,	10,	12,	 4,	 4,	Func_RestartGame,		NULL }, // Restart Game
	{	NULL,	FRAME_STRINGS[13],	345.0,	350.0,	250.0,	40.0,	11,	 7,	 5,	 5,	Func_PlayGame,			NULL }, // Play Game
};

MainFrame::MainFrame()
{
	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
		FRAME_BUTTONS[i].button = new menu::Button(buttonImage, &FRAME_BUTTONS[i].buttonString, 
										FRAME_BUTTONS[i].x, FRAME_BUTTONS[i].y, 
										FRAME_BUTTONS[i].width, FRAME_BUTTONS[i].height);

	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
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

MainFrame::~MainFrame()
{
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, FRAME_BUTTONS[i].button);
		delete FRAME_BUTTONS[i].button;
	}
	delete buttonFocusImage;
	delete buttonImage;

}

extern MenuContext *pMenuContext;

void Func_LoadROM()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADROM);
}

void Func_LoadSave()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_LOADSAVE);
}

void Func_SaveGame()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SAVEGAME);
}

void Func_StateManage()
{
	menu::MessageBox::getInstance().setMessage("Save states not implemented");
}

void Func_SelectCPU()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTCPU);
}

void Func_ShowCredits()
{
	menu::MessageBox::getInstance().setMessage("Credits not implemented");
}

extern int rom_length;
extern int dvdInitialized;
extern BOOL hasLoadedROM;

void Func_StopDVD()
{
	if((hasLoadedROM) && (dvdInitialized)){
		dvdInitialized = 0;
		if (rom_length<15728640){
		#ifndef HW_RVL
			dvd_motor_off();
			dvd_read_id();
		#else
			DI_StopMotor();
		#endif
			menu::MessageBox::getInstance().setMessage("Motor stopped");
			return;
		}
		else 
			menu::MessageBox::getInstance().setMessage("Game still needs DVD");
			return;
	}
	#ifndef HW_RVL
		dvd_motor_off();
		dvd_read_id();
	#else
		DI_StopMotor();
	#endif
	dvdInitialized = 0;
	menu::MessageBox::getInstance().setMessage("Motor stopped");
	return;

/*	//TODO: Add Swap DVD functionality here?
	static char* dvd_swap_func(){
	#ifndef HW_RVL
		dvd_motor_off();
		dvd_read_id();
	#else
		DI_Eject();
		DI_Mount();
	#endif
		return "Swap disc now";
	}*/
}

void Func_DevFeatures()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_DEV);
}

void Func_ControllerPaks()
{
	menu::MessageBox::getInstance().setMessage("Controller pak menu not implemented");
}

extern char  audioEnabled;

void Func_ToggleAudio()
{
	audioEnabled ^= 1;
	if (audioEnabled) FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[10];
	else FRAME_BUTTONS[9].buttonString = FRAME_STRINGS[9];
}

void Func_ExitToLoader()
{
#ifdef WII
	DI_Close();
#endif
	void (*rld)() = (void (*)()) 0x80001800;
	rld();
}

extern "C" {
void cpu_init();
void cpu_deinit();
}

void Func_RestartGame()
{
	if(hasLoadedROM)
	{
		cpu_deinit();
		cpu_init();
		menu::MessageBox::getInstance().setMessage("Game restarted");
	}
	else	
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
	}
}

extern "C" {
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
void go(void); 
}

void Func_PlayGame()
{
	if(!hasLoadedROM)
	{
		menu::MessageBox::getInstance().setMessage("Please load a ROM first");
		return;
	}
	
	resumeAudio();
	resumeInput();
#ifdef DEBUGON
	_break();
#endif
	go();
#ifdef DEBUGON
	_break();
#endif
	pauseInput();
	pauseAudio();

	menu::Cursor::getInstance().clearCursorFocus();
}
