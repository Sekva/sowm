#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

#include "sowm.h"

static client       *list = {0}, *ws_list[1] = {0}, *cur;
static int          ws = 1, sw, sh, wx, wy, numlock = 0;
static unsigned int ww, wh;

static Display      *d;
static XButtonEvent mouse;
static Window       root;
int          s;

Window janela_ir_menu;
Window janela_ir_janela;
Window janela_ir_systray;
Window janela_voltar_systray;

enum Estado {
    Janela,
    Menu,
    Systray,
};
enum Estado estado_atual  = Janela;


static void (*events[LASTEvent])(XEvent *e) = {
    [ButtonPress]      = button_press,
    [ButtonRelease]    = button_release,
    [ConfigureRequest] = configure_request,
    [KeyPress]         = key_press,
    [MapRequest]       = map_request,
    [DestroyNotify]    = notify_destroy,
    [EnterNotify]      = notify_enter,
    [MotionNotify]     = notify_motion
};

#include "config.h"

void win_focus(client *c) {
    cur = c;
    XSetInputFocus(d, cur->w, RevertToParent, CurrentTime);
    XSetWindowBorder(d, cur->w, 16711680);
}

void notify_destroy(XEvent *e) {
    win_del(e->xdestroywindow.window);

    if (list) win_focus(list->prev);
}

void notify_enter(XEvent *e) {
    while(XCheckTypedEvent(d, EnterNotify, e));

    for win if (c->w == e->xcrossing.window) win_focus(c);
}

void notify_motion(XEvent *e) {
    if (!mouse.subwindow || cur->f) return;

    while(XCheckTypedEvent(d, MotionNotify, e));

    int xd = e->xbutton.x_root - mouse.x_root;
    int yd = e->xbutton.y_root - mouse.y_root;

    XMoveResizeWindow(d, mouse.subwindow,
        wx + (mouse.button == 1 ? xd : 0),
        wy + (mouse.button == 1 ? yd : 0),
        MAX(1, ww + (mouse.button == 3 ? xd : 0)),
        MAX(1, wh + (mouse.button == 3 ? yd : 0)));
}

void key_press(XEvent *e) {
    KeySym keysym = XkbKeycodeToKeysym(d, e->xkey.keycode, 0, 0);

    for (unsigned int i=0; i < sizeof(keys)/sizeof(*keys); ++i)
        if (keys[i].keysym == keysym &&
            mod_clean(keys[i].mod) == mod_clean(e->xkey.state))
            keys[i].function(keys[i].arg);
}

void button_press(XEvent *e) {
    if (!e->xbutton.subwindow) return;

    win_size(e->xbutton.subwindow, &wx, &wy, &ww, &wh);
    XRaiseWindow(d, e->xbutton.subwindow);
    mouse = e->xbutton;
}

void button_release(XEvent *e) {
    mouse.subwindow = 0;
}

void win_add(Window w,  int bordas_check) {

    client *c;

    if (!(c = (client *) calloc(1, sizeof(client))))
        exit(1);


    int bordas = 40;
    if(bordas_check) {
        XWindowChanges wc;
        wc.x = bordas;
        wc.y = bordas;
        wc.width = sw - bordas;
        wc.height= sh - bordas;
        wc.border_width = 4;
        XConfigureWindow(d, w, CWBorderWidth, &wc);
        XResizeWindow(d, w, sw - 2*bordas, sh - 2*bordas);
    }

    c->w  = w;
    c->wx = bordas;
    c->wy = bordas;
    c->ww = sw - bordas;
    c->wh = sh - bordas;

    if (list) {
        list->prev->next = c;
        c->prev          = list->prev;
        list->prev       = c;
        c->next          = list;

    } else {
        list = c;
        list->prev = list->next = list;
    }

    ws_save(ws);
}

void win_del(Window w) {
    client *x = 0;

    for win if (c->w == w) x = c;

    if (!list || !x)  return;
    if (x->prev == x) list = 0;
    if (list == x)    list = x->next;
    if (x->next)      x->next->prev = x->prev;
    if (x->prev)      x->prev->next = x->next;

    free(x);
    ws_save(ws);
}

void win_kill(const Arg arg) {
    if (cur) XKillClient(d, cur->w);
}

void win_center(const Arg arg) {
    if (!cur) return;

    win_size(cur->w, &(int){0}, &(int){0}, &ww, &wh);
    XMoveWindow(d, cur->w, (sw - ww) / 2, (sh - wh) / 2);
}

