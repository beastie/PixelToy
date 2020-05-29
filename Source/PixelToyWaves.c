/*

PixelToyWaves.c

Full support for wave and spectrum interactivity

*/

#include "PixelToyWaves.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToySound.h"
#include "PixelToyFFT.h"

#define NUMBANDS 16

#define SV_SNDWAVE 0
#define SV_FREQBAR 1
#define SV_FREQBRICK 2
#define SV_FREQLINE 3

/*
struct PTWaveGen
{
    char        active;
    Str63        name;
    char        type;
    char        vertical;
    char        solid;
    char        horizflip;
    char        vertflip;
    short        thickness;
    short        spacing;
    short        brightness;
    short        action;
    short        actionVar[8];
    Rect        place;
    short        reserved1;
    short        reserved2;
}
*/

extern WindowRef gMainWindow;
extern Boolean gDone, gTwosComplement;
extern struct preferences prefs;
extern struct setinformation curSet;
extern PaletteHandle winPalette;
extern ModalFilterUPP DialogFilterProc;
extern CGrafPtr offWorld;
extern short gSysVersionMajor;

// Shared sound data
extern Boolean gCapableSoundIn;
extern long inDeviceBufSize;
extern short gNumInputChannels, gSampleBytes;
extern SPBPtr gSoundParm;
extern Ptr soundPtr[];

short curWave;
Rect waveListRect, wavePreviewRect;
short bar_heights[NUMBANDS];

/*******************************
 *  Local Function Prototypes  *
 *******************************/
void ShowWaveValues(DialogPtr theDialog, short i);
void GenerateWaveNames(void);
void HandleWavePreviewImageClick(DialogPtr theDialog, Rect *rect);
void PreviewWaves(Rect *drawRect);
void DrawWaveList(Rect *drawRect);
void SetWaveRect(short i, short x, short y, Rect *rect);
void DrawFrequency(short i, Rect *rect);
void DrawFrequencyLine(short i, Rect *rect);
void DrawWave(short i, Rect *rect);
void MaintainSpectrum(void);
static void calc_freq(short *dest, short *src);

#define ID_WSAVE 1
#define ID_WCANCEL 2
#define ID_WLIST 3
#define ID_WNEW 4
#define ID_WDELETE 5
#define ID_WDUPLICATE 6
#define ID_WTOFRONT 7
#define ID_WTOBACK 8
#define ID_WPREVIEW 9
#define ID_WTYPE 10
#define ID_WACTION 11
#define ID_WSOLID 12
#define ID_WROTATE90 13
#define ID_WHFLIP 14
#define ID_WVFLIP 15
#define ID_WBRIGHTNESS 16
#define ID_WTHICKNESS 17
#define ID_WSPACING 18

