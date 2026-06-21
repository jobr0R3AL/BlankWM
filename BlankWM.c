#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/XF86keysym.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))

int main(void) {
    Display * dpy;
    Window root;
    XEvent ev;
    XWindowAttributes attr;
    XButtonEvent start;
    Cursor cursor;

    if (!(dpy = XOpenDisplay(0x0))) return 1;
    root = DefaultRootWindow(dpy);

    int screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));

    cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Return), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_m), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_n), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_d), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume), AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioLowerVolume), AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XF86XK_AudioMute),        AnyModifier, root, True, GrabModeAsync, GrabModeAsync);

    XGrabButton(dpy, 1, Mod1Mask, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);
    XGrabButton(dpy, 3, Mod1Mask, root, True, ButtonPressMask|ButtonReleaseMask|PointerMotionMask, GrabModeAsync, GrabModeAsync, None, None);

    start.subwindow = None;

    if (fork() == 0) {
        setenv("DISPLAY", ":0", 1);
        execl("/usr/local/bin/ghostty", "ghostty", NULL);
        exit(0);
    }

    while (!XNextEvent(dpy, &ev)) {
        if (ev.type == KeyPress) {
            if (ev.xkey.keycode == XKeysymToKeycode(dpy, XK_Return)) {
                if (fork() == 0) {
                    setenv("DISPLAY", ":0", 1);
                    execl("/usr/local/bin/ghostty", "ghostty", NULL);
                    exit(0);
                }
            }
            
            else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XK_d)) {
                if (fork() == 0) {
                    setenv("DISPLAY", ":0", 1);
                    execl("/usr/local/bin/rofi", "rofi", "-show", "run", NULL);
                    exit(0);
                }
            }

            else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XF86XK_AudioRaiseVolume)) {
                if (fork() == 0) {
                    execl("/usr/sbin/mixer", "mixer", "vol", "+5", NULL);
                    exit(0);
                }
            }
            else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XF86XK_AudioLowerVolume)) {
                if (fork() == 0) {
                    execl("/usr/sbin/mixer", "mixer", "vol", "-5", NULL);
                    exit(0);
                }
            }
            else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XF86XK_AudioMute)) {
                if (fork() == 0) {
                    execl("/usr/sbin/mixer", "mixer", "vol", "^", NULL);
                    exit(0);
                }
            }

            Window focused_win;
            int revert_to;
            XGetInputFocus(dpy, &focused_win, &revert_to);

            if (focused_win != None && focused_win != root) {
                if (ev.xkey.keycode == XKeysymToKeycode(dpy, XK_q)) {
                    XKillClient(dpy, focused_win);
                }
                else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XK_m)) {
                    XMoveResizeWindow(dpy, focused_win, 0, 0, screen_width, screen_height);
                }
                else if (ev.xkey.keycode == XKeysymToKeycode(dpy, XK_n)) {
                    XUnmapWindow(dpy, focused_win);
                }
            }
        }
        else if (ev.type == ButtonPress && ev.xbutton.subwindow != None) {
            XRaiseWindow(dpy, ev.xbutton.subwindow);
            XGetWindowAttributes(dpy, ev.xbutton.subwindow, &attr);
            start = ev.xbutton;
        }
        else if (ev.type == MotionNotify && start.subwindow != None) {
            int xdiff = ev.xbutton.x_root - start.x_root;
            int ydiff = ev.xbutton.y_root - start.y_root;
            XMoveResizeWindow(dpy, start.subwindow,
                attr.x + (start.button==1 ? xdiff : 0),
                attr.y + (start.button==1 ? ydiff : 0),
                MAX(1, attr.width + (start.button==3 ? xdiff : 0)),
                MAX(1, attr.height + (start.button==3 ? ydiff : 0)));
        }
        else if (ev.type == ButtonRelease) {
            start.subwindow = None;
        }
    }
}