void win_fs(const Arg arg) {
    if (!cur) return;

    if ((cur->f = cur->f ? 0 : 1)) {
        win_size(cur->w, &cur->wx, &cur->wy, (unsigned int*) &cur->ww, (unsigned int*) &cur->wh);
        XMoveResizeWindow(d, cur->w, 0, 0, sw, sh);

    } else {
        XMoveResizeWindow(d, cur->w, cur->wx, cur->wy, cur->ww, cur->wh);
    }
}

void win_to_ws(const Arg arg) {
    int tmp = ws;

    if (arg.i == tmp) return;

    ws_sel(arg.i);
    win_add(cur->w, 1);
    ws_save(arg.i);

    ws_sel(tmp);
    win_del(cur->w);
    XUnmapWindow(d, cur->w);
    ws_save(tmp);

    if (list) win_focus(list);
}

void win_prev(const Arg arg) {
    if (!cur) return;

    XRaiseWindow(d, cur->prev->w);
    win_focus(cur->prev);
}

void win_next(const Arg arg) {
    if (!cur) return;

    XRaiseWindow(d, cur->next->w);
    win_focus(cur->next);
}

void ws_go(const Arg arg) {
    int tmp = ws;

    if (arg.i == ws) return;

    ws_save(ws);
    ws_sel(arg.i);

    for win XMapWindow(d, c->w);

    ws_sel(tmp);

    for win XUnmapWindow(d, c->w);

    ws_sel(arg.i);

    if (list) win_focus(list); else cur = 0;
}

void configure_request(XEvent *e) {
    XConfigureRequestEvent *ev = &e->xconfigurerequest;

    XConfigureWindow(d, ev->window, ev->value_mask, &(XWindowChanges) {
        .x          = ev->x,
        .y          = ev->y,
        .width      = ev->width,
        .height     = ev->height,
        .sibling    = ev->above,
        .stack_mode = ev->detail
    });
}

void map_request(XEvent *e) {
    Window w = e->xmaprequest.window;

    XSelectInput(d, w, StructureNotifyMask|EnterWindowMask);
    win_size(w, &wx, &wy, &ww, &wh);
    win_add(w, 1);
    cur = list->prev;

    if (wx + wy == 0) win_center((Arg){0});

    XMapWindow(d, w);
    win_focus(list->prev);
}

void run(const Arg arg) {
    if (fork()) return;
    if (d) close(ConnectionNumber(d));

    setsid();
    execvp((char*)arg.com[0], (char**)arg.com);
}

void input_grab(Window root) {
    unsigned int i, j, modifiers[] = {0, LockMask, numlock, numlock|LockMask};
    XModifierKeymap *modmap = XGetModifierMapping(d);
    KeyCode code;

    for (i = 0; i < 8; i++)
        for (int k = 0; k < modmap->max_keypermod; k++)
            if (modmap->modifiermap[i * modmap->max_keypermod + k]
                == XKeysymToKeycode(d, 0xff7f))
                numlock = (1 << i);

    for (i = 0; i < sizeof(keys)/sizeof(*keys); i++)
        if ((code = XKeysymToKeycode(d, keys[i].keysym)))
            for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
                XGrabKey(d, code, keys[i].mod | modifiers[j], root,
                        True, GrabModeAsync, GrabModeAsync);

    for (i = 1; i < 4; i += 2)
        for (j = 0; j < sizeof(modifiers)/sizeof(*modifiers); j++)
            XGrabButton(d, i, MOD | modifiers[j], root, True,
                ButtonPressMask|ButtonReleaseMask|PointerMotionMask,
                GrabModeAsync, GrabModeAsync, 0, 0);

    XFreeModifiermap(modmap);
}










void mover_pra_menu(const Arg arg) {

    estado_atual = Menu;
    XMoveWindow(d, janela_ir_menu, sw - 10, sh + 10);
    XMoveWindow(d, janela_ir_janela, 0, sh - 10);
    XMoveWindow(d, janela_ir_systray, sw/2, sh - 10);
    XMoveWindow(d, janela_voltar_systray, sw/2, -11);

    //printf("movendo pro menu...\n");
    int quanto_falta = sw;

    while(quanto_falta > 0) {
        quanto_falta--;
        for win {
            c->wx--;
            XMoveWindow(d, c->w, c->wx, c->wy);
        }
    }
    //printf("movido.\n");

}

