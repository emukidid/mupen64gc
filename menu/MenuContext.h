#ifndef MENUCONTEXT_H
#define MENUCONTEXT_H

#include "../libgui/Gui.h"
#include "../libgui/Frame.h"
#include "../libgui/InputManager.h"
#include "MainFrame.h"
#include "LoadRomFrame.h"
#include "FileBrowserFrame.h"
#include "LoadSaveFrame.h"
#include "SaveGameFrame.h"
#include "SelectCPUFrame.h"
#include "DevFrame.h"

#include "MenuTypes.h"

class MenuContext
{
public:
	MenuContext(GXRModeObj *vmode);
	~MenuContext();
	bool isRunning();
	void setActiveFrame(int frameIndex);
	enum FrameIndices
	{
		FRAME_MAIN=1,
		FRAME_LOADROM,
		FRAME_FILEBROWSER,
		FRAME_LOADSAVE,
		FRAME_SAVEGAME,
		FRAME_SELECTCPU,
		FRAME_DEV
		
	};


private:
	void draw();
	menu::Frame *currentActiveFrame;
	MainFrame *mainFrame;
	LoadRomFrame *loadRomFrame;
	FileBrowserFrame *fileBrowserFrame;
	LoadSaveFrame *loadSaveFrame;
	SaveGameFrame *saveGameFrame;
	SelectCPUFrame *selectCPUFrame;
	DevFrame *devFrame;

};

#endif
