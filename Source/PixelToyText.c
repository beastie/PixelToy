// PixelToyText.c

#include "PixelToyText.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToySound.h"

extern CGrafPtr offWorld;    //, mainWorld;
//extern GDHandle                    mainDevice;
extern struct preferences prefs;
extern struct setinformation curSet;
extern ModalFilterUPP DialogFilterProc;
extern float curTextXLoc[MAXTEXTS], curTextYLoc[MAXTEXTS];
extern Boolean gDone;
extern PaletteHandle winPalette;

MenuHandle textFontMenu;
char curText;

/*******************************
 *  Local Function Prototypes  *
 *******************************/
void ShowTextValues(DialogPtr theDialog, char t);
void DrawTextList(Rect *drawRect);
void PreviewTextParameters(Rect *rect);

#define ID_TSAVE 1
#define ID_TCANCEL 2
#define ID_TBGROUP 3
#define ID_TSGROUP 4
#define ID_TNEW 5
#define ID_TDELETE 6
#define ID_TBACK 7
#define ID_TFRONT 8
#define ID_TLIST 9
#define ID_TPREVIEW 10
#define ID_TTEXT 11
#define ID_TFONT 12
#define ID_TACTION 13
#define ID_TBFIXED 14
#define ID_TBPULSE 15
#define ID_TBSOUND 16
#define ID_TBRIGHT 19
#define ID_TSIZE 18
#define ID_TSSOUND 17
#define ID_TDUPLICATE 21
#define ID_TOUTLINE 22

