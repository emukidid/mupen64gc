#include "MenuContext.h"
#include "../libgui/FocusManager.h"
#include "../libgui/CursorManager.h"

MenuContext *pMenuContext;

MenuContext::MenuContext(GXRModeObj *vmode)
		: guiInstance(new menu::Gui(vmode)),
		  currentActiveFrame(0),
		  mainFrame(0),
		  loadRomFrame(0),
		  fileBrowserFrame(0),
		  loadSaveFrame(0),
		  selectCPUFrame(0),
		  devFrame(0)
{
	pMenuContext = this;

	mainFrame = new MainFrame();
	loadRomFrame = new LoadRomFrame();
	fileBrowserFrame = new FileBrowserFrame();
	loadSaveFrame = new LoadSaveFrame();
	saveGameFrame = new SaveGameFrame();
	selectCPUFrame = new SelectCPUFrame();
	devFrame = new DevFrame();

	guiInstance->addFrame(mainFrame);
	guiInstance->addFrame(loadRomFrame);
	guiInstance->addFrame(fileBrowserFrame);
	guiInstance->addFrame(loadSaveFrame);
	guiInstance->addFrame(saveGameFrame);
	guiInstance->addFrame(selectCPUFrame);
	guiInstance->addFrame(devFrame);

	setActiveFrame(FRAME_MAIN);
	menu::Focus::getInstance().setFocusActive(true);
}

MenuContext::~MenuContext()
{
	delete devFrame;
	delete selectCPUFrame;
	delete saveGameFrame;
	delete loadSaveFrame;
	delete fileBrowserFrame;
	delete loadRomFrame;
	delete mainFrame;
	delete guiInstance;
	pMenuContext = NULL;
}

bool MenuContext::isRunning()
{
	bool isRunning = true;
//	printf("MenuContext isRunning\n");
	draw();

/*	PADStatus* gcPad = menu::Input::getInstance().getPad();
	if(gcPad[0].button & PAD_BUTTON_START)
		isRunning = false;*/
	
	return isRunning;
}

void MenuContext::setActiveFrame(int frameIndex)
{
	if(currentActiveFrame)
		currentActiveFrame->hideFrame();

	switch(frameIndex) {
	case FRAME_MAIN:
		currentActiveFrame = mainFrame;
		break;
	case FRAME_LOADROM:
		currentActiveFrame = loadRomFrame;
		break;
	case FRAME_FILEBROWSER:
		currentActiveFrame = fileBrowserFrame;
		break;
	case FRAME_LOADSAVE:
		currentActiveFrame = loadSaveFrame;
		break;
	case FRAME_SAVEGAME:
		currentActiveFrame = saveGameFrame;
		break;
	case FRAME_SELECTCPU:
		currentActiveFrame = selectCPUFrame;
		break;
	case FRAME_DEV:
		currentActiveFrame = devFrame;
		break;
	}

	if(currentActiveFrame)
	{
		currentActiveFrame->showFrame();
		menu::Focus::getInstance().setCurrentFrame(currentActiveFrame);
		menu::Cursor::getInstance().setCurrentFrame(currentActiveFrame);
	}
}

void MenuContext::draw()
{
	guiInstance->draw();
}
