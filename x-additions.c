#include <string.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include "x-additions.h"

void XWriteString(Display* x_display, char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if(str[i] >= 'A' && str[i] <= 'Z') {
            XTestFakeKeyEvent(x_display, XKeysymToKeycode(x_display, XK_Shift_L), True, 0);
        }
        XWriteSymbol(x_display, str[i]);
        if(str[i] >= 'A' && str[i] <= 'Z') {
            XTestFakeKeyEvent(x_display, XKeysymToKeycode(x_display, XK_Shift_L), False, 0);
        }
        XFlush(x_display);
    }
}

void XWriteSymbol(Display* x_display, KeySym sym) {
    int delay = 0;
    unsigned int keycode = XKeysymToKeycode(x_display, sym);
    XTestFakeKeyEvent(x_display, keycode, True, delay);
    XTestFakeKeyEvent(x_display, keycode, False, delay);
    XFlush(x_display);
}
