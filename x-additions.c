#include <string.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include "x-additions.h"

void XWriteString(Display* x_display, char* str) {
    int delay = 0;
    char buf[2];
    buf[1] = '\0';
    int len = strlen(str);
    for (int i = 0; i < len; i++) {
        if (str[i] == ' ') {
            XWriteSymbol(x_display,XK_space);
        } else {
            buf[0] = str[i];
            unsigned int keycode = XKeysymToKeycode(x_display, XStringToKeysym(buf));
            if(buf[0] >= 'A' && buf[0] <= 'Z') {
                XTestFakeKeyEvent(x_display, XKeysymToKeycode(x_display, XK_Shift_L), True, delay);
            }
            XTestFakeKeyEvent(x_display, keycode, True, delay);
            XTestFakeKeyEvent(x_display, keycode, False, delay);
            if(buf[0] >= 'A' && buf[0] <= 'Z') {
                XTestFakeKeyEvent(x_display, XKeysymToKeycode(x_display, XK_Shift_L), False, delay);
            }
        }
    }
    XFlush(x_display);
}

void XWriteSymbol(Display* x_display, KeySym sym) {
    int delay = 0;
    unsigned int keycode = XKeysymToKeycode(x_display, sym);
    XTestFakeKeyEvent(x_display, keycode, True, delay);
    XTestFakeKeyEvent(x_display, keycode, False, delay);
    XFlush(x_display);
}