void PixelToyTextEditor(void) {
    Point mouse;
    Boolean dialogDone, storeDoodle /*, selected, shownAlert*/;
    ControlHandle ctrl;
    DialogPtr theDialog;
    Rect myRect, listRect, previewRect;
    Str255 str;
    float xdiv, ydiv, scaler;
    short xs, ys, offx, offy, avail, temp, itemHit, c, fontSize;
    short width, t, stageWidth, stageHeight;
    long number, lx, ly;
    struct PTTextGen tgTemp;
    struct setinformation backup;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;

    StopAutoChanges();
    storeDoodle = curSet.action[ACTDOODLE];
    curSet.action[ACTDOODLE] = false;
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_TEXTEDITOR, nil, (WindowPtr)-1);
    GetDialogItemAsControl(theDialog, ID_TFONT, &ctrl);
    textFontMenu = GetControlPopupMenuHandle(ctrl);
    AppendResMenu(textFontMenu, 'FOND');
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    if (prefs.windowPosition[WINTEXTOPTS].h != 0 || prefs.windowPosition[WINTEXTOPTS].v != 0)
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINTEXTOPTS].h, prefs.windowPosition[WINTEXTOPTS].v, false);
    SetPortDialogPort(theDialog);
    ResetCursor();

    curText = -1;
    avail = 0;
    for (c = MAXTEXTS - 1; c >= 0; c--) {
        if (curSet.textGen[c].behave > 0) {
            avail++;
            curText = c;
        }
    }
    if (avail > 15) {   // dim New button since we're all full
        GetDialogItemAsControl(theDialog, ID_TNEW, &ctrl);
        DeactivateControl(ctrl);
        GetDialogItemAsControl(theDialog, ID_TDUPLICATE, &ctrl);
        DeactivateControl(ctrl);
    }

    if (curText == -1)
        curText = CreateNewText(true);
    GetDialogItem(theDialog, ID_TLIST, &temp, &ctrl, &listRect);
    GetDialogItem(theDialog, ID_TPREVIEW, &temp, &ctrl, &previewRect);
    xdiv = stageWidth / (float)(previewRect.right - previewRect.left);
    ydiv = stageHeight / (float)(previewRect.bottom - previewRect.top);
    if (ydiv > xdiv) {    // make narrower
        previewRect.right = previewRect.left + (stageWidth / ydiv);
    } else {    // make shorter
        previewRect.top = previewRect.bottom - (stageHeight / xdiv);
    }

    ShowTextValues(theDialog, curText);
    SetSliderActions(theDialog, DLG_TEXTEDITOR);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    UpdateSliderDisplay(theDialog);

    DrawTextList(&listRect);
    PreviewTextParameters(&previewRect);

    SetDialogDefaultItem(theDialog, ID_TSAVE);
    dialogDone = false;
    ConfigureFilter(ID_TSAVE, ID_TCANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_TSAVE:
                dialogDone = true;
                CurSetTouched();
                break;
            case ID_TCANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case ID_TNEW:
            case ID_TDUPLICATE:
                avail = 0;
                for (c = 0; c < MAXTEXTS; c++) {
                    if (curSet.textGen[c].behave == 0)
                        avail++;
                }
                if (avail > 0) {
                    c = curText;    // remember original
                    curText = CreateNewText(true);
                    if (avail < 2) {   // dim New button since we're all full
                        GetDialogItemAsControl(theDialog, ID_TNEW, &ctrl);
                        DeactivateControl(ctrl);
                        GetDialogItemAsControl(theDialog, ID_TDUPLICATE, &ctrl);
                        DeactivateControl(ctrl);
                    }
                    if (itemHit == ID_TDUPLICATE) {
                        curSet.textGen[curText] = curSet.textGen[c];
                        if (!OptionIsPressed()) {
                            if (curSet.textGen[curText].xpos < 31000)
                                curSet.textGen[curText].xpos += 1000;
                            if (curSet.textGen[curText].ypos < 31000)
                                curSet.textGen[curText].ypos += 1000;
                        }
                    }
                } else {
                    SysBeep(1);
                }
                DrawTextList(&listRect);
                PreviewTextParameters(&previewRect);
                ShowTextValues(theDialog, curText);
                break;
            case ID_TDELETE:
                avail = 0;
                for (c = 0; c < MAXTEXTS; c++) {
                    if (curSet.textGen[c].behave > 0)
                        avail++;
                }
                if (avail > 1) {
                    for (t = (curText + 1); t < MAXTEXTS; t++) {
                        curSet.textGen[t - 1] = curSet.textGen[t];
                    }
                    curSet.textGen[MAXTEXTS - 1].behave = 0;
                    while (curSet.textGen[curText].behave == 0) {
                        curText--;
                        if (curText < 0)
                            curText = MAXTEXTS - 1;
                    }
                    DrawTextList(&listRect);
                    PreviewTextParameters(&previewRect);
                    ShowTextValues(theDialog, curText);
                    GetDialogItemAsControl(theDialog, ID_TNEW, &ctrl);
                    ActivateControl(ctrl);
                    GetDialogItemAsControl(theDialog, ID_TDUPLICATE, &ctrl);
                    ActivateControl(ctrl);
                } else {
                    DoStandardAlert(kAlertStopAlert, 7);
                    DrawTextList(&listRect);
                    PreviewTextParameters(&previewRect);
                }
                break;
            case ID_TFRONT:
                tgTemp = curSet.textGen[curText];
                curSet.textGen[curText].behave = 0;
                // eliminate emptiness
                t = 0;
                temp = t;
                while (t < MAXTEXTS) {
                    if (curSet.textGen[t].behave) {
                        if (t != temp)
                            curSet.textGen[temp] = curSet.textGen[t];
                        temp++;
                    }
                    t++;
                }
                // stick item at end of list
                curSet.textGen[temp] = tgTemp;
                curText = temp;
                DrawTextList(&listRect);
                PreviewTextParameters(&previewRect);
                break;
            case ID_TBACK:
                tgTemp = curSet.textGen[curText];
                curSet.textGen[curText].behave = 0;
                // sort everything to end of list
                t = MAXTEXTS - 1;
                temp = t;
                while (t >= 0) {
                    if (curSet.textGen[t].behave) {
                        if (t != temp) {
                            curSet.textGen[temp] = curSet.textGen[t];
                            curSet.textGen[t].behave = 0;
                        }
                        temp--;
                    }
                    t--;
                }
                // stick item at beginning of list
                curSet.textGen[0] = tgTemp;
                curText = 0;
                // now eliminate emptiness
                t = 0;
                temp = t;
                while (t < MAXTEXTS) {
                    if (curSet.textGen[t].behave) {
                        if (t != temp) {
                            curSet.textGen[temp] = curSet.textGen[t];
                            curSet.textGen[t].behave = 0;
                        }
                        temp++;
                    }
                    t++;
                }
                DrawTextList(&listRect);
                PreviewTextParameters(&previewRect);
                break;
            case ID_TLIST:
                GetMouse(&mouse);
                temp = (mouse.v - listRect.top) / 15;
                avail = -1;
                t = -1;
                for (c = 0; c < MAXTEXTS; c++) {
                    if (curSet.textGen[c].behave > 0) {
                        avail++;
                        if (temp == avail)
                            t = c;
                    }
                }
                if (t != curText && t != -1) {
                    curText = t;
                    ShowTextValues(theDialog, curText);
                }
                break;
            case ID_TTEXT:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 62)
                    str[0] = 62;
                BlockMoveData(str, curSet.textGen[curText].string, str[0] + 1);
                break;
            case ID_TFONT:    // Text font pop-up
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetMenuItemText(textFontMenu, GetControlValue(ctrl), str);
                if (str[0] > 62)
                    str[0] = 62;
                BlockMoveData(str, curSet.textGen[curText].fontName, str[0] + 1);
                GetFNum(str, &curSet.textGen[curText].fontID);
                break;
            case ID_TSIZE:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                StringToNum(str, &number);
                curSet.textGen[curText].size = (unsigned char)number;
                if (curSet.textGen[curText].size < 1)
                    curSet.textGen[curText].size = 1;
                if (curSet.textGen[curText].size > 200)
                    curSet.textGen[curText].size = 200;
                break;
            case ID_TOUTLINE:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = GetControlValue(ctrl);
                SetControlValue(ctrl, !temp);
                curSet.textGen[curText].reserved1 ^= 0x0001;
                break;
            case ID_TSSOUND:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = GetControlValue(ctrl);
                SetControlValue(ctrl, !temp);
                curSet.textGen[curText].sizeReactSound = !temp;
                GetDialogItemAsControl(theDialog, ID_TSIZE, &ctrl);
                break;
            case ID_TACTION:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                curSet.textGen[curText].behave = GetControlValue(ctrl);
                break;
            // Radio buttons
            case ID_TBFIXED:
            case ID_TBPULSE:
            case ID_TBSOUND:
                GetDialogItemAsControl(theDialog, ID_TBFIXED, &ctrl);
                SetControlValue(ctrl, (itemHit == ID_TBFIXED));
                GetDialogItemAsControl(theDialog, ID_TBPULSE, &ctrl);
                SetControlValue(ctrl, (itemHit == ID_TBPULSE));
                GetDialogItemAsControl(theDialog, ID_TBSOUND, &ctrl);
                SetControlValue(ctrl, (itemHit == ID_TBSOUND));
                if (itemHit == ID_TBFIXED) {
                    curSet.textGen[curText].brightPulse = curSet.textGen[curText].brightReactSound = false;
                }
                if (itemHit == ID_TBPULSE) {
                    curSet.textGen[curText].brightPulse = true;
                    curSet.textGen[curText].brightReactSound = false;
                }
                if (itemHit == ID_TBSOUND) {
                    curSet.textGen[curText].brightPulse = false;
                    curSet.textGen[curText].brightReactSound = true;
                }
                GetDialogItemAsControl(theDialog, ID_TBRIGHT, &ctrl);
                if (itemHit == ID_TBFIXED) {
                    ActivateControl(ctrl);
                } else {
                    DeactivateControl(ctrl);
                }
                UpdateSliderDisplay(theDialog);
                break;
            case ID_TPREVIEW:
                GetMouse(&mouse);
                mouse.h -= previewRect.left;
                mouse.v -= previewRect.top;
                temp = -1;
                for (c = 0; c < MAXTEXTS; c++) {
                    if (curSet.textGen[c].behave > 0) {
                        TextFont(curSet.textGen[c].fontID);
                        scaler = 100.0 / (previewRect.bottom - previewRect.top);
                        fontSize = curSet.textGen[c].size / scaler;
                        TextSize(fontSize);
                        scaler = 32768.0 / (previewRect.right - previewRect.left);
                        xs = (curSet.textGen[c].xpos / scaler);
                        scaler = 32768.0 / (previewRect.bottom - previewRect.top);
                        ys = (curSet.textGen[c].ypos / scaler);
                        width = StringWidth(curSet.textGen[c].string);
                        myRect.left = xs - (width / 2);
                        myRect.right = myRect.left + width;
                        myRect.bottom = ys + (fontSize * .15);
                        myRect.top = myRect.bottom - (fontSize * .9);
                        if (PtInRect(mouse, &myRect))
                            temp = c;
                    }
                }
                TextFont(0);
                TextSize(12);
                if (temp != -1) {
                    if (temp != curText) {
                        curText = temp;
                        DrawTextList(&listRect);
                        ShowTextValues(theDialog, curText);
                        PreviewTextParameters(&previewRect);
                    }
                    // stilldown draggage
                    scaler = 32768.0 / (previewRect.right - previewRect.left);
                    offx = curSet.textGen[curText].xpos - mouse.h * scaler;
                    scaler = 32768.0 / (previewRect.bottom - previewRect.top);
                    offy = curSet.textGen[curText].ypos - mouse.v * scaler;
                    while (StillDown()) {
                        GetMouse(&mouse);
                        mouse.h -= previewRect.left;
                        mouse.v -= previewRect.top;
                        scaler = 32768.0 / (previewRect.right - previewRect.left);
                        lx = mouse.h * scaler + offx;
                        scaler = 32768.0 / (previewRect.bottom - previewRect.top);
                        ly = mouse.v * scaler + offy;
                        if (lx < 0)
                            lx = 0;
                        if (lx > 32767)
                            lx = 32767;
                        if (ly < 0)
                            ly = 0;
                        if (ly > 32767)
                            ly = 32767;
                        curSet.textGen[curText].xpos = (short)lx;
                        curSet.textGen[curText].ypos = (short)ly;
                        PreviewTextParameters(&previewRect);
                    }
                }
                break;
            default: break;
        }
        DrawTextList(&listRect);
        PreviewTextParameters(&previewRect);
        UpdateCurrentSetNeedsSoundInputMessage();
    }
    curSet.action[ACTDOODLE] = storeDoodle;
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINTEXTOPTS].h = myRect.left;
    prefs.windowPosition[WINTEXTOPTS].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINTEXTOPTS]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen) {
        MyHideMenuBar();
    }
}

