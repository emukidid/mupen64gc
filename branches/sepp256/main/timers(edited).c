/****************************************************************************************
* timers.c - timer functions borrowed from mupen64 and mupen64plus, modified by sepp256
****************************************************************************************/

#include <time.h>
#include <stdio.h>
#include "winlnxdefs.h"
#include "ogc/lwp_watchdog.h"
#include "rom.h"
#include "timers.h"
#include "../r4300/r4300.h"
#include "../gui/DEBUG.h"

timers Timers = {0.0, 0.0, 0, 1, 0, 100};
static float VILimit = 60.0;
static double VILimitMicroseconds = 1000000.0/60.0;
static double VILimitSeconds = 1.0/60.0;

int GetVILimit(void)
{
	switch (ROM_HEADER->Country_code&0xFF)
	{
		// PAL codes
		case 0x44:
		case 0x46:
		case 0x49:
		case 0x50:
		case 0x53:
		case 0x55:
		case 0x58:
		case 0x59:
			return 50;
			break;

		// NTSC codes
		case 0x37:
		case 0x41:
		case 0x45:
		case 0x4a:
			return 60;
			break;

		// Fallback for unknown codes
		default:
			return 60;
	}
}

void InitTimer(void) {
	int temp;
	VILimit = GetVILimit();
	if (Timers.useFpsModifier) {
		temp = Timers.fpsModifier ; 
	}  
	else {
		temp = 100;
	}    
	VILimitMicroseconds = (double) 1000000.0/(VILimit * temp / 100);
	VILimitSeconds = (double) 1.0/(VILimit * temp / 100);
	Timers.frameDrawn = 0;
}

extern unsigned long gettick();
extern unsigned int usleep(unsigned int us);

void new_frame(void) {
	DWORD CurrentFPSTime;
	static DWORD LastFPSTime;
	static DWORD CounterTime;
	static int Fps_Counter=0;
	
	//if (!Config.showFPS) return;
	Fps_Counter++;
	Timers.frameDrawn = 1;
	
	CurrentFPSTime = ticks_to_microsecs(gettick());
	
	if (CurrentFPSTime - CounterTime >= 1000000 ) {
		Timers.fps = (float) (Fps_Counter * 1000000.0 / (CurrentFPSTime - CounterTime));
		sprintf(txtbuffer,"Timer.fps: Current = %dus; Last = %dus; FPS_count = %d", CurrentFPSTime, CounterTime, Fps_Counter);
		DEBUG_print(txtbuffer,1);
		CounterTime = ticks_to_microsecs(gettick());
		Fps_Counter = 0;
	}

	LastFPSTime = CurrentFPSTime ;
}

void new_vi(void) {
//	DWORD Dif;
//	DWORD CurrentFPSTime;
//	static DWORD LastFPSTime = 0;
//	static DWORD CounterTime = 0;
//	static DWORD CalculatedTime ;
	double Dif;
	time_t CurrentFPSTime;
	static time_t LastFPSTime = 0;
	static time_t CounterTime = 0;
	static time_t CalculatedTime ;
	static int VI_Counter = 0;
	static int VI_WaitCounter = 0;
	long time;
	bool init = TRUE;

	if(init) {
		time(&LastFPSTime);
		time(&CounterTime);
		init = FALSE;
	}

	start_section(IDLE_SECTION);
//	if ( (!Config.showVIS) && (!Config.limitFps) ) return;
	VI_Counter++;

//	CurrentFPSTime = ticks_to_microsecs(gettick());
	time(&CurrentFPSTime);

//	Dif = CurrentFPSTime - LastFPSTime;
	Dif = difftime(CurrentFPSTime,LastFPSTime);
	if (Timers.limitVIs) {
		if (Timers.limitVIs == 2 && Timers.frameDrawn == 0)
			VI_WaitCounter++;
		else
		{
//			if (Dif <  (double) VILimitMicroseconds * (VI_WaitCounter + 1) )
			if (Dif <  (double) VILimitSeconds * (VI_WaitCounter + 1) )
			{
//				CalculatedTime = CounterTime + (double)VILimitMicroseconds * (double)VI_Counter;
//				time = (int)(CalculatedTime - CounterTime);
				time = (int) (((double)VILimitSeconds * (double)VI_Counter - difftime(CurrentFPSTime,CounterTime)) * 1000000.0);
				if (time>0&&time<1000000) {
					usleep(time);
				}
				CurrentFPSTime = CurrentFPSTime + time;
			}
			Timers.frameDrawn = 0;
			VI_WaitCounter = 0;
		}
	}

//	CurrentFPSTime = ticks_to_millisecs(gettick());
//	CurrentFPSTime = gettick();
//	DWORD diff_millisecs = ticks_to_millisecs(diff_ticks(CounterTime,CurrentFPSTime));
	Dif = difftime(CurrentFPSTime,CounterTime);
//	if (CurrentFPSTime - CounterTime >= 1000.0 ) {
//	if (diff_millisecs >= 1000.0 ) {
	if (Dif >= 1.0 ) {
//		if (VI_Counter) Timers.vis = (float) (VI_Counter * 1000.0 / (CurrentFPSTime - CounterTime));
//		if (VI_Counter) Timers.vis = (float) (VI_Counter * 1000.0 / diff_millisecs);
		if (VI_Counter) Timers.vis = (float) (VI_Counter / Dif);
//		sprintf(txtbuffer,"Timer.VIs: Current = %dms; Last = %dms; diff_ms = %d; FPS_count = %d", CurrentFPSTime, CounterTime, diff_millisecs, VI_Counter);
//		DEBUG_print(txtbuffer,0);
//		CounterTime = ticks_to_microsecs(gettick());
//		CounterTime = ticks_to_millisecs(gettick());
//		CounterTime = gettick();
		time(&CounterTime);
		VI_Counter = 0 ;
	}

	LastFPSTime = CurrentFPSTime ;
    end_section(IDLE_SECTION);
}