void WavesEditor(void) {
    struct setinformation backup;
    struct PTWaveGen waveTemp;
    ControlHandle ctrl;
    DialogPtr theDialog;
    Boolean storeDoodle, dialogDone;
    Point point;
    float xdiv, ydiv;
    Rect myRect;
    short stageWidth, stageHeight, i, thisOne, avail, temp, itemHit;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;

    storeDoodle = curSet.action[ACTDOODLE];
    curSet.action[ACTDOODLE] = false;
    StopAutoChanges();
    EnsureSystemPalette();
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    theDialog = GetNewDialog(DLG_WAVEEDITOR, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    if (prefs.windowPosition[WINWAVEOPTS].h != 0 || prefs.windowPosition[WINWAVEOPTS].v != 0)
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINWAVEOPTS].h, prefs.windowPosition[WINWAVEOPTS].v, false);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    ResetCursor();
    curWave = -1;
    for (i = MAXWAVES - 1; i >= 0; i--) {
        if (curSet.wave[i].active)
            curWave = i;
    }
    if (curWave == -1)
        curWave = CreateNewWave();
    // draw initial list
    GetDialogItem(theDialog, ID_WLIST, &temp, &ctrl, &waveListRect);
    GetDialogItem(theDialog, ID_WPREVIEW, &temp, &ctrl, &wavePreviewRect);
    xdiv = stageWidth / (float)(wavePreviewRect.right - wavePreviewRect.left);
    ydiv = stageHeight / (float)(wavePreviewRect.bottom - wavePreviewRect.top);
    if (ydiv > xdiv) {    // make narrower
        wavePreviewRect.right = wavePreviewRect.left + (stageWidth / ydiv);
    } else {    // make shorter
        wavePreviewRect.bottom = wavePreviewRect.top + (stageHeight / xdiv);
    }
    GenerateWaveNames();
    DrawWaveList(&waveListRect);
    PreviewWaves(&wavePreviewRect);
    ShowWaveValues(theDialog, curWave);
    SetSliderActions(theDialog, DLG_WAVEEDITOR);
    UpdateSliderDisplay(theDialog);

    SetDialogDefaultItem(theDialog, ID_WSAVE);
    dialogDone = false;
    ConfigureFilter(ID_WSAVE, ID_WCANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_WSAVE:
                dialogDone = true;
                curSet.actMisc2[ACTSWAVE] = 42;
                CurSetTouched();
                break;
            case ID_WCANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case ID_WNEW:
            case ID_WDUPLICATE:
                avail = 0;
                for (i = 0; i < MAXWAVES; i++) {
                    if (!curSet.wave[i].active)
                        avail++;
                }
                if (avail > 0) {
                    temp = curWave;    // remember original for duplicate
                    curWave = CreateNewWave();
                    if (avail < 2) {   // dim New button since we're all full
                        GetDialogItemAsControl(theDialog, ID_WNEW, &ctrl);
                        DeactivateControl(ctrl);
                        GetDialogItemAsControl(theDialog, ID_WDUPLICATE, &ctrl);
                        DeactivateControl(ctrl);
                    }
                    if (itemHit == ID_WDUPLICATE) {
                        curSet.wave[curWave] = curSet.wave[temp];
                        curSet.wave[curWave].name[++curSet.wave[curWave].name[0]] = '+';
                        if (!OptionIsPressed()) {
                            OffsetRect(&curSet.wave[curWave].place, 500, 500);
                        }
                    }
                    ShowWaveValues(theDialog, curWave);
                } else {
                    SysBeep(1);
                }
                break;
            case ID_WDELETE:
                avail = 0;
                for (i = 0; i < MAXWAVES; i++) {
                    if (curSet.wave[i].active)
                        avail++;
                }
                if (avail > 1) {
                    for (i = (curWave + 1); i < MAXWAVES; i++) {
                        curSet.wave[i - 1] = curSet.wave[i];
                    }
                    curSet.wave[MAXIMAGES - 1].active = false;
                    while (!curSet.wave[curWave].active) {
                        curWave--;
                        if (curWave < 0)
                            curWave = MAXWAVES - 1;
                    }
                    GetDialogItemAsControl(theDialog, ID_WNEW, &ctrl);
                    ActivateControl(ctrl);
                    GetDialogItemAsControl(theDialog, ID_WDUPLICATE, &ctrl);
                    ActivateControl(ctrl);
                    ShowWaveValues(theDialog, curWave);
                } else {
                    DoStandardAlert(kAlertStopAlert, 26);
                }
                break;
            case ID_WTOFRONT:
                waveTemp = curSet.wave[curWave];
                curSet.wave[curWave].active = false;
                // eliminate emptiness
                i = 0;
                temp = i;
                while (i < MAXWAVES) {
                    if (curSet.wave[i].active) {
                        if (i != temp) {
                            curSet.wave[temp] = curSet.wave[i];
                            curSet.wave[i].active = false;
                        }
                        temp++;
                    }
                    i++;
                }
                // stick item at end of list
                curSet.wave[temp] = waveTemp;
                curWave = temp;
                ShowWaveValues(theDialog, curWave);
                break;
            case ID_WTOBACK:
                waveTemp = curSet.wave[curWave];
                curSet.wave[curWave].active = false;
                // sort everything to end of list
                i = MAXWAVES - 1;
                temp = i;
                while (i >= 0) {
                    if (curSet.wave[i].active) {
                        if (i != temp) {
                            curSet.wave[temp] = curSet.wave[i];
                            curSet.wave[i].active = false;
                        }
                        temp--;
                    }
                    i--;
                }
                // stick item at beginning
                curSet.wave[0] = waveTemp;
                curWave = 0;
                // eliminate emptiness
                i = 0;
                temp = i;
                while (i < MAXWAVES) {
                    if (curSet.wave[i].active) {
                        if (i != temp) {
                            curSet.wave[temp] = curSet.wave[i];
                            curSet.wave[i].active = false;
                        }
                        temp++;
                    }
                    i++;
                }
                ShowWaveValues(theDialog, curWave);
                break;
            case ID_WPREVIEW: HandleWavePreviewImageClick(theDialog, &wavePreviewRect); break;
            case ID_WLIST:
                GetMouse(&point);
                temp = (point.v - waveListRect.top) / 15;
                avail = -1;
                thisOne = -1;
                for (i = 0; i < MAXWAVES; i++) {
                    if (curSet.wave[i].active) {
                        avail++;
                        if (temp == avail)
                            thisOne = i;
                    }
                }
                if (thisOne != curWave && thisOne != -1) {
                    curWave = thisOne;
                    ShowWaveValues(theDialog, curWave);
                }
                break;
            case ID_WROTATE90:    // Checkboxes
            case ID_WHFLIP:
            case ID_WVFLIP:
            case ID_WSOLID:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = !GetControlValue(ctrl);
                if (itemHit == ID_WROTATE90)
                    curSet.wave[curWave].vertical = temp;
                if (itemHit == ID_WSOLID)
                    curSet.wave[curWave].solid = temp;
                if (itemHit == ID_WHFLIP)
                    curSet.wave[curWave].horizflip = temp;
                if (itemHit == ID_WVFLIP)
                    curSet.wave[curWave].vertflip = temp;
                ShowWaveValues(theDialog, curWave);
                break;
            case ID_WTYPE:
            case ID_WACTION:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = GetControlValue(ctrl);
                if (itemHit == ID_WTYPE)
                    curSet.wave[curWave].type = temp - 1;
                if (itemHit == ID_WACTION)
                    curSet.wave[curWave].action = temp - 1;
                ShowWaveValues(theDialog, curWave);
                break;
        }
        GenerateWaveNames();
        DrawWaveList(&waveListRect);
        PreviewWaves(&wavePreviewRect);
        UpdateSliderDisplay(theDialog);
    }
    curSet.action[ACTDOODLE] = storeDoodle;
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINWAVEOPTS].h = myRect.left;
    prefs.windowPosition[WINWAVEOPTS].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINWAVEOPTS]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen)
        MyHideMenuBar();
}

