#ifndef INPUTMANAGER_H
#define INPUTMANAGER_H

#include "GuiTypes.h"

namespace menu {

class Input
{
public:
	void refreshInput();
	WPADData* getWpad();
	PADStatus* getPad();
	static Input& getInstance()
	{
		static Input obj;
		return obj;
	}

private:
	Input();
	~Input();
	PADStatus gcPad[4];
	WPADData wiiPad[4];
	int wiiPadError[4];

};

} //namespace menu 

#endif
