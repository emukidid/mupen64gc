/**
 * Wii64 - controller-WiimoteNunchuk.c
 * Copyright (C) 2007, 2008, 2009 Mike Slegeir
 * Copyright (C) 2007, 2008, 2009 sepp256
 * 
 * Wiimote + Nunchuk input module
 *
 * Wii64 homepage: http://www.emulatemii.com
 * email address: tehpola@gmail.com
 *                sepp256@gmail.com
 *
 *
 * This program is free software; you can redistribute it and/
 * or modify it under the terms of the GNU General Public Li-
 * cence as published by the Free Software Foundation; either
 * version 2 of the Licence, or any later version.
 *
 * This program is distributed in the hope that it will be use-
 * ful, but WITHOUT ANY WARRANTY; without even the implied war-
 * ranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public Licence for more details.
 *
**/


#include <string.h>
#include <math.h>
#include <wiiuse/wpad.h>
#include "controller.h"

#ifndef PI
#define PI 3.14159f
#endif

enum { STICK_X, STICK_Y };
static int getStickValue(joystick_t* j, int axis, int maxAbsValue){
	double angle = PI * j->ang/180.0f;
	double magnitude = (j->mag > 1.0f) ? 1.0f :
	                    (j->mag < -1.0f) ? -1.0f : j->mag;
	double value;
	if(axis == STICK_X)
		value = magnitude * sin( angle );
	else
		value = magnitude * cos( angle );
	return (int)(value * maxAbsValue);
}

static int _GetKeys(int Control, BUTTONS * Keys )
{
	if(wpadNeedScan){ WPAD_ScanPads(); wpadNeedScan = 0; }
	WPADData* wpad = WPAD_Data(Control);
	BUTTONS* c = Keys;
	memset(c, 0, sizeof(BUTTONS));

	// Only use a connected nunchuck controller
	if(wpad->err == WPAD_ERR_NONE &&
	   wpad->exp.type == WPAD_EXP_NUNCHUK){
		controller_WiimoteNunchuk.available[Control] = 1;
	} else {
		controller_WiimoteNunchuk.available[Control] = 0;
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_CLASSIC){
			controller_Classic.available[Control] = 1;
		}
		return 0;
	}

	unsigned int b  = wpad->btns_h;
	int isHeld(unsigned int button){ return (b & button) == button; }
	
	c->R_DPAD       = isHeld(WPAD_BUTTON_RIGHT | WPAD_BUTTON_2);
	c->L_DPAD       = isHeld(WPAD_BUTTON_LEFT  | WPAD_BUTTON_2);
	c->D_DPAD       = isHeld(WPAD_BUTTON_DOWN  | WPAD_BUTTON_2);
	c->U_DPAD       = isHeld(WPAD_BUTTON_UP    | WPAD_BUTTON_2);
	c->START_BUTTON = isHeld(WPAD_BUTTON_HOME);
	c->B_BUTTON     = isHeld(WPAD_BUTTON_MINUS) | isHeld(WPAD_BUTTON_PLUS);
	c->A_BUTTON     = isHeld(WPAD_BUTTON_A);

	c->Z_TRIG       = isHeld(WPAD_NUNCHUK_BUTTON_Z);
	c->R_TRIG       = isHeld(WPAD_BUTTON_B);
	c->L_TRIG       = isHeld(WPAD_NUNCHUK_BUTTON_C);

	c->R_CBUTTON    = isHeld(WPAD_BUTTON_RIGHT);
	c->L_CBUTTON    = isHeld(WPAD_BUTTON_LEFT);
	c->D_CBUTTON    = isHeld(WPAD_BUTTON_DOWN);
	c->U_CBUTTON    = isHeld(WPAD_BUTTON_UP);

	c->X_AXIS       = getStickValue(&wpad->exp.nunchuk.js, STICK_X, 127);
	c->Y_AXIS       = getStickValue(&wpad->exp.nunchuk.js, STICK_Y, 127);

	// 1+2 quits to menu
	return isHeld(WPAD_BUTTON_1 | WPAD_BUTTON_2);
}

static void pause(int Control){ }

static void resume(int Control){ }

static void rumble(int Control, int rumble){
	WPAD_Rumble(Control, rumble ? 1 : 0);
}

static void configure(int Control){
	// Don't know how this should be integrated
}

static void assign(int p, int v){
	// TODO: Light up the LEDs appropriately
}

static void init(void);

controller_t controller_WiimoteNunchuk =
	{ _GetKeys,
	  configure,
	  init,
	  assign,
	  pause,
	  resume,
	  rumble,
	  {0, 0, 0, 0}
	 };

static void init(void){
	int i;
	WPAD_ScanPads();
	for(i=0; i<4; ++i){
		WPADData* wpad = WPAD_Data(i);
		// Only use a connected nunchuk
		if(wpad->err == WPAD_ERR_NONE &&
		   wpad->exp.type == WPAD_EXP_NUNCHUK){
			controller_WiimoteNunchuk.available[i] = 1;
			WPAD_SetDataFormat(i, WPAD_DATA_EXPANSION);
		} else
			controller_WiimoteNunchuk.available[i] = 0;
	}
}
