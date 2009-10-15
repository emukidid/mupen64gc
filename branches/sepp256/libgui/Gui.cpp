#include "Gui.h"
#include "IPLFont.h"
#include "InputManager.h"
#include "CursorManager.h"
#include "FocusManager.h"

namespace menu {

Gui::Gui(GXRModeObj *vmode)
		: gfx(vmode)
{
	IplFont::getInstance().setVmode(vmode);

	printf("Gui constructor\n");
}

void Gui::addFrame(Frame *frame)
{
//	printf("Gui addFrame\n");
	frameList.push_back(frame);
}

void Gui::removeFrame(Frame *frame)
{
	frameList.erase(std::remove(frameList.begin(),frameList.end(),frame),frameList.end());
}

void Gui::draw()
{
//	printf("Gui draw\n");
	Input::getInstance().refreshInput();
	Cursor::getInstance().updateCursor();
	Focus::getInstance().updateFocus();
	//Update time??
	//Get graphics framework and pass to Frame draw fns?
	gfx.drawInit();
	FrameList::const_iterator iteration;
	for (iteration = frameList.begin(); iteration != frameList.end(); iteration++)
	{
		(*iteration)->drawChildren(gfx);
	}
	Cursor::getInstance().drawCursor(gfx);

	gfx.swapBuffers();
}

} //namespace menu 
