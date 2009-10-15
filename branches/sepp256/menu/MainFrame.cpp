#include "MenuContext.h"
#include "MainFrame.h"
#include "../libgui/Button.h"
#include "../libgui/resources.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"
#ifdef DEBUGON
# include <debug.h>
#endif
#ifdef WII
extern "C" {
#include <di/di.h>
}
#endif 

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

static char mainFrameStrings[14][20] =
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

#define NUM_MAIN_BUTTONS 13

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
} mainFrameButtons[NUM_MAIN_BUTTONS] =
{ //	button	buttonString			x		y		width	height	Up	Dwn	Lft	Rt	clickFunc				returnFunc
	{	NULL,	mainFrameStrings[0],	 70.0,	 50.0,	250.0,	40.0,	 6,	 1,	 7,	 7,	Func_LoadROM,			NULL }, // Load ROM
	{	NULL,	mainFrameStrings[1],	 70.0,	110.0,	250.0,	40.0,	 0,	 2,	 8,	 8,	Func_LoadSave,			NULL }, // Load Save File
	{	NULL,	mainFrameStrings[2],	 70.0,	170.0,	250.0,	40.0,	 1,	 3,	 9,	 9,	Func_SaveGame,			NULL }, // Save Game
	{	NULL,	mainFrameStrings[3],	 70.0,	230.0,	250.0,	40.0,	 2,	 4,	10,	10,	Func_StateManage,		NULL }, // State Management
	{	NULL,	mainFrameStrings[4],	 70.0,	290.0,	250.0,	40.0,	 3,	 5,	11,	11,	Func_SelectCPU,			NULL }, // Select CPU Core
	{	NULL,	mainFrameStrings[5],	 70.0,	350.0,	250.0,	40.0,	 4,	 6,	12,	12,	Func_ShowCredits,		NULL }, // Show Credits
	{	NULL,	mainFrameStrings[6],	 70.0,	410.0,	250.0,	40.0,	 5,	 0,	-1,	-1,	Func_StopDVD,			NULL }, // Stop/Swap DVD
	{	NULL,	mainFrameStrings[7],	380.0,	 50.0,	250.0,	40.0,	12,	 8,	 0,	 0,	Func_DevFeatures,		NULL }, // Dev Features
	{	NULL,	mainFrameStrings[8],	380.0,	110.0,	250.0,	40.0,	 7,	 9,	 1,	 1,	Func_ControllerPaks,	NULL }, // Controller Paks
	{	NULL,	mainFrameStrings[9],	380.0,	170.0,	250.0,	40.0,	 8,	10,	 2,	 2,	Func_ToggleAudio,		NULL }, // Toggle Audio
	{	NULL,	mainFrameStrings[11],	380.0,	230.0,	250.0,	40.0,	 9,	11,	 3,	 3,	Func_ExitToLoader,		NULL }, // Exit to Loader
	{	NULL,	mainFrameStrings[12],	380.0,	290.0,	250.0,	40.0,	10,	12,	 4,	 4,	Func_RestartGame,		NULL }, // Restart Game
	{	NULL,	mainFrameStrings[13],	380.0,	350.0,	250.0,	40.0,	11,	 7,	 5,	 5,	Func_PlayGame,			NULL }, // Play Game
};

MainFrame::MainFrame()/*(MenuContext *menuContext)
		: menuContext(menuContext)*/
{
//	printf("MainFrame constructor\n");

	buttonImage = new menu::Image(ButtonTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	buttonFocusImage = new menu::Image(ButtonFocusTexture, 16, 16, GX_TF_I8, GX_CLAMP, GX_CLAMP, GX_FALSE);
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
		mainFrameButtons[i].button = new menu::Button(buttonImage, &mainFrameButtons[i].buttonString, 
										mainFrameButtons[i].x, mainFrameButtons[i].y, 
										mainFrameButtons[i].width, mainFrameButtons[i].height);

	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
	{
		mainFrameButtons[i].button->setFocusImage(buttonFocusImage);
		if (mainFrameButtons[i].focusUp != -1) mainFrameButtons[i].button->setNextFocus(menu::Focus::DIRECTION_UP, mainFrameButtons[mainFrameButtons[i].focusUp].button);
		if (mainFrameButtons[i].focusDown != -1) mainFrameButtons[i].button->setNextFocus(menu::Focus::DIRECTION_DOWN, mainFrameButtons[mainFrameButtons[i].focusDown].button);
		if (mainFrameButtons[i].focusLeft != -1) mainFrameButtons[i].button->setNextFocus(menu::Focus::DIRECTION_LEFT, mainFrameButtons[mainFrameButtons[i].focusLeft].button);
		if (mainFrameButtons[i].focusRight != -1) mainFrameButtons[i].button->setNextFocus(menu::Focus::DIRECTION_RIGHT, mainFrameButtons[mainFrameButtons[i].focusRight].button);
		mainFrameButtons[i].button->setActive(true);
		if (mainFrameButtons[i].clickedFunc) mainFrameButtons[i].button->setClicked(mainFrameButtons[i].clickedFunc);
		if (mainFrameButtons[i].returnFunc) mainFrameButtons[i].button->setReturn(mainFrameButtons[i].returnFunc);
		add(mainFrameButtons[i].button);
		menu::Cursor::getInstance().addComponent(this, mainFrameButtons[i].button, mainFrameButtons[i].x, 
												mainFrameButtons[i].x+mainFrameButtons[i].width, mainFrameButtons[i].y, 
												mainFrameButtons[i].y+mainFrameButtons[i].height);
	}
	setDefaultFocus(mainFrameButtons[0].button);
	setEnabled(true);

}

MainFrame::~MainFrame()
{
	for (int i = 0; i < NUM_MAIN_BUTTONS; i++)
	{
		menu::Cursor::getInstance().removeComponent(this, mainFrameButtons[i].button);
		delete mainFrameButtons[i].button;
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
}

void Func_SelectCPU()
{
	pMenuContext->setActiveFrame(MenuContext::FRAME_SELECTCPU);
}

void Func_ShowCredits()
{
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
			return; // "Motor stopped";
		}
		else 
			return; // "Game still needs DVD";
	}
	#ifndef HW_RVL
		dvd_motor_off();
		dvd_read_id();
	#else
		DI_StopMotor();
	#endif
	dvdInitialized = 0;
//	return "Motor Stopped";

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
}

extern char  audioEnabled;

void Func_ToggleAudio()
{
	audioEnabled ^= 1;
	if (audioEnabled) mainFrameButtons[9].buttonString = mainFrameStrings[10];
	else mainFrameButtons[9].buttonString = mainFrameStrings[9];
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
//		return "Game restarted";
	}
	else	
	{
//		return "Please load a ROM first";
	}
}

extern "C" {
void pauseAudio(void);  void pauseInput(void);
void resumeAudio(void); void resumeInput(void);
void go(void); 
}

void Func_PlayGame()
{
//	if(!hasLoadedROM) return "Please load a ROM first";
	if(!hasLoadedROM) return;
	
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
}