void SetLiveWaveSliderValue(short item, short value) {
    short i;

    if (item == ID_WTHICKNESS) {
        curSet.wave[curWave].thickness = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXWAVES; i++)
                if (curSet.wave[i].active)
                    curSet.wave[i].thickness = value;
        }
    }

    if (item == ID_WBRIGHTNESS) {
        curSet.wave[curWave].brightness = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXWAVES; i++)
                if (curSet.wave[i].active)
                    curSet.wave[i].brightness = value;
        }
    }

    if (item == ID_WSPACING) {
        curSet.wave[curWave].spacing = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXWAVES; i++)
                if (curSet.wave[i].active)
                    curSet.wave[i].spacing = value;
        }
    }
}

void ShowWaveValues(DialogPtr theDialog, short i) {
    Boolean active;
    ControlHandle ctrl;

    GetDialogItemAsControl(theDialog, ID_WTYPE, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].type + 1);
    active = (curSet.wave[i].type > 0);

    GetDialogItemAsControl(theDialog, ID_WSPACING, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].spacing);
    if (active)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);

    GetDialogItemAsControl(theDialog, ID_WACTION, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].action + 1);

    GetDialogItemAsControl(theDialog, ID_WROTATE90, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].vertical);

    GetDialogItemAsControl(theDialog, ID_WHFLIP, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].horizflip);

    GetDialogItemAsControl(theDialog, ID_WVFLIP, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].vertflip);

    GetDialogItemAsControl(theDialog, ID_WSOLID, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].solid);
    active = !curSet.wave[i].solid;

    GetDialogItemAsControl(theDialog, ID_WTHICKNESS, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].thickness);
    if (active)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);

    GetDialogItemAsControl(theDialog, ID_WBRIGHTNESS, &ctrl);
    SetControlValue(ctrl, curSet.wave[i].brightness);
    UpdateSliderDisplay(theDialog);
}

void GenerateWaveNames(void) {
    Boolean nameGood, match;
    Str255 str, add;
    short i, j, numeral;

    // Clear all names
    for (i = 0; i < MAXWAVES; i++) {
        curSet.wave[i].name[0] = 0;
    }
    // Generate new names
    for (i = 0; i < MAXWAVES; i++) {
        if (curSet.wave[i].active) {
            // Make a descriptive name
            GetIndString(str, BASERES, 45 + curSet.wave[i].type);
            if (curSet.wave[i].vertical) {
                GetIndString(add, BASERES, 49);
                BlockMoveData(add + 1, str + str[0] + 1, add[0]);
                str[0] += add[0];
            }
            if (curSet.wave[i].solid) {
                GetIndString(add, BASERES, 52);
                BlockMoveData(add + 1, str + str[0] + 1, add[0]);
                str[0] += add[0];
            }
            // Add a # if this name already exists
            nameGood = false;
            numeral = 1;
            while (!nameGood) {
                match = false;
                for (j = 0; j < i; j++) {
                    if (curSet.wave[j].active && i != j && !match) {
                        if (EqualString(str, curSet.wave[j].name, false, false))
                            match = true;
                    }
                }
                if (!match) {
                    nameGood = true;
                } else {
                    numeral++;
                    if (numeral < 3) {
                        str[++str[0]] = ' ';
                    }    // add a space or
                    else {
                        str[0]--;
                        if (numeral > 10)
                            str[0]--;
                    }    // remove existing number
                    if (numeral > 9) {
                        str[++str[0]] = '1';
                        str[++str[0]] = '0' + (numeral - 10);
                    } else {
                        str[++str[0]] = '0' + numeral;
                    }
                }
            }
            // Definitely have a good name, use it
            BlockMoveData(str, curSet.wave[i].name, str[0] + 1);
        }
    }
}

char CreateNewWave(void) {
    int i = 0;
    while (curSet.wave[i].active)
        i++;
    // New wave definition start
    curSet.wave[i].active = true;
    curSet.wave[i].type = 0;        // plain soundwave
    curSet.wave[i].vertical = 0;    // horizontal
    curSet.wave[i].solid = 0;
    curSet.wave[i].spacing = 50;
    curSet.wave[i].horizflip = 0;
    curSet.wave[i].vertflip = 0;
    curSet.wave[i].thickness = 10;
    curSet.wave[i].brightness = 100;    // max brightness
    curSet.wave[i].action = 0;          // stationary
    int c;
    for (c = 0; c < 8; c++)
        curSet.wave[i].actionVar[c] = 0;
    SetRect(&curSet.wave[i].place, 200, 200, 16284, 16284);
    curSet.wave[i].reserved1 = 0;
    curSet.wave[i].reserved2 = 0;
    return i;
}