void SetLiveTextSliderValue(short item, short value) {
    short i;

    if (item == ID_TBRIGHT) {
        curSet.textGen[curText].brightness = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXTEXTS; i++)
                if (curSet.textGen[i].behave != 0)
                    curSet.textGen[i].brightness = value;
        }
    }
}

void ShowTextValues(DialogPtr theDialog, char t) {
    Boolean gotFont;
    short radio, numItems, c;
    ControlHandle ctrl;
    Str255 str;

    if (curSet.textGen[t].behave == 0)
        return;
    // set up text and select it
    GetDialogItemAsControl(theDialog, ID_TTEXT, &ctrl);
    SetDialogItemText((Handle)ctrl, curSet.textGen[t].string);
    SelectDialogItemText(theDialog, ID_TTEXT, 0, 32767);
    // set font menu
    numItems = CountMenuItems(textFontMenu);
    gotFont = false;
    c = 1;
    while (!gotFont && c <= numItems) {
        GetMenuItemText(textFontMenu, c, str);
        if (EqualString(curSet.textGen[t].fontName, str, false, false)) {
            GetDialogItemAsControl(theDialog, ID_TFONT, &ctrl);
            SetControlValue(ctrl, c);
            gotFont = true;
        } else {
            c++;
        }
    }
    // set size
    GetDialogItemAsControl(theDialog, ID_TSIZE, &ctrl);
    NumToString(curSet.textGen[t].size, str);
    SetDialogItemText((Handle)ctrl, str);
    // size react sound
    GetDialogItemAsControl(theDialog, ID_TSSOUND, &ctrl);
    SetControlValue(ctrl, curSet.textGen[t].sizeReactSound);
    // set brightness
    radio = ID_TBFIXED;
    if (curSet.textGen[t].brightPulse)
        radio = ID_TBPULSE;
    if (curSet.textGen[t].brightReactSound)
        radio = ID_TBSOUND;
    GetDialogItemAsControl(theDialog, ID_TBFIXED, &ctrl);
    SetControlValue(ctrl, (radio == ID_TBFIXED));
    GetDialogItemAsControl(theDialog, ID_TBPULSE, &ctrl);
    SetControlValue(ctrl, (radio == ID_TBPULSE));
    GetDialogItemAsControl(theDialog, ID_TBSOUND, &ctrl);
    SetControlValue(ctrl, (radio == ID_TBSOUND));
    GetDialogItemAsControl(theDialog, ID_TBRIGHT, &ctrl);
    SetControlValue(ctrl, curSet.textGen[t].brightness);
    GetDialogItemAsControl(theDialog, ID_TBRIGHT, &ctrl);
    if (radio == ID_TBFIXED) {
        ActivateControl(ctrl);
    } else {
        DeactivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_TOUTLINE, &ctrl);
    SetControlValue(ctrl, (curSet.textGen[t].reserved1 & 0x0001));
    // set action
    GetDialogItemAsControl(theDialog, ID_TACTION, &ctrl);
    SetControlValue(ctrl, curSet.textGen[t].behave);
    UpdateSliderDisplay(theDialog);
}

