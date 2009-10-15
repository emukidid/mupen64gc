#ifndef GUI_H
#define GUI_H

#include "GuiTypes.h"
#include "Frame.h"
#include "GraphicsGX.h"

namespace menu {

class Gui
{
public:
	Gui(GXRModeObj *vmode);
	void addFrame(Frame* frame);
	void removeFrame(Frame* frame);
	void draw();
private:
	Graphics gfx;
	FrameList frameList;
};

} //namespace menu 

#endif
