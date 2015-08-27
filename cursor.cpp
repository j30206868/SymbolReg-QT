#include "cursor.h"

const byte LED_PIN = 13; //use the LED @ Arduino pin 13, this should not change so make it const (constant)

//<<constructor>> setup the LED, make pin 13 an OUTPUT
Cursor::Cursor(){
    int pause;
    int *h, *v;

    h = new int();
    v = new int();

    GetDesktopResolution(*h, *v);
    nowX = *h/2;
    nowY = *v/2;

    sW = *h;
    sH = *v;

    cX = nowX;
    cY = nowY;

    //SetCursorPos(nowX , nowY);
}

//<<destructor>>
Cursor::~Cursor(){/*nothing to destruct*/}

//turn the LED on
void Cursor::moveTo(int x, int y){
    nowX = x;
    nowY = y;
    SetCursorPos( nowX, nowY );
}

void Cursor::GetDesktopResolution(int& horizontal, int& vertical)
{
   RECT desktop;
   // Get a handle to the desktop window
   const HWND hDesktop = GetDesktopWindow();
   // Get the size of screen to the variable desktop
   GetWindowRect(hDesktop, &desktop);
   // The top left corner will have coordinates (0,0)
   // and the bottom right corner will have coordinates
   // (horizontal, vertical)
   horizontal = desktop.right;
   vertical = desktop.bottom;
}