char CreateNewText(Boolean defaultText) {
    Str255 str;
    char avail = 0, c;

    for (c = 0; c < MAXTEXTS; c++) {
        if (curSet.textGen[c].behave != 0)
            avail++;
    }
    if (avail < MAXTEXTS) {
        c = 0;
        while (curSet.textGen[c].behave > 0)
            c++;
        if (defaultText || curSet.textGen[c].string[0] < 1) {
            GetIndString(str, BASERES, 4);
            BlockMoveData(str, curSet.textGen[c].string, str[0] + 1);
        }
        curSet.textGen[c].fontID = 0;    // default
        GetFontName(curSet.textGen[c].fontID, curSet.textGen[c].fontName);
        curSet.textGen[c].size = 10;
        curSet.textGen[c].sizeReactSound = false;
        curSet.textGen[c].xpos = 16384;
        curSet.textGen[c].ypos = 16384;
        curSet.textGen[c].brightness = 100;
        curSet.textGen[c].brightReactSound = false;
        curSet.textGen[c].brightPulse = false;
        curSet.textGen[c].behave = 1;    // stationary
        return c;
    }
    return 0;
}

void PreviewTextParameters(Rect *rect) {
    Rect previewRect, myRect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short t, width, xs, ys, fontSize;
    float scaler;
    RGBColor grey, darkGrey;

    previewRect.left = previewRect.top = 0;
    previewRect.right = (rect->right - rect->left);
    previewRect.bottom = (rect->bottom - rect->top);
    GetPort(&curPort);
    GetGWorld(&curWorld, &curDevice);
    error = NewGWorld(&previewWorld, 16, &previewRect, nil, nil, 0);
    if (error != noErr)
        StopError(33, error);
    if (!LockPixels(GetGWorldPixMap(previewWorld))) {
        DisposeGWorld(previewWorld);
        return;
    }
    SetGWorld(previewWorld, nil);
    BackColor(blackColor);
    EraseRect(&previewRect);
    grey.red = grey.green = grey.blue = 32767;
    darkGrey.red = darkGrey.green = darkGrey.blue = 16384;
    for (t = 0; t < MAXTEXTS; t++) {
        if (curSet.textGen[t].behave > 0) {
            TextFont(curSet.textGen[t].fontID);
            scaler = 100.0 / (rect->bottom - rect->top);
            fontSize = curSet.textGen[t].size / scaler;
            TextSize(fontSize);
            scaler = 32768.0 / (rect->right - rect->left);
            xs = (curSet.textGen[t].xpos / scaler);
            scaler = 32768.0 / (rect->bottom - rect->top);
            ys = (curSet.textGen[t].ypos / scaler);
            width = StringWidth(curSet.textGen[t].string);
            RGBForeColor(&darkGrey);
            myRect.left = xs - (width / 2);
            myRect.right = myRect.left + width;
            myRect.bottom = ys + (fontSize * .15);
            myRect.top = myRect.bottom - (fontSize * .9);
            FrameRect(&myRect);
            if (t == curText) {
                ForeColor(whiteColor);
            } else {
                RGBForeColor(&grey);
            }
            MoveTo(xs - (width / 2), ys);
            DrawString(curSet.textGen[t].string);
        }
    }
    ForeColor(blackColor);
    BackColor(whiteColor);
    FrameRect(&previewRect);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &previewRect, rect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void DrawTextList(Rect *drawRect) {
    Rect listRect, hiliteRect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short c, y;
    Str255 displayStr;
    RGBColor highlightColor;

    listRect.left = listRect.top = 0;
    listRect.right = (drawRect->right - drawRect->left);
    listRect.bottom = (drawRect->bottom - drawRect->top);
    GetPort(&curPort);
    GetGWorld(&curWorld, &curDevice);
    error = NewGWorld(&previewWorld, 16, &listRect, nil, nil, 0);
    if (error != noErr)
        StopError(33, error);
    if (!LockPixels(GetGWorldPixMap(previewWorld))) {
        DisposeGWorld(previewWorld);
        return;
    }
    SetGWorld(previewWorld, nil);
    BackColor(whiteColor);
    ForeColor(blackColor);
    TextMode(srcOr);
    EraseRect(&listRect);
    LMGetHiliteRGB(&highlightColor);
    if (highlightColor.red < 24576)
        highlightColor.red = 24576;
    if (highlightColor.green < 24576)
        highlightColor.green = 24576;
    if (highlightColor.blue < 24576)
        highlightColor.blue = 24576;
    y = 14;
    for (c = 0; c < MAXTEXTS; c++) {
        if (curSet.textGen[c].behave) {
            BlockMoveData(curSet.textGen[c].string, displayStr, curSet.textGen[c].string[0] + 1);
            while (StringWidth(displayStr) > listRect.right - 6) {
                displayStr[0]--;
                displayStr[displayStr[0]] = 0xC9; // É
            }
            MoveTo(4, y);
            DrawString(displayStr);
            if (curText == c) {
                hiliteRect.left = listRect.left + 2;
                hiliteRect.right = listRect.right - 2;
                hiliteRect.top = (y - 12);
                hiliteRect.bottom = hiliteRect.top + 15;
                RGBForeColor(&highlightColor);
                PenMode(adMin);
                PaintRect(&hiliteRect);
                ForeColor(blackColor);
                PenMode(srcCopy);
            }
            y += 15;
        }
    }
    FrameRect(&listRect);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &listRect, drawRect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

#define TB_STATIONARY 1
#define TB_JITTER 2
#define TB_ZEROG 3
#define TB_GRAVITY 4
#define TB_WANDER 5
#define TB_MOUSE 6
#define TB_VIBRATE 7

void DoText(void) {
    Point mouse;
    static float textdx[MAXTEXTS], textdy[MAXTEXTS];
    static short pulseIndex = 0;
    short c, index, topEdge, maxSpeed, halfWidth, width, fontSize, yKick;
    short sound, jitAmount, x, y, outlineOffset;
    float scaler;
    short useWidth, useHeight;

    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    if (!LockAllWorlds())
        return;
    SaveContext();
    SetGWorld(offWorld, nil);
    outlineOffset = PixelSize(1);
    for (c = 0; c < MAXTEXTS; c++) {
        if (curSet.textGen[c].behave) {
            sound = 0;
            TextFont(curSet.textGen[c].fontID);
            scaler = 100.0 / useHeight;
            fontSize = curSet.textGen[c].size / scaler;
            if (curSet.textGen[c].sizeReactSound) {
                index = RecentVolume();
                if (index > -1)
                    fontSize = (fontSize * index) / 128;
            }
            TextSize(fontSize);
            width = StringWidth(curSet.textGen[c].string);
            pulseIndex += 2;
            if (pulseIndex > 510)
                pulseIndex = 1;
            if (curSet.textGen[c].brightPulse) {
                index = pulseIndex;
                if (index > 254)
                    index = 511 - index;
            } else if (curSet.textGen[c].brightReactSound) {
                index = RecentVolume() * 2;
                if (index < 0 || index > 255)
                    index = 255;
            } else {
                index = (curSet.textGen[c].brightness * 2.56);
                if (index < 0 || index > 255)
                    index = 255;
            }
            halfWidth = width / 2;
            switch (curSet.textGen[c].behave) {
                case TB_STATIONARY:
                case TB_VIBRATE:
                case TB_JITTER:
                    scaler = 32768.0 / useWidth;
                    jitAmount = 200;
                    if (curSet.textGen[c].behave == TB_JITTER)
                        jitAmount = 500;
                    if (curSet.textGen[c].behave == TB_STATIONARY) {
                        curTextXLoc[c] = curSet.textGen[c].xpos / scaler;
                    } else {
                        curTextXLoc[c] = (curSet.textGen[c].xpos + RandVariance(jitAmount)) / scaler;
                    }

                    scaler = 32768.0 / useHeight;
                    if (curSet.textGen[c].behave == TB_STATIONARY) {
                        curTextYLoc[c] = curSet.textGen[c].ypos / scaler;
                    } else {
                        curTextYLoc[c] = (curSet.textGen[c].ypos + RandVariance(jitAmount)) / scaler;
                    }
                    textdx[c] = textdy[c] = 0;
                    break;
                case TB_ZEROG:
                case TB_GRAVITY:
                case TB_WANDER:
                    yKick = ((useHeight - 60) / 40) + 13;
                    topEdge = fontSize - (fontSize / 2);
                    maxSpeed = useHeight / 16;
                    if (textdx[c] == 0 && textdy[c] == 0) {
                        textdx[c] = PixelSize(RandVariance(maxSpeed));
                        textdy[c] = PixelSize(RandVariance(maxSpeed));
                    }
                    curTextXLoc[c] += textdx[c];
                    curTextYLoc[c] += textdy[c];
                    if (curSet.textGen[c].behave == TB_WANDER) {
                        textdx[c] += (RandNum(-3, 4) / 8.0);
                        textdy[c] += (RandNum(-3, 4) / 8.0);
                    }
                    if (curSet.textGen[c].behave == TB_GRAVITY)
                        textdy[c] += .2;
                    if (curSet.textGen[c].behave == TB_WANDER) {   // wander bumps,
                        if (curTextXLoc[c] < halfWidth) {
                            curTextXLoc[c] = halfWidth;
                            textdx[c] = PixelSize(1);
                        }
                        if (curTextYLoc[c] < topEdge) {
                            curTextYLoc[c] = topEdge;
                            textdy[c] = PixelSize(1);
                        }
                        if (curTextXLoc[c] > (useWidth - halfWidth)) {
                            curTextXLoc[c] = (useWidth - halfWidth);
                            textdx[c] = -PixelSize(1);
                        }
                        if (curTextYLoc[c] > useHeight) {
                            curTextYLoc[c] = useHeight;
                            textdy[c] = -PixelSize(1);
                        }
                    } else {   // everything else bounces
                        if (curTextXLoc[c] < halfWidth) {
                            curTextXLoc[c] = halfWidth;
                            textdx[c] = (-textdx[c]) - 1;
                        }
                        if (curTextYLoc[c] < topEdge) {
                            curTextYLoc[c] = topEdge;
                            textdy[c] = (-textdy[c]) - 2;
                            if (textdy[c] < 1)
                                textdy[c] = 1;
                        }
                        if (curTextXLoc[c] > (useWidth - halfWidth)) {
                            curTextXLoc[c] = (useWidth - halfWidth);
                            textdx[c] = (-textdx[c]) + 1;
                        }
                        if (curTextYLoc[c] > useHeight) {
                            curTextYLoc[c] = useHeight;
                            textdy[c] = -textdy[c] * .75;
                            if (curSet.textGen[c].behave == TB_GRAVITY && textdy[c] > -4)
                                textdy[c] = -RandNum(4, yKick);
                        }
                    }
                    while (textdx[c] < 2 && textdx[c] > -2)
                        textdx[c] = RandNum(-maxSpeed, maxSpeed);
                    // avoid vertical zero-g stagnation
                    if (curSet.textGen[c].behave == TB_ZEROG) {
                        while (textdy[c] < 2 && textdy[c] > -2)
                            textdy[c] = RandNum(-maxSpeed, maxSpeed);
                    }
                    if (textdx[c] > maxSpeed)
                        textdx[c] = maxSpeed;
                    if (textdy[c] > maxSpeed)
                        textdy[c] = maxSpeed;
                    if (textdx[c] < (-maxSpeed))
                        textdx[c] = -maxSpeed;
                    if (textdy[c] < (-maxSpeed))
                        textdy[c] = -maxSpeed;
                    break;
                case TB_MOUSE:
                    GetMouse(&mouse);
                    curTextXLoc[c] = mouse.h - prefs.windowPosition[WINMAIN].h;
                    curTextYLoc[c] = mouse.v - prefs.windowPosition[WINMAIN].v;
                    if (prefs.lowQualityMode) {
                        curTextXLoc[c] /= 2;
                        curTextYLoc[c] /= 2;
                    }
                    if (prefs.highQualityMode) {
                        curTextXLoc[c] *= 2;
                        curTextYLoc[c] *= 2;
                    }
                    break;
            }
            if (fontSize > 0) {
                x = curTextXLoc[c] - (width / 2);
                y = curTextYLoc[c];
                if (curSet.textGen[c].reserved1 & 0x0001) {   // draw black outline
                    MyDrawText(curSet.textGen[c].string, x - outlineOffset, y, 0);
                    MyDrawText(curSet.textGen[c].string, x + outlineOffset, y, 0);
                    MyDrawText(curSet.textGen[c].string, x, y - outlineOffset, 0);
                    MyDrawText(curSet.textGen[c].string, x, y + outlineOffset, 0);
                }
                MyDrawText(curSet.textGen[c].string, x, y, index);
                if (sound)
                    PlaySound(sound, true);
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
}

void ForceTextVisible(void *mem, short method) {   // method 0 = size down, method 1 = move
    Boolean doesntFit = true;
    Boolean left, right, top;
    float sizeScale, xScale, yScale;
    short stageWidth, stageHeight, halfWidth, height;
    long scaledHeight;

    struct PTTextGen *tg = mem;
    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 1;

    sizeScale = 100.0 / stageHeight;
    xScale = 32768.0 / stageWidth;
    yScale = 32768.0 / stageHeight;

    TextFont(tg->fontID);
    TextSize(tg->size / sizeScale);
    while (doesntFit) {
        doesntFit = left = right = top = false;
        halfWidth = StringWidth(tg->string) / 2;
        height = (tg->size / sizeScale) * .8;
        if ((halfWidth * xScale) > tg->xpos) {
            left = true;
            doesntFit = true;
        }
        if ((halfWidth * xScale) > 32768 - tg->xpos) {
            right = true;
            doesntFit = true;
        }
        if ((height * yScale) > tg->ypos && tg->ypos < 24000) {
            top = true;
            doesntFit = true;
        }
        if (doesntFit && (method == FTV_SIZEDOWN || (halfWidth * 2) >= stageWidth)) {
            tg->size -= 4;
            TextSize(tg->size / sizeScale);
        }
        if (doesntFit && method == FTV_MOVE) {
            if (top) {
                scaledHeight = (height * yScale) + 1;
                if (scaledHeight > 32700) {
                    tg->ypos = 32700;
                } else {
                    tg->ypos = scaledHeight;
                }
            }
            if (left)
                tg->xpos = (halfWidth * xScale) + 1;
            if (right)
                tg->xpos = 32767 - (halfWidth * xScale);
        }
    }
}
