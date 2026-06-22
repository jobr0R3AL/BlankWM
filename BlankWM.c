#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/cursorfont.h>
#include <X11/XF86keysym.h>
#include <X11/Xatom.h>
#include <unistd.h>
#include <stdlib.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))


int get_window_workspace(Display *dpy, Window win, Atom ws_atom) {
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytes_after;
    unsigned char *prop = NULL;
    int ws = 1;

    if (XGetWindowProperty(dpy, win, ws_atom, 0, 1, False, XA_CARDINAL,
                           &actual_type, &actual_format, &nitems, &bytes_after, &prop) == Success && prop) {
        ws = (int)*prop;
        XFree(prop);
    }
    return ws;
}

void set_window_workspace(Display *dpy, Window win, Atom ws_atom, int ws) {
    unsigned char prop = ws;
    XChangeProperty(dpy, win, ws_atom, XA_CARDINAL, 8, PropModeReplace, &prop, 1);
}

void switch_workspace(Display *dpy, Window root, int new_ws, int *current_ws, Atom ws_atom) {
    if (new_ws == *current_ws) return;

    unsigned int nwins;
    Window root_return, parent_return, *wins = NULL;
    
    if (XQueryTree(dpy, root, &root_return, &parent_return, &wins, &nwins) && wins) {
        for (unsigned int i = 0; i < nwins; i++) {
            XWindowAttributes attr;
            XGetWindowAttributes(dpy, wins[i], &attr);
            if (wins[i] == root || attr.override_redirect) continue;

            int win_ws = get_window_workspace(dpy, wins[i], ws_atom);

            if (win_ws == *current_ws) {
                XUnmapWindow(dpy, wins[i]);
            }

            else if (win_ws == new_ws) {
                XMapWindow(dpy, wins[i]);
            }
        }
        XFree(wins);
    }
    *current_ws = new_ws;
}

int main(void) {
    Display * dpy;
    Window root;
    XEvent ev;
    XWindowAttributes attr;
    XButtonEvent start;
    Cursor cursor;
    int current_ws = 1;

    if (!(dpy = XOpenDisplay(0x0))) return 1;
    root = DefaultRootWindow(dpy);

    Atom ws_atom = XInternAtom(dpy, "_BLANKWM_WORKSPACE", False);

    XStoreName(dpy, root, "BlankWM");

    int screen_width = DisplayWidth(dpy, DefaultScreen(dpy));
    int screen_height = DisplayHeight(dpy, DefaultScreen(dpy));

    cursor = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cursor);

    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_Return), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_m), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_n), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, XKeysymToKeycode(dpy, XK_d), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);

    for (int i = 1; i <= 9; i++) {
        XGrabKey(dpy, XKeysymToKeycode(dpy, XK_0 + i), Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    }

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
            KeySym keysym = XLookupKeysym(&ev.xkey, 0);

            if (keysym >= XK_1 && keysym <= XK_9) {
                int target_ws = keysym - XK_0;
                switch_workspace(dpy, root, target_ws, &current_ws, ws_atom);
            }
            
            else if (keysym == XK_Return) {
                if (fork() == 0) {
                    setenv("DISPLAY", ":0", 1);
                    execl("/usr/local/bin/ghostty", "ghostty", NULL);
                    exit(0);
                }
            }
            else if (keysym == XK_d) {
                if (fork() == 0) {
                    setenv("DISPLAY", ":0", 1);
                    execl("/usr/local/bin/rofi", "rofi", "-show", "run", NULL);
                    exit(0);
                }
            }
            else if (keysym == XF86XK_AudioRaiseVolume) {
                if (fork() == 0) { execl("/usr/sbin/mixer", "mixer", "vol", "+5", NULL); exit(0); }
            }
            else if (keysym == XF86XK_AudioLowerVolume) {
                if (fork() == 0) { execl("/usr/sbin/mixer", "mixer", "vol", "-5", NULL); exit(0); }
            }
            else if (keysym == XF86XK_AudioMute) {
                if (fork() == 0) { execl("/usr/sbin/mixer", "mixer", "vol", "^", NULL); exit(0); }
            }

            Window focused_win;
            int revert_to;
            XGetInputFocus(dpy, &focused_win, &revert_to);

            if (focused_win != None && focused_win != root) {
                if (keysym == XK_q) {
                    XKillClient(dpy, focused_win);
                }
                else if (keysym == XK_m) {
                    XMoveResizeWindow(dpy, focused_win, 0, 0, screen_width, screen_height);
                }
                else if (keysym == XK_n) {
                    XUnmapWindow(dpy, focused_win);
                }
            }
        }
        

        else if (ev.type == ButtonPress && ev.xbutton.subwindow != None) {
            XSetInputFocus(dpy, ev.xbutton.subwindow, RevertToParent, CurrentTime);
            XRaiseWindow(dpy, ev.xbutton.subwindow);
            
            if (get_window_workspace(dpy, ev.xbutton.subwindow, ws_atom) == 1 && current_ws != 1) {
                set_window_workspace(dpy, ev.xbutton.subwindow, ws_atom, current_ws);
            } else {
                set_window_workspace(dpy, ev.xbutton.subwindow, ws_atom, current_ws);
            }

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