void HandleWavePreviewImageClick(DialogPtr theDialog, Rect *rect) {
    Boolean resize = false;
    short i, sel = -1, stageWidth, stageHeight;
    Point start, point;
    Rect bRect;
    float xm, ym, tf;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;
    GetMouse(&start);
    start.h -= rect->left;
    start.v -= rect->top;

    tf = (rect->right - rect->left);
    if (tf == 0)
        tf = .01;
    xm = 16384.0 / tf;
    if (xm == 0)
        xm = .01;
    tf = (rect->bottom - rect->top);
    if (tf == 0)
        tf = .01;
    ym = 16384.0 / tf;
    if (ym == 0)
        ym = .01;

    i = MAXWAVES - 1;
    while (i >= 0 && sel == -1) {   // first try for a resize box
        if (curSet.wave[i].active) {
            bRect = curSet.wave[i].place;
            bRect.right /= xm;
            bRect.bottom /= ym;
            bRect.left = bRect.right - 5;
            bRect.top = bRect.bottom - 5;
            if (PtInRect(start, &bRect)) {
                sel = i;
                resize = true;
            }
        }
        i--;
    }
    if (sel == -1) {   // they didn't click a resize box, try whole boxes
        i = MAXWAVES - 1;
        while (i >= 0 && sel == -1) {
            if (curSet.wave[i].active) {
                bRect = curSet.wave[i].place;
                bRect.left /= xm;
                bRect.right /= xm;
                bRect.top /= ym;
                bRect.bottom /= ym;
                if (PtInRect(start, &bRect)) {
                    sel = i;
                    resize = false;
                }
            }
            i--;
        }
    }
    if (sel == -1)
        return;
    if (sel != curWave) {
        curWave = sel;
        DrawWaveList(&waveListRect);
        ShowWaveValues(theDialog, curWave);
        PreviewWaves(&wavePreviewRect);
        UpdateSliderDisplay(theDialog);
    }
    bRect = curSet.wave[curWave].place;
    while (StillDown()) {
        GetMouse(&point);
        if (PtInRect(point, rect)) {
            point.h -= rect->left;
            point.v -= rect->top;
            curSet.wave[curWave].place = bRect;
            if (resize) {
                curSet.wave[curWave].place.right += ((point.h - start.h) * xm);
                if (curSet.wave[curWave].place.right < bRect.left + (5 * xm))
                    curSet.wave[curWave].place.right = bRect.left + 5 * xm;
                curSet.wave[curWave].place.bottom += ((point.v - start.v) * ym);
                if (curSet.wave[curWave].place.bottom < bRect.top + (5 * ym))
                    curSet.wave[curWave].place.bottom = bRect.top + 5 * ym;
            } else {
                OffsetRect(&curSet.wave[curWave].place, (point.h - start.h) * xm, (point.v - start.v) * ym);
            }
            PreviewWaves(&wavePreviewRect);
        }
    }
}

