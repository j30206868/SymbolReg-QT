#ifndef CURSOR_H
#define CURSOR_H

#include <windows.h>

typedef unsigned char byte;

class Cursor {
public:
    Cursor();
    ~Cursor();
    void moveTo(int x, int y);

    long nowX;
    long nowY;
    long cX; //center x
    long cY; //center y
    long sW;
    long sH;
private:
    void GetDesktopResolution(int& horizontal, int& vertical);
};

#endif // CURSOR_H

