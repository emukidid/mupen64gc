#include "FocusManager.h"
#include "InputManager.h"
#include "Frame.h"
#include "IPLFont.h"

namespace menu {

Focus::Focus()
		: focusActive(false),
		  pressed(false),
		  frameSwitch(true),
		  buttonsPressed(0),
		  focusList(0),
		  primaryFocusOwner(0),
		  secondaryFocusOwner(0)
{
	for (int i=0; i<4; i++) {
		previousButtonsWii[i] = 0;
		previousButtonsGC[i] = 0;
	}
}

Focus::~Focus()
{
}

void Focus::updateFocus()
{
	int focusDirection = 0;
	int buttonsDown = 0;
	WPADData* wiiPad = Input::getInstance().getWpad();
//	PADStatus* gcPad = Input::getInstance().getPad();

	if (!focusActive) return;

	if (frameSwitch)
	{
		primaryFocusOwner = NULL;
		frameSwitch = false;
	}

	for (int i=0; i<4; i++)
	{
		u16 currentButtonsGC = PAD_ButtonsHeld(i);
		if (currentButtonsGC ^ previousButtonsGC[i])
		{
			u16 currentButtonsDownGC = (currentButtonsGC ^ previousButtonsGC[i]) & currentButtonsGC;
			switch (currentButtonsDownGC & 0xf) {
			case PAD_BUTTON_LEFT:
				focusDirection = DIRECTION_LEFT;
				break;
			case PAD_BUTTON_RIGHT:
				focusDirection = DIRECTION_RIGHT;
				break;
			case PAD_BUTTON_DOWN:
				focusDirection = DIRECTION_DOWN;
				break;
			case PAD_BUTTON_UP:
				focusDirection = DIRECTION_UP;
				break;
			default:
				focusDirection = DIRECTION_NONE;
			}
			if (currentButtonsDownGC & PAD_BUTTON_A) buttonsDown |= ACTION_SELECT;
			if (currentButtonsDownGC & PAD_BUTTON_B) buttonsDown |= ACTION_BACK;
			if (primaryFocusOwner == NULL) primaryFocusOwner = currentFrame->getDefaultFocus();
			primaryFocusOwner = primaryFocusOwner->updateFocus(focusDirection,buttonsDown);
			previousButtonsGC[i] = currentButtonsGC;
			break;
		}
		if (wiiPad[i].btns_h ^ previousButtonsWii[i])
		{
			u32 currentButtonsDownWii = (wiiPad[i].btns_h ^ previousButtonsWii[i]) & wiiPad[i].btns_h;
			switch (currentButtonsDownWii & 0xf00) {
			case WPAD_BUTTON_LEFT:
				focusDirection = DIRECTION_LEFT;
				break;
			case WPAD_BUTTON_RIGHT:
				focusDirection = DIRECTION_RIGHT;
				break;
			case WPAD_BUTTON_DOWN:
				focusDirection = DIRECTION_DOWN;
				break;
			case WPAD_BUTTON_UP:
				focusDirection = DIRECTION_UP;
				break;
			default:
				focusDirection = DIRECTION_NONE;
			}
			if (currentButtonsDownWii & WPAD_BUTTON_A) buttonsDown |= ACTION_SELECT;
			if (currentButtonsDownWii & WPAD_BUTTON_B) buttonsDown |= ACTION_BACK;
			if (primaryFocusOwner == NULL) primaryFocusOwner = currentFrame->getDefaultFocus();
			primaryFocusOwner = primaryFocusOwner->updateFocus(focusDirection,buttonsDown);
			previousButtonsWii[i] = wiiPad[i].btns_h;
			break;
		}
	}
}

void Focus::addComponent(Component* component)
{
	focusList.push_back(component);
}

void Focus::removeComponent(Component* component)
{
	ComponentList::iterator iter = std::find(focusList.begin(), focusList.end(),component);
	if(iter != focusList.end())
	{
		focusList.erase(iter);
	}
}

void Focus::setCurrentFrame(Frame* frame)
{
	if(primaryFocusOwner) primaryFocusOwner->setFocus(false);
//	setFocusActive(false);
	primaryFocusOwner = NULL;
	currentFrame = frame;
	frameSwitch = true;
}

void Focus::setFocusActive(bool focusActiveBool)
{
	focusActive = focusActiveBool;
	if (!focusActive && primaryFocusOwner) primaryFocusOwner->setFocus(false);
}

void Focus::clearPrimaryFocus()
{
	if(primaryFocusOwner) primaryFocusOwner->setFocus(false);
	primaryFocusOwner = NULL;
}

} //namespace menu 
