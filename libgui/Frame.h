#ifndef FRAME_H
#define FRAME_H

#include "GuiTypes.h"
#include "Component.h"

namespace menu {

class Frame : public Component
{
public:
	Frame();
	~Frame();
	void showFrame();
	void hideFrame();
	void setEnabled(bool enable);
	void drawChildren(Graphics& gfx) const;
	void remove(Component* component);
	void removeAll();
	void add(Component* comp);
	void updateTime(float deltaTime);
	void setDefaultFocus(Component* comp);
	Component* getDefaultFocus();

private:
	Component* defaultFocus;
};

} //namespace menu 

#endif