void PreviewWaves(Rect *drawRect) {
    Rect previewRect, bRect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short i, genevaFnum;
    float xm, ym, tf;
    RGBColor grey, darkGrey;
    Str32 str;

    previewRect.left = previewRect.top = 0;
    previewRect.right = (drawRect->right - drawRect->left);
    previewRect.bottom = (drawRect->bottom - drawRect->top);
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
    ForeColor(blackColor);
    PaintRect(&previewRect);
    grey.red = grey.green = grey.blue = 32767;
    darkGrey.red = darkGrey.green = darkGrey.blue = 16384;

    tf = (float)previewRect.right;
    if (tf == 0)
        tf = .01;
    xm = 16384.0 / tf;
    if (xm == 0)
        xm = .01;
    tf = (float)previewRect.bottom;
    if (tf == 0)
        tf = .01;
    ym = 16384.0 / tf;
    if (ym == 0)
        ym = .01;

    GetFNum("\pGeneva", &genevaFnum);
    TextFont(genevaFnum);
    TextSize(9);
    TextMode(srcOr);
    for (i = 0; i < MAXWAVES; i++) {
        if (curSet.wave[i].active) {
            if (i == curWave) {
                ForeColor(whiteColor);
            } else {
                RGBForeColor(&grey);
            }
            bRect = curSet.wave[i].place;
            bRect.left /= xm;
            bRect.top /= ym;
            bRect.right /= xm;
            bRect.bottom /= ym;
            BlockMoveData(curSet.wave[i].name, str, curSet.wave[i].name[0] + 1);
            while (StringWidth(str) > (bRect.right - bRect.left) - 2 && str[0] > 1) {
                str[--str[0]] = 0xC9; // É
            }
            if (str[0] > 1) {
                MoveTo(bRect.left + (bRect.right - bRect.left) / 2 - (StringWidth(str) / 2),
                       bRect.top + (bRect.bottom - bRect.top) / 2 + 3);
                DrawString(str);
            }
            FrameRect(&bRect);
            bRect.left = bRect.right - 5;
            bRect.top = bRect.bottom - 5;
            PaintRect(&bRect);
        }
    }
    ForeColor(blackColor);
    BackColor(whiteColor);
    FrameRect(&previewRect);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &previewRect, drawRect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void DrawWaveList(Rect *drawRect) {
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
    ForeColor(blackColor);
    BackColor(whiteColor);
    TextMode(srcOr);
    EraseRect(&listRect);
    y = 14;
    LMGetHiliteRGB(&highlightColor);
    if (highlightColor.red < 24576)
        highlightColor.red = 24576;
    if (highlightColor.green < 24576)
        highlightColor.green = 24576;
    if (highlightColor.blue < 24576)
        highlightColor.blue = 24576;
    for (c = 0; c < MAXWAVES; c++) {
        if (curSet.wave[c].active) {
            BlockMoveData(curSet.wave[c].name, displayStr, curSet.wave[c].name[0] + 1);
            while (StringWidth(displayStr) > listRect.right - 6) {
                displayStr[0]--;
                displayStr[displayStr[0]] = 0xC9; // É
            }
            MoveTo(4, y);
            DrawString(displayStr);
            if (curWave == c) {
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
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &listRect, drawRect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void SetWaveRect(short i, short x, short y, Rect *rect) {
    float f, useWidth, useHeight;

    useWidth = prefs.winxsize;
    if (useWidth == 0)
        useWidth = .01;
    useHeight = prefs.winysize;
    if (useHeight == 0)
        useHeight = .01;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    f = 16384.0 / useWidth;
    if (f == 0)
        f = .01;
    rect->left = x / f;
    rect->right = rect->left + (curSet.wave[i].place.right - curSet.wave[i].place.left) / f;

    f = 16384.0 / useHeight;
    if (f == 0)
        f = .01;
    rect->top = y / f;
    rect->bottom = rect->top + (curSet.wave[i].place.bottom - curSet.wave[i].place.top) / f;
}

#define SVSLIDESPEED 100

#define SVA_STATIONARY 0
#define SVA_VERTSLIDE 1
#define SVA_HORIZSLIDE 2
#define SVA_WANDER 3
#define SVA_RANDOMJUMP 4
#define SVA_MOUSE 5

void DoSoundVisuals(void) {
    Rect rect;
    Point point;
    short i, x, y, r;
    static short wX[MAXWAVES], wY[MAXWAVES], dx[MAXIMAGES], dy[MAXIMAGES];

    if (!gCapableSoundIn)
        return;
    WaitUntilRecordingComplete();
    if (!LockAllWorlds())
        return;
    SaveContext();
    SetGWorld(offWorld, nil);
    MaintainSpectrum();
    for (i = 0; i < MAXWAVES; i++) {
        if (curSet.wave[i].active) {
            // Determine current screen position (0-16384)
            switch (curSet.wave[i].action) {
                case SVA_STATIONARY:
                    dx[i] = 0;
                    dy[i] = 0;
                    SetWaveRect(i, curSet.wave[i].place.left, curSet.wave[i].place.top, &rect);
                    break;
                case SVA_RANDOMJUMP:
                    r = (curSet.wave[i].place.right - curSet.wave[i].place.left) / 2;
                    x = RandNum(-r, 16384 - r);
                    r = (curSet.wave[i].place.bottom - curSet.wave[i].place.top) / 2;
                    y = RandNum(-r, 16384 - r);
                    SetWaveRect(i, x, y, &rect);
                    break;
                case SVA_VERTSLIDE:
                    if (dx[i] != 42) {    // initialise
                        dx[i] = 42;
                        dy[i] = SVSLIDESPEED;
                        wY[i] = curSet.wave[i].place.top;
                    }
                    r = (curSet.wave[i].place.bottom - curSet.wave[i].place.top) / 2;
                    wY[i] += dy[i];
                    if (wY[i] > 16384 - r)
                        dy[i] = -SVSLIDESPEED;
                    if (wY[i] < -r)
                        dy[i] = SVSLIDESPEED;
                    SetWaveRect(i, curSet.wave[i].place.left, wY[i], &rect);
                    break;
                case SVA_HORIZSLIDE:
                    if (dy[i] != 42) {    // initialise
                        dy[i] = 42;
                        dx[i] = SVSLIDESPEED;
                        wX[i] = curSet.wave[i].place.left;
                    }
                    r = (curSet.wave[i].place.right - curSet.wave[i].place.left) / 2;
                    wX[i] += dx[i];
                    if (wX[i] > 16384 - r)
                        dx[i] = -SVSLIDESPEED;
                    if (wX[i] < -r)
                        dx[i] = SVSLIDESPEED;
                    SetWaveRect(i, wX[i], curSet.wave[i].place.top, &rect);
                    break;
                case SVA_WANDER:
                    if (dx[i] == 0 && dy[i] == 0) {    // initialise
                        dx[i] = RandVariance(500);
                        dy[i] = RandVariance(500);
                        wX[i] = curSet.wave[i].place.left;
                        wY[i] = curSet.wave[i].place.top;
                    }
                    wX[i] += dx[i];
                    wY[i] += dy[i];
                    if (curSet.wave[i].action == SVA_WANDER) {
                        dx[i] += RandVariance(50);
                        dy[i] += RandVariance(50);
                    }
                    x = curSet.wave[i].place.right - curSet.wave[i].place.left;
                    y = curSet.wave[i].place.bottom - curSet.wave[i].place.top;
                    if (curSet.wave[i].action == SVA_WANDER) {    // wander bumps,
                        if (wX[i] < -(x / 2))
                            dx[i] = SVSLIDESPEED;
                        if (wX[i] > (16384 - (x / 2)))
                            dx[i] = -SVSLIDESPEED;
                        if (wY[i] < -(y / 2))
                            dy[i] = SVSLIDESPEED;
                        if (wY[i] > (16384 - (y / 2)))
                            dy[i] = -SVSLIDESPEED;
                    }
                    while (dx[i] < 80 && dx[i] > -80) {
                        dx[i] = RandVariance(500);
                    }
                    SetWaveRect(i, wX[i], wY[i], &rect);
                    break;
                case SVA_MOUSE:
                    GetMouse(&point);
                    LocalToGlobal(&point);
                    x = point.h - prefs.windowPosition[WINMAIN].h;
                    y = point.v - prefs.windowPosition[WINMAIN].v;
                    if (prefs.lowQualityMode) {
                        x /= 2;
                        y /= 2;
                    }
                    if (prefs.highQualityMode) {
                        x *= 2;
                        y *= 2;
                    }
                    SetWaveRect(i, 0, 0, &rect);
                    x -= (rect.right / 2);
                    y -= (rect.bottom / 2);
                    OffsetRect(&rect, x, y);
                    break;
            }
            // Draw appropriate visual type
            switch (curSet.wave[i].type) {
                case SV_SNDWAVE: DrawWave(i, &rect); break;
                case SV_FREQBAR:
                case SV_FREQBRICK: DrawFrequency(i, &rect); break;
                case SV_FREQLINE: DrawFrequencyLine(i, &rect); break;
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
}

void DrawFrequency(short i, Rect *rect) {
    short b, swap, thick, sizer;
    Rect r;
    float xscale, yscale;
    RGBColor color;

    if (curSet.wave[i].vertical) {    // Vertical
        xscale = (float)(rect->right - rect->left) / (float)256;
        yscale = (float)(rect->bottom - rect->top) / (float)NUMBANDS;
        if (yscale == 0)
            yscale = .01;
        thick = ((float)(curSet.wave[i].spacing) * (float)(yscale / 100.0));
    } else {    // Horizontal
        xscale = (float)(rect->right - rect->left) / (float)NUMBANDS;
        yscale = (float)(rect->bottom - rect->top) / (float)256;
        if (xscale == 0)
            xscale = .01;
        thick = ((float)(curSet.wave[i].spacing) * (float)(xscale / 100.0));
    }

    sizer = curSet.wave[i].thickness / 2;
    if (prefs.lowQualityMode)
        sizer /= 4;
    if (!prefs.lowQualityMode && !prefs.highQualityMode)
        sizer /= 2;
    if (sizer < 1)
        sizer = 1;
    PenSize(sizer, sizer);
    if (curSet.wave[i].brightness == 100)
        Index2Color(255, &color);
    else
        Index2Color(curSet.wave[i].brightness * 2.55, &color);
    RGBForeColor(&color);
    for (b = 0; b < NUMBANDS; b++) {
        if (bar_heights[b] > 0) {
            if (curSet.wave[i].vertical) {   // Vertical
                switch (curSet.wave[i].type) {
                    case SV_FREQBAR:
                        r.left = 0;
                        r.right = bar_heights[b] * xscale;
                        break;
                    case SV_FREQBRICK:
                        r.left = bar_heights[b] * xscale;
                        r.right = r.left + (yscale - thick) / 2.0;
                        if (r.right <= r.left)
                            r.right = r.left + 1;
                        break;
                }
                r.top = b * yscale + (thick / 2.0);
                r.bottom = (b + 1) * yscale - (thick / 2.0);
                if (r.bottom <= r.top)
                    r.bottom = r.top + 1;
            } else {   // Horizontal
                switch (curSet.wave[i].type) {
                    case SV_FREQBAR:
                        r.bottom = rect->bottom - rect->top;
                        r.top = r.bottom - (bar_heights[b] * yscale);
                        break;
                    case SV_FREQBRICK:
                        r.top = (rect->bottom - rect->top) - (bar_heights[b] * yscale);
                        r.bottom = r.top + (xscale - thick) / 2.0;
                        if (r.bottom <= r.top)
                            r.bottom = r.top + 1;
                        break;
                }
                r.left = b * xscale + (thick / 2.0);
                r.right = (b + 1) * xscale - (thick / 2.0);
                if (r.right <= r.left)
                    r.right = r.left + 1;
            }
            // Vertical flip
            if (curSet.wave[i].vertflip) {
                swap = r.top;
                r.top = (rect->bottom - rect->top) - r.bottom;
                r.bottom = (rect->bottom - rect->top) - swap;
            }
            // Horizontal flip
            if (curSet.wave[i].horizflip) {
                swap = r.left;
                r.left = (rect->right - rect->left) - r.right;
                r.right = (rect->right - rect->left) - swap;
            }
            OffsetRect(&r, rect->left, rect->top);
            if (curSet.wave[i].solid)
                PaintRect(&r);
            else
                FrameRect(&r);
        }
    }
}

void DrawFrequencyLine(short i, Rect *rect) {
    short b, bx, by, x, y, sizer, halfSize = 0;
    float xscale, yscale;
    RGBColor color;
    RgnHandle rgn;

    if (curSet.wave[i].vertical) {    // Vertical
        xscale = (float)(rect->right - rect->left) / (float)256;
        yscale = (float)(rect->bottom - rect->top) / (float)(NUMBANDS - 1);
    } else {    // Horizontal
        xscale = (float)(rect->right - rect->left) / (float)(NUMBANDS - 1);
        yscale = (float)(rect->bottom - rect->top) / (float)256;
    }
    sizer = curSet.wave[i].thickness / 2;
    if (prefs.lowQualityMode)
        sizer /= 4;
    if (!prefs.lowQualityMode && !prefs.highQualityMode)
        sizer /= 2;
    if (sizer < 1)
        sizer = 1;

    if (curSet.wave[i].solid) {
        rgn = NewRgn();
        OpenRgn();
    }
    if (curSet.wave[i].brightness == 100)
        Index2Color(255, &color);
    else
        Index2Color(curSet.wave[i].brightness * 2.55, &color);
    RGBForeColor(&color);
    if (!curSet.wave[i].solid) {
        PenSize(sizer, sizer);
        halfSize = sizer / 2;
    }
    for (b = 0; b < NUMBANDS; b++) {
        if (curSet.wave[i].vertical) {   // Vertical
            x = bar_heights[b] * xscale;
            if (curSet.wave[i].horizflip)
                x = (rect->right - rect->left) - x;
            y = b * yscale;
            if (curSet.wave[i].vertflip)
                y = (rect->bottom - rect->top) - y;
        } else {   // Horizontal
            x = b * xscale;
            if (curSet.wave[i].horizflip)
                x = (rect->right - rect->left) - x;
            y = (rect->bottom - rect->top) - (bar_heights[b] * yscale);
            if (curSet.wave[i].vertflip)
                y = (rect->bottom - rect->top) - y;
        }
        x = rect->left + x - halfSize;
        y = rect->top + y - halfSize;
        if (b == 0) {
            bx = x;
            by = y;
            MoveTo(x, y);
        } else
            LineTo(x, y);
    }
    if (curSet.wave[i].solid) {
        if (curSet.wave[i].vertical) {
            if (curSet.wave[i].horizflip)
                x = rect->right;
            else
                x = rect->left;
            LineTo(x, y);
            if (curSet.wave[i].vertflip)
                y = rect->bottom;
            else
                y = rect->top;
            LineTo(x, y);
        } else {
            if (curSet.wave[i].vertflip)
                y = rect->top;
            else
                y = rect->bottom;
            LineTo(x, y);
            if (curSet.wave[i].horizflip)
                x = rect->right;
            else
                x = rect->left;
            LineTo(x, y);
        }
        LineTo(bx, by);
        CloseRgn(rgn);
        PaintRgn(rgn);
        DisposeRgn(rgn);
    }
}

#define SKIPBUFFER 256 + 16
void DrawWave(short i, Rect *rect) {
    float waveWidth = rect->right - rect->left;
    if (waveWidth < 0.01)
        waveWidth = 0.01;

    // Get SInt16 array of samples.
    UInt32 desiredSamples = ceilf(waveWidth);
    if (prefs.lowQualityMode)
        desiredSamples /= 2;
    else if (prefs.highQualityMode)
        desiredSamples *= 2;
    if (desiredSamples < 1)
        return;
    Ptr sampPtr = GetSoundInput16BitSamples(desiredSamples * 2);
    if (!sampPtr)
        return;
    UInt32 numSamples = GetPtrSize(sampPtr) / sizeof(SInt16);
    if (numSamples < desiredSamples) {
        fprintf(stderr, "Not enough sound samples!\n");
        DisposePtr(sampPtr);
        return;
    }
    SInt16 *theSamples = (SInt16 *)sampPtr;
    float waveHeight = rect->bottom - rect->top;
    if (waveHeight < 0.01)
        waveHeight = 0.01;
    float amp = 1.0 + (float)prefs.soundInputSoftwareGain / 25.0;

    RGBColor color;
    int penSize = curSet.wave[i].thickness / 2;
    if (prefs.lowQualityMode)
        penSize /= 4;
    else if (!prefs.highQualityMode)
        penSize /= 2;
    if (penSize < 1)
        penSize = 1;
    PenSize(penSize, penSize);
    int halfPenSize = penSize / 2;
    RgnHandle rgn = NULL;
    int rgnStartX = 0, rgnStartY = 0;
    if (curSet.wave[i].solid) {
        rgn = NewRgn();
        OpenRgn();
        halfPenSize = 0;
    }
    if (curSet.wave[i].brightness == 100)
        Index2Color(255, &color);
    else
        Index2Color(curSet.wave[i].brightness * 2.55, &color);
    RGBForeColor(&color);

    if (curSet.wave[i].vertical) {
        int halfWidth = waveWidth / 2;
        float scale = (32768.0 / waveWidth);
        if (scale < 0.01)
            scale = 0.01;

        int x, y;
        int ys;
        for (y = 0; y < waveHeight; y++) {
            SInt16 thisSample = theSamples[y];
            x = halfWidth + (((float)thisSample / scale) * amp);
            if (x < 0)
                x = 0;
            else if (x > waveWidth)
                x = waveWidth;
            ys = y;
            if (curSet.wave[i].vertflip)
                ys = waveHeight - y;
            if (y == 0) {
                rgnStartX = rect->left + x;
                rgnStartY = rect->top + ys;
                MoveTo(rgnStartX, rgnStartY);
            } else
                LineTo(rect->left + x, rect->top + ys);
        }

        if (curSet.wave[i].solid) {
            if (curSet.wave[i].horizflip)
                x = rect->left;
            else
                x = rect->right;
            LineTo(x, rect->top + ys);
            if (curSet.wave[i].vertflip)
                y = rect->bottom;
            else
                y = rect->top;
            LineTo(x, y);
            LineTo(rgnStartX, rgnStartY);
            CloseRgn(rgn);
            PaintRgn(rgn);
            DisposeRgn(rgn);
        }
    } else {   // Horizontal Wave
        int halfHeight = waveHeight / 2;
        float scale = (32768.0 / waveHeight);
        if (scale < 0.01)
            scale = 0.01;

        int x, y;
        int xs;
        for (x = 0; x < waveWidth; x++) {
            SInt16 thisSample = theSamples[x];
            y = halfHeight + (((float)thisSample / scale) * amp);
            if (y < 0)
                y = 0;
            else if (y > waveHeight)
                y = waveHeight;
            xs = x;
            if (curSet.wave[i].horizflip)
                xs = waveWidth - x;
            if (x == 0) {
                rgnStartX = rect->left + xs;
                rgnStartY = rect->top + y;
                MoveTo(rgnStartX, rgnStartY);
            } else
                LineTo(rect->left + xs, rect->top + y);
        }

        if (curSet.wave[i].solid) {
            if (curSet.wave[i].vertflip)
                y = rect->top;
            else
                y = rect->bottom;
            LineTo(rect->left + xs, y);
            if (curSet.wave[i].horizflip)
                x = rect->right;
            else
                x = rect->left;
            LineTo(x, y);
            LineTo(rgnStartX, rgnStartY);
            CloseRgn(rgn);
            PaintRgn(rgn);
            DisposeRgn(rgn);
        }
    }

    DisposePtr(sampPtr);
    Index2Color(255, &color);
    RGBForeColor(&color);
}

#define SPECDECAY 1
void MaintainSpectrum(void) {
    float scale, amp;
    short y, i, c, value, bufNum;
    short xscale[] = {0, 1, 2, 3, 5, 7, 10, 14, 20, 28, 40, 54, 74, 101, 137, 187, 255};
    static short dy[NUMBANDS];
    short forFFT[FFT_BUFFER_SIZE], fromFFT[FFT_BUFFER_SIZE];

    bufNum = 0;
    scale = (float)256 / log(256);
    amp = (float)prefs.soundInputSoftwareGain / 25.0;
    SInt16 *sampPtr = (SInt16 *)GetSoundInput16BitSamples(FFT_BUFFER_SIZE);
    if (!sampPtr)
        return;
    UInt32 numSamples = GetPtrSize((Ptr)sampPtr) / sizeof(SInt16);
    if (numSamples < FFT_BUFFER_SIZE) {
        free(sampPtr);
        return;
    }
    for (c = 0; c < FFT_BUFFER_SIZE; c++) {
        value = sampPtr[c];
        if (prefs.soundInputSoftwareGain)
            value *= amp;
        forFFT[c] = value / 128;
    }
    calc_freq(fromFFT, forFFT);
    for (i = 0; i < NUMBANDS; i++) {
        y = 0;
        for (c = xscale[i]; c < xscale[i + 1]; c++) {
            value = fromFFT[c];
            if (value > y)
                y = value;
        }
        if (y != 0) {
            y = (log(y) * scale);
            if (y > 255)
                y = 255;
        }

        if (y > bar_heights[i]) {
            bar_heights[i] = y;
            dy[i] = 0;
        } else {
            dy[i] += SPECDECAY;
            bar_heights[i] -= dy[i];
            if (bar_heights[i] < 0) {
                bar_heights[i] = 0;
                dy[i] = 0;
            }
        }
    }
    free(sampPtr);
    return;
}

static void calc_freq(short *dest, short *src) {
    static fft_state *state = NULL;
    float tmp_out[257];
    short i;

    if (!state)
        state = fft_init();

    fft_perform(src, tmp_out, state);

    for (i = 0; i < 256; i++)
        dest[i] = ((short)sqrt(tmp_out[i + 1])) >> 8;
}