void mover_pra_janela(const Arg arg) {

    estado_atual = Janela;
    XMoveWindow(d, janela_ir_menu, sw - 10, sh - 10);
    XMoveWindow(d, janela_ir_janela, 0, sh + 10);
    XMoveWindow(d, janela_ir_systray, sw/2, sh + 10);
    XMoveWindow(d, janela_voltar_systray, sw/2, -11);

    //printf("movendo pra janela...\n");
    int quanto_falta = sw;

    while(quanto_falta > 0) {
        quanto_falta--;
        for win {
            c->wx++;
            XMoveWindow(d, c->w, c->wx, c->wy);
        }
    }
    //printf("movido.\n");
}

void mover_pra_systray(const Arg arg) {

    estado_atual = Systray;
    XMoveWindow(d, janela_ir_menu, sw - 10, sh + 10);
    XMoveWindow(d, janela_ir_janela, 0, sh + 10);
    XMoveWindow(d, janela_ir_systray, sw/2, sh + 10);
    XMoveWindow(d, janela_voltar_systray, sw/2, 0);

    //printf("movendo pra systray...\n");
    int quanto_falta = sh;

    while(quanto_falta > 0) {
        quanto_falta--;
        for win {
            c->wy--;
            XMoveWindow(d, c->w, c->wx, c->wy);
        }
    }
    //printf("movido.\n");
}

void voltar_da_systray(const Arg arg) {

    estado_atual = Menu;
    XMoveWindow(d, janela_ir_menu, sw - 10, sh + 10);
    XMoveWindow(d, janela_ir_janela, 0, sh - 10);
    XMoveWindow(d, janela_ir_systray, sw/2, sh - 10);
    XMoveWindow(d, janela_voltar_systray, sw/2, -11);

    //printf("voltando da systray...\n");
    int quanto_falta = sh;

    while(quanto_falta > 0) {
        quanto_falta--;
        for win {
            c->wy++;
            XMoveWindow(d, c->w, c->wx, c->wy);
        }
    }
    //printf("movido.\n");
}






void criar_ida_menu(){
    // Janela ir menu
    janela_ir_menu = XCreateSimpleWindow(d, root, sw - 10, sh - 10, 10, 10, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, janela_ir_menu, PointerMotionMask);
    XMapWindow(d, janela_ir_menu);
}

void criar_ida_janela(){
    // Janela ir janela
    janela_ir_janela = XCreateSimpleWindow(d, root, 0, sh, 10, 10, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, janela_ir_janela, ExposureMask | KeyPressMask | PointerMotionMask);
    XMapWindow(d, janela_ir_janela);
}

void criar_ida_systray(){
    // Janela ir systray
    janela_ir_systray = XCreateSimpleWindow(d, root, sw/2, sh, 10, 10, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, janela_ir_systray, ExposureMask | KeyPressMask | PointerMotionMask);
    XMapWindow(d, janela_ir_systray);
}

void criar_volta_systray(){
    // Janela voltar systray
    janela_voltar_systray = XCreateSimpleWindow(d, root, sw/2, -11, 10, 10, 1, BlackPixel(d, s), WhitePixel(d, s));
    XSelectInput(d, janela_voltar_systray, ExposureMask | KeyPressMask | PointerMotionMask);
    XMapWindow(d, janela_voltar_systray);
}


void criar_janelas_movimento() {
    criar_ida_menu();
    criar_ida_janela();
    criar_ida_systray();
    criar_volta_systray();
}



int main(void) {

    if (!(d = XOpenDisplay(0))) exit(1);

    signal(SIGCHLD, SIG_IGN);
    XSetErrorHandler(xerror);

    s     = DefaultScreen(d);
    root  = RootWindow(d, s);
    sw    = XDisplayWidth(d, s);
    sh    = XDisplayHeight(d, s);

    XSelectInput(d,  root, SubstructureRedirectMask);
    XDefineCursor(d, root, XCreateFontCursor(d, 68));
    input_grab(root);

    criar_janelas_movimento();

    XEvent ev;
    while (1) {

        if(!XNextEvent(d, &ev)) {
            if (events[ev.type]) events[ev.type](&ev);
        }

        int win_x, win_y, root_x, root_y = 0;
        unsigned int mask = 0;
        Window child_win, root_win;
        XQueryPointer(d, root, &child_win, &root_win, &root_x, &root_y, &win_x, &win_y, &mask);

        Arg a;

        if(root_win == janela_ir_menu && estado_atual == Janela) {
            mover_pra_menu(a);
        }

        if(root_win ==  janela_ir_janela && estado_atual == Menu) {
            mover_pra_janela(a);
        }

        if(root_win ==  janela_ir_systray && estado_atual == Menu) {
            mover_pra_systray(a);
        }

        if(root_win ==  janela_voltar_systray && estado_atual == Systray) {
            voltar_da_systray(a);
        }


    }
}
