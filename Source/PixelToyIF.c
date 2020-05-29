// PixelToyIF.c

#define PREFSVERSION 2

#include "PixelToyIF.h"
#import "CocoaBridge.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAbout.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyBitmap.h"
#include "PixelToyFilters.h"
#include "PixelToyPalette.h"
#include "PixelToyParticles.h"
#include "PixelToySets.h"
#include "PixelToySound.h"
#include "PixelToyText.h"
#include "PixelToyWaves.h"
#include "PixelToyWindows.h"
#include "EndianRemapping.h"

extern struct setinformation curSet;
extern struct preferences prefs;
extern ControlActionUPP SliderFeedbackProc;
extern ModalFilterUPP DialogFilterProc;
extern PaletteHandle winPalette;
extern RgnHandle gGrayRgn;
extern WindowPtr gMainWindow, gAboutWindow, gWindow[MAXWINDOW];
extern Boolean gDone, gFirstLaunch, gNoPalTrans, gQTExporting, gCapableSoundIn;
extern Boolean gSISupportsHWGain, gSIPlaythruChangeable, gSIAGCChangeable;
extern Rect gDragRect;
extern short gSysVersionMajor, gSysVersionMinor, palTransRemain;
extern short gBarHeight;
extern long maxlines, maxballs, maxbees, maxdrops, maxparticles;
extern long gSoundInRefNum;

static CFArrayRef possibleDisplaySettings = NULL;
CFDictionaryRef gSavedDisplayMode = NULL;
GDHandle windowDevice;
Boolean gSwitched = false;
short logVRefNum, curDialogID;
DialogPtr curDialog;
short gOrgColorDepth[16], logFRefNum = 0;
short filterOK, filterCancel;
Handle savedIconPositions;

const CFStringRef kCurrentSetKey = CFSTR("setData");
const CFStringRef kCurrentPrefsKey = CFSTR("prefData");

// Private methods
void SetResolutionsMenuPopup(ControlHandle ctrl);

#define FORCEMARGIN 22
void ForceOnScreen(WindowPtr win) {
    Rect wr, gr, ur, mr;
    short dist;
    BitMap qdbm;

    GetQDGlobalsScreenBits(&qdbm);
    mr = qdbm.bounds;
    GetWindowDeviceRect(win, &gr);
    GetGlobalWindowRect(win, &wr);

    if (EqualRect(&gr, &mr)) {   // on main screen
        if (wr.top < (gr.top + 35)) {   // must be completely below menu bar
            dist = (gr.top + 35) - wr.top;
            wr.top += dist;
            wr.bottom += dist;
            MoveWindow(win, wr.left, wr.top, false);
        }
    } else {   // on other screen, must be below top
        if (wr.top < gr.top) {   // must be completely below menu bar
            dist = gr.top - wr.top;
            wr.top += dist;
            wr.bottom += dist;
            MoveWindow(win, wr.left, wr.top, false);
        }
    }

    UnionRect(&wr, &gr, &ur);
    if (!EqualRect(&gr, &ur)) {
        if (wr.left > (gr.right - FORCEMARGIN)) {   // too far right
            dist = (wr.left - (gr.right - FORCEMARGIN));
            wr.left -= dist;
            wr.right -= dist;
        }
        if (wr.right < (gr.left + FORCEMARGIN)) {   // too far left
            dist = ((gr.left + FORCEMARGIN) - wr.right);
            wr.left += dist;
            wr.right += dist;
        }
        if (wr.top > (gr.bottom - FORCEMARGIN)) {   // too far down
            dist = (wr.top - (gr.bottom - FORCEMARGIN));
            wr.top -= dist;
            wr.bottom -= dist;
        }
        if (wr.top < (gr.top + FORCEMARGIN)) {   // too far up
            dist = (gr.top + FORCEMARGIN) - wr.top;
            wr.top += dist;
            wr.bottom += dist;
        }
        if (wr.left < gr.left) {   // too far left (fully onscreen)
            wr.right += (gr.left - wr.left) + 0;
            wr.left = gr.left + 0;
        }
        MoveWindow(win, wr.left, wr.top, false);
    }
}

void GetGlobalWindowRect(WindowPtr win, Rect *rect) {
    Point wp;
    Rect cRect;

    SaveContext();
    SetPortWindowPort(win);
    GetWindowPortBounds(win, &cRect);
    wp.h = cRect.left;
    wp.v = cRect.top;
    LocalToGlobal(&wp);
    rect->left = wp.h;
    rect->top = wp.v;
    wp.h = cRect.right;
    wp.v = cRect.bottom;
    LocalToGlobal(&wp);
    rect->right = wp.h;
    rect->bottom = wp.v;
    RestoreContext();
}

short GetWindowDevice(WindowPtr win) {   // returns color depth of this device
    Rect gr;
    Point wp;
    Boolean gotDevice = false;
    short i = 0;
    Rect wr;
    Boolean displayMgrPresent;
    long value = 0;
    CGrafPtr curWorld;

    Gestalt(gestaltDisplayMgrAttr, &value);
    displayMgrPresent = value & (1 << gestaltDisplayMgrPresent);

    if (displayMgrPresent) {
        GetGlobalWindowRect(win, &wr);
        // First step through all devices to find if any
        // corner is showing on one.
        windowDevice = DMGetFirstScreenDevice(dmOnlyActiveDisplays);
        while (windowDevice && i < 32 && !gotDevice) {
            gr = (*windowDevice)->gdRect;
            wp.h = wr.left;
            wp.v = wr.top;
            gotDevice = (PtInRect(wp, &gr));
            if (!gotDevice) {
                wp.h = wr.right;
                wp.v = wr.top;
                gotDevice = (PtInRect(wp, &gr));
            }
            if (!gotDevice) {
                wp.h = wr.left;
                wp.v = wr.bottom;
                gotDevice = (PtInRect(wp, &gr));
            }
            if (!gotDevice) {
                wp.h = wr.right;
                wp.v = wr.bottom;
                gotDevice = (PtInRect(wp, &gr));
            }
            if (!gotDevice) {   // no corner was on this device, so get next one.
                windowDevice = DMGetNextScreenDevice(windowDevice, dmOnlyActiveDisplays);
                i++;
            }
        }
        if (!gotDevice) {
            GetGWorld(&curWorld, &windowDevice);
        }
    } else {   // no display manager
        GetGWorld(&curWorld, &windowDevice);
    }
    return (*(*windowDevice)->gdPMap)->pixelSize;
}

void GetWindowDeviceRect(WindowPtr win, Rect *gr) {
    Point wp;
    Boolean gotDevice = false;
    short i = 0;
    Rect wr, cRect;
    GDHandle gd;
    Boolean displayMgrPresent;
    long value = 0;
    BitMap qdbm;

    GetQDGlobalsScreenBits(&qdbm);
    cRect = qdbm.bounds;
    Gestalt(gestaltDisplayMgrAttr, &value);
    displayMgrPresent = value & (1 << gestaltDisplayMgrPresent);

    if (displayMgrPresent) {
        GetGlobalWindowRect(win, &wr);
        // First step through all devices to find if any
        // corner is showing on one.
        gd = DMGetFirstScreenDevice(dmOnlyActiveDisplays);
        while (gd && i < 32 && !gotDevice) {
            *gr = (*gd)->gdRect;
            wp.h = wr.left + 1;
            wp.v = wr.top + 1;
            if (PtInRect(wp, gr))
                gotDevice = true;
            if (!gotDevice) {
                wp.h = wr.right - 1;
                wp.v = wr.top + 1;
                if (PtInRect(wp, gr))
                    gotDevice = true;
            }
            if (!gotDevice) {
                wp.h = wr.left + 1;
                wp.v = wr.bottom - 1;
                if (PtInRect(wp, gr))
                    gotDevice = true;
            }
            if (!gotDevice) {
                wp.h = wr.right - 1;
                wp.v = wr.bottom - 1;
                if (PtInRect(wp, gr))
                    gotDevice = true;
            }
            gd = DMGetNextScreenDevice(gd, dmOnlyActiveDisplays);
            i++;
        }
        if (!gotDevice) {
            gr->left = cRect.left;
            gr->top = cRect.top;
            gr->right = cRect.right;
            gr->bottom = cRect.bottom;
        }
    } else {   // no display manager
        gr->left = cRect.left;
        gr->top = cRect.top;
        gr->right = cRect.right;
        gr->bottom = cRect.bottom;
    }
}

#pragma mark -
#pragma mark __Preferences__

void GetDefaultPrefs(Boolean isFirstLaunch) {
    memset(&prefs, 0, sizeof(struct preferences));
    prefs.winxsize = 1024;
    prefs.winysize = 768;
    PTResetWindows();
    prefs.speedLimited = true;
    prefs.FPSlimit = 60;
    prefs.autoSetChange = isFirstLaunch;
    prefs.autoSetInterval = 20;
    prefs.notifyFontMissing = false;
    prefs.useFirstSet = true;
    prefs.showComments = true;
    prefs.palTransFrames = 40;
    prefs.exchange0and255 = (gSysVersionMajor == 9);
    prefs.soundInputDevice = -1;
    prefs.soundInputSource = -1;
    prefs.soundInputPlayThru = true;

    int effectiveWidth = prefs.winxsize;
    int effectiveHeight = prefs.winysize;
    if (prefs.lowQualityMode) {
        effectiveWidth *= 2;
        effectiveHeight *= 2;
    }
    if (prefs.highQualityMode) {
        effectiveWidth /= 2;
        effectiveHeight /= 2;
    }

    BitMap qdbm;
    GetQDGlobalsScreenBits(&qdbm);
    Rect cRect = qdbm.bounds;
    prefs.windowPosition[WINMAIN].h = ((cRect.right) / 2 - effectiveWidth / 2) + 20;
    prefs.windowPosition[WINMAIN].v = ((cRect.bottom) / 2 - effectiveHeight / 2) - 20;
}

void GetDefaultSet(void) {
    memset(&curSet, 0, sizeof(struct setinformation));
    curSet.action[ACTSWARM] = true;
    curSet.action[ACTDOODLE] = true;
    SetMiscOptionsDefaults();
    CreateNewText(true);
    CreateNewGenerator();
    CreateNewImage();
    GetIndString(curSet.windowTitle, BASERES, 5);
    curSet.filter[3] = true;    // blur/sink
#ifdef __i386__
    curSet.setComment[255] = 1;
#else
    curSet.setComment[255] = 0;
#endif
    SetDefaultCustomFilter();

    Handle colorTable = GetNamedResource('clut', "\pRainbow");
    LoadResource(colorTable);
    int c;
    if (colorTable != nil) {
        int index = 4;
        UInt16 *values = (UInt16 *)(*colorTable);
        for (c = 0; c < 256; c++) {
            index++;    // skip identifier UInt16
            curSet.palentry[c].red = values[index++];
            curSet.palentry[c].green = values[index++];
            curSet.palentry[c].blue = values[index++];
        }
        ReleaseResource(colorTable);
    } else
        RandomPalette(false);
}

void GetPrefs(void) {
    CFDataRef curPrefsData = CFPreferencesCopyAppValue(kCurrentPrefsKey, kCFPreferencesCurrentApplication);
    gFirstLaunch = (curPrefsData == NULL);
    if (curPrefsData && CFDataGetLength(curPrefsData) == sizeof(struct preferences)) {
        CFDataGetBytes(curPrefsData, CFRangeMake(0, sizeof(struct preferences)), (UInt8 *)&prefs);
    } else
        GetDefaultPrefs(gFirstLaunch);
    if (curPrefsData)
        CFRelease(curPrefsData);

    CFDataRef curSetData = CFPreferencesCopyAppValue(kCurrentSetKey, kCFPreferencesCurrentApplication);
    if (curSetData && CFDataGetLength(curSetData) == sizeof(struct setinformation)) {
        CFDataGetBytes(curSetData, CFRangeMake(0, sizeof(struct setinformation)), (UInt8 *)&curSet);
    } else
        GetDefaultSet();
    if (curSetData)
        CFRelease(curSetData);

    ValidateParticleGeneratorBrightnesses(&curSet);
    prefs.visiblePrefsTab = 0;
    prefs.autoSetChange = false;
    if (prefs.autoPilotValue[0] != AUTOPILOTVERSION)
        SetAutoPilotDefaults();
    SetCopyMode();
}

void PutPrefs(void) {
    CFDataRef curPrefsData = CFDataCreate(NULL, (UInt8 *)&prefs, sizeof(struct preferences));
    if (curPrefsData) {
        CFPreferencesSetAppValue(kCurrentPrefsKey, curPrefsData, kCFPreferencesCurrentApplication);
        CFRelease(curPrefsData);
    }

    CFDataRef curSetData = CFDataCreate(NULL, (UInt8 *)&curSet, sizeof(struct setinformation));
    if (curSetData) {
        CFPreferencesSetAppValue(kCurrentSetKey, curSetData, kCFPreferencesCurrentApplication);
        CFRelease(curSetData);
    }

    CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

#define ID_PR_SAVE 1
#define ID_PR_CANCEL 2
#define ID_PR_TABS 3
#define ID_PR_USEREZLIB 4
#define ID_PR_RESMENU 5
//#define ID_PR_COLORSMENU    6
#define ID_PR_LIMITED 7
#define ID_PR_FPSLIMIT 8
#define ID_PR_XSIZE 11
#define ID_PR_YSIZE 13
#define ID_PR_PALTRANS 15
#define ID_PR_NORESIZE 17
#define ID_PR_NOSPLASH 18
#define ID_PR_USEFIRSTSET 19
#define ID_PR_SHOWCOMMENTS 20
#define ID_PR_ROUGHCOLOR 21
#define ID_PR_HIDEMOUSE 22
#define ID_PR_NOTEFONTS 23
#define ID_PR_SHOWFPS 24
#define ID_PR_DEVICEPOPUP 26
#define ID_PR_SOURCEPOPUP 28
#define ID_PR_PLAYTHRU 29
#define ID_PR_AUTOGAIN 30
#define ID_PR_GAINSLIDER 35
#define ID_PR_HWGAINSLIDER 32

void PreferencesDialog(void) {
    Rect myRect;
    Boolean dialogDone, forceSoundInputMenu = false;
    DialogPtr theDialog;
    Point point;
    OSErr err;
    short i;
    short itemHit, temp, startwinx, startwiny, changedx, changedy;
    ControlHandle ctrl;
    long number;
    Str255 tempStr;
    struct preferences backup;

    SaveContext();
    StopAutoChanges();
    BlockMoveData(&prefs, &backup, sizeof(struct preferences));
    GetWindowPortBounds(gMainWindow, &myRect);
    changedx = startwinx = myRect.right;
    changedy = startwiny = myRect.bottom;
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_PREFERENCES, nil, (WindowPtr)-1);

    // Move Audio tab items up into position
    for (itemHit = 25; itemHit < 40; itemHit++) {
        GetDialogItemAsControl(theDialog, itemHit, &ctrl);
        GetControlBounds(ctrl, &myRect);
        MoveControl(ctrl, myRect.left, myRect.top - 400);
    }
    // Resize tab region to proper height
    GetDialogItemAsControl(theDialog, ID_PR_TABS, &ctrl);
    GetControlBounds(ctrl, &myRect);
    SizeControl(ctrl, myRect.right - myRect.left, (myRect.bottom - myRect.top) / 2);

    GetDialogItemAsControl(theDialog, ID_PR_DEVICEPOPUP, &ctrl);
    PrepareSoundInputDeviceMenu(ctrl);

    GetDialogItemAsControl(theDialog, ID_PR_SOURCEPOPUP, &ctrl);
    PrepareSoundInputSourceMenu(ctrl);

    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    if (prefs.windowPosition[WINPREFS].h != 0 || prefs.windowPosition[WINPREFS].v != 0) {
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINPREFS].h, prefs.windowPosition[WINPREFS].v, false);
    }

    SetPortDialogPort(theDialog);
    ResetCursor();

    GetDialogItemAsControl(theDialog, ID_PR_RESMENU, &ctrl);
    SetResolutionsMenuPopup(ctrl);
    if (!prefs.changeResolution)
        DeactivateControl(ctrl);

    // set up sliders
    GetDialogItemAsControl(theDialog, ID_PR_GAINSLIDER, &ctrl);
    SetControlValue(ctrl, prefs.soundInputSoftwareGain);

    GetDialogItemAsControl(theDialog, ID_PR_HWGAINSLIDER, &ctrl);
    SetControlValue(ctrl, prefs.soundInputHardwareGain + 50);

    // set up EditTexts
    NumToString(prefs.FPSlimit, tempStr);
    GetDialogItemAsControl(theDialog, ID_PR_FPSLIMIT, &ctrl);
    SetDialogItemText((Handle)ctrl, tempStr);
    SelectDialogItemText(theDialog, ID_PR_FPSLIMIT, 0, 32767);

    GetDialogItemAsControl(theDialog, ID_PR_PALTRANS, &ctrl);
    if (gNoPalTrans) {
        SetDialogItemText((Handle)ctrl, "\p0");
        DeactivateControl(ctrl);
    } else {
        NumToString(prefs.palTransFrames, tempStr);
        SetDialogItemText((Handle)ctrl, tempStr);
    }

    NumToString(startwinx, tempStr);
    GetDialogItemAsControl(theDialog, ID_PR_XSIZE, &ctrl);
    SetDialogItemText((Handle)ctrl, tempStr);

    NumToString(startwiny, tempStr);
    GetDialogItemAsControl(theDialog, ID_PR_YSIZE, &ctrl);
    SetDialogItemText((Handle)ctrl, tempStr);

    GetDialogItemAsControl(theDialog, ID_PR_USEREZLIB, &ctrl);
    SetControlValue(ctrl, prefs.changeResolution);
    GetDialogItemAsControl(theDialog, ID_PR_NOTEFONTS, &ctrl);
    SetControlValue(ctrl, prefs.notifyFontMissing);
    GetDialogItemAsControl(theDialog, ID_PR_NORESIZE, &ctrl);
    SetControlValue(ctrl, prefs.noImageResize);
    GetDialogItemAsControl(theDialog, ID_PR_LIMITED, &ctrl);
    SetControlValue(ctrl, prefs.speedLimited);
    GetDialogItemAsControl(theDialog, ID_PR_ROUGHCOLOR, &ctrl);
    SetControlValue(ctrl, prefs.roughPalette);
    GetDialogItemAsControl(theDialog, ID_PR_HIDEMOUSE, &ctrl);
    SetControlValue(ctrl, prefs.hideMouse);
    GetDialogItemAsControl(theDialog, ID_PR_NOSPLASH, &ctrl);
    SetControlValue(ctrl, prefs.noSplashScreen);
    GetDialogItemAsControl(theDialog, ID_PR_USEFIRSTSET, &ctrl);
    SetControlValue(ctrl, prefs.useFirstSet);
    GetDialogItemAsControl(theDialog, ID_PR_SHOWCOMMENTS, &ctrl);
    SetControlValue(ctrl, prefs.showComments);
    GetDialogItemAsControl(theDialog, ID_PR_SHOWFPS, &ctrl);
    SetControlValue(ctrl, prefs.showFPS);

    GetDialogItemAsControl(theDialog, ID_PR_TABS, &ctrl);
    if (prefs.visiblePrefsTab)
        SetControlValue(ctrl, prefs.visiblePrefsTab);
    PrefsTabDisplay(theDialog, ctrl);
    PrefsEnableDisableControls(theDialog);

    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_PR_SAVE);
    dialogDone = false;
    ConfigureFilter(ID_PR_SAVE, ID_PR_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_PR_SAVE:
                GetDialogItemAsControl(theDialog, ID_PR_XSIZE, &ctrl);
                GetDialogItemText((Handle)ctrl, tempStr);
                StringToNum(tempStr, &number);
                changedx = (short)number;
                GetDialogItemAsControl(theDialog, ID_PR_YSIZE, &ctrl);
                GetDialogItemText((Handle)ctrl, tempStr);
                StringToNum(tempStr, &number);
                changedy = (short)number;
                dialogDone = true;
                break;
            case ID_PR_CANCEL: {
                {
                    short currentSource = prefs.soundInputSource;
                    short currentDevice = prefs.soundInputDevice;
                    BlockMoveData(&backup, &prefs, sizeof(struct preferences));
                    if (prefs.soundInputSource != currentSource || prefs.soundInputDevice != currentDevice) {
                        CloseSoundIn();
                        OpenSoundIn(prefs.soundInputDevice);
                        MaintainSoundInput();
                        if (!gCapableSoundIn)
                            prefs.soundInputDevice = 0;
                        UpdateCurrentSetNeedsSoundInputMessage();
                    }
                    dialogDone = true;
                }
            } break;
            case ID_PR_TABS:
                GetDialogItemAsControl(theDialog, ID_PR_TABS, &ctrl);
                PrefsTabDisplay(theDialog, ctrl);
                PrefsEnableDisableControls(theDialog);
                break;
            // Non-live slider(s):
            case ID_PR_GAINSLIDER:
                GetDialogItemAsControl(theDialog, ID_PR_GAINSLIDER, &ctrl);
                prefs.soundInputSoftwareGain = GetControlValue(ctrl);
                break;
            case ID_PR_HWGAINSLIDER:
                GetDialogItemAsControl(theDialog, ID_PR_HWGAINSLIDER, &ctrl);
                prefs.soundInputHardwareGain = GetControlValue(ctrl) - 50;
                SetSoundInputGain(gSoundInRefNum, prefs.soundInputHardwareGain);
                break;
            // EditTexts
            case ID_PR_FPSLIMIT:
            case ID_PR_PALTRANS:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, tempStr);
                StringToNum(tempStr, &number);
                if (itemHit == ID_PR_FPSLIMIT) {
                    if (number != prefs.FPSlimit) {
                        prefs.FPSlimit = number;
                        if (prefs.FPSlimit > 60)
                            prefs.FPSlimit = 60;
                        if (prefs.FPSlimit < 1)
                            prefs.FPSlimit = 1;
                        UpdateAnimationTimer();
                    }
                }
                if (itemHit == ID_PR_PALTRANS) {
                    prefs.palTransFrames = number;
                    if (prefs.palTransFrames > 999)
                        prefs.palTransFrames = 999;
                    if (prefs.palTransFrames < 1)
                        prefs.palTransFrames = 1;
                    if (palTransRemain > prefs.palTransFrames)
                        palTransRemain = 1;
                    if (prefs.palTransFrames != number) {
                        NumToString(prefs.palTransFrames, tempStr);
                        SetDialogItemText((Handle)ctrl, tempStr);
                    }
                }
                break;
            // Sound Device popup
            case ID_PR_DEVICEPOPUP:
                GetDialogItemAsControl(theDialog, ID_PR_DEVICEPOPUP, &ctrl);
                temp = GetControlValue(ctrl) - 2;
                if (temp < 0)
                    temp = 0;
                if (temp != prefs.soundInputDevice) {
                    prefs.soundInputDevice = temp;
                    CloseSoundIn();
                    OpenSoundIn(prefs.soundInputDevice);
                    MaintainSoundInput();
                    if (!gCapableSoundIn) {
                        prefs.soundInputDevice = 0;
                        GetDialogItemAsControl(theDialog, ID_PR_DEVICEPOPUP, &ctrl);
                        PrepareSoundInputDeviceMenu(ctrl);
                    }
                    GetDialogItemAsControl(theDialog, ID_PR_SOURCEPOPUP, &ctrl);
                    PrepareSoundInputSourceMenu(ctrl);
                    Draw1Control(ctrl);
                    forceSoundInputMenu = true;
                    // no break, fall into source popup selector
                } else
                    break;
            // Sound Source popup
            case ID_PR_SOURCEPOPUP:
                GetDialogItemAsControl(theDialog, ID_PR_SOURCEPOPUP, &ctrl);
                temp = GetControlValue(ctrl);
                if (gCapableSoundIn && ((temp != prefs.soundInputSource) || forceSoundInputMenu)) {
                    prefs.soundInputSource = temp;
                    CloseSoundIn();
                    OpenSoundIn(prefs.soundInputDevice);
                    MaintainSoundInput();
                    if (!gCapableSoundIn) {
                        prefs.soundInputSource = 0;
                        PrepareSoundInputSourceMenu(ctrl);
                    }
                }
                PrefsEnableDisableControls(theDialog);
                UpdateCurrentSetNeedsSoundInputMessage();
                forceSoundInputMenu = false;
                break;
            // resolutions pop-up
            case ID_PR_RESMENU:
                GetDialogItemAsControl(theDialog, ID_PR_RESMENU, &ctrl);
                i = GetControlValue(ctrl);
                CFDictionaryRef newModeDict = CFArrayGetValueAtIndex(possibleDisplaySettings, i - 1);
                CFPreferencesSetAppValue(CFSTR("displayMode"), newModeDict, kCFPreferencesCurrentApplication);
                CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
                break;
            // Checkboxes
            case ID_PR_NORESIZE:
            case ID_PR_LIMITED:
            case ID_PR_ROUGHCOLOR:
            case ID_PR_NOTEFONTS:
            case ID_PR_HIDEMOUSE:
            case ID_PR_NOSPLASH:
            case ID_PR_USEFIRSTSET:
            case ID_PR_SHOWCOMMENTS:
            case ID_PR_USEREZLIB:
            case ID_PR_SHOWFPS:
            case ID_PR_PLAYTHRU:
            case ID_PR_AUTOGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = (GetControlValue(ctrl) == 0);
                SetControlValue(ctrl, temp);
                if (itemHit == ID_PR_PLAYTHRU) {    // try changing to new value but display result regardless
                    prefs.soundInputPlayThru = temp;
                    err = SPBSetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &temp);
                    err = SPBGetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &temp);
                    prefs.soundInputPlayThru = (temp > 0);
                    SetControlValue(ctrl, prefs.soundInputPlayThru);
                }
                if (itemHit == ID_PR_AUTOGAIN) {    // try changing to new value but display result regardless
                    prefs.soundInputAutoGain = temp;
                    err = SPBSetDeviceInfo(gSoundInRefNum, siAGCOnOff, &temp);
                    err = SPBGetDeviceInfo(gSoundInRefNum, siAGCOnOff, &temp);
                    prefs.soundInputAutoGain = (temp > 0);
                    SetControlValue(ctrl, prefs.soundInputAutoGain);
                }
                if (itemHit == ID_PR_USEREZLIB) {
                    prefs.changeResolution = temp;
                    GetDialogItemAsControl(theDialog, ID_PR_RESMENU, &ctrl);
                    if (temp)
                        ActivateControl(ctrl);
                    else
                        DeactivateControl(ctrl);
                }
                if (itemHit == ID_PR_NORESIZE)
                    prefs.noImageResize = temp;
                if (itemHit == ID_PR_LIMITED) {
                    prefs.speedLimited = temp;
                    UpdateAnimationTimer();
                }
                if (itemHit == ID_PR_ROUGHCOLOR)
                    prefs.roughPalette = temp;
                if (itemHit == ID_PR_NOTEFONTS)
                    prefs.notifyFontMissing = temp;
                if (itemHit == ID_PR_HIDEMOUSE)
                    prefs.hideMouse = temp;
                if (itemHit == ID_PR_NOSPLASH)
                    prefs.noSplashScreen = temp;
                if (itemHit == ID_PR_USEFIRSTSET)
                    prefs.useFirstSet = temp;
                if (itemHit == ID_PR_SHOWCOMMENTS)
                    prefs.showComments = temp;
                if (itemHit == ID_PR_SHOWFPS) {
                    prefs.showFPS = temp;
                    if (!prefs.showFPS)
                        MySetWindowTitle();
                }
                if (itemHit == ID_PR_LIMITED) {
                    GetDialogItemAsControl(theDialog, ID_PR_FPSLIMIT, &ctrl);
                    if (prefs.speedLimited) {
                        ActivateControl(ctrl);
                    } else {
                        DeactivateControl(ctrl);
                    }
                }
                break;
            default: break;
        }
    }
    MySetWindowTitle();
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    point.h = myRect.left;
    point.v = myRect.top;
    LocalToGlobal(&point);
    prefs.windowPosition[WINPREFS].h = point.h;
    prefs.windowPosition[WINPREFS].v = point.v;
    GetDialogItemAsControl(theDialog, ID_PR_TABS, &ctrl);
    prefs.visiblePrefsTab = GetControlValue(ctrl);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    if ((changedx != startwinx) || (changedy != startwiny)) {
        long memNeeded, memCurrent;
        memCurrent = MemoryRequiredCurrent();
        memNeeded = MemoryRequiredWithValues(changedx, changedy, prefs.lowQualityMode, prefs.highQualityMode);
        if (CanAdjustWorlds(memNeeded - memCurrent)) {
            StopAnimationTimer();
            if (prefs.lowQualityMode) {
                changedx /= 2;
                changedy /= 2;
            }
            if (prefs.highQualityMode) {
                changedx *= 2;
                changedy *= 2;
            }
            prefs.winxsize = changedx;
            prefs.winysize = changedy;
            MakeWindow(true);
            if (!SetUpWorlds()) {
                if (prefs.lowQualityMode) {
                    startwinx /= 2;
                    startwinx /= 2;
                }
                if (prefs.highQualityMode) {
                    startwinx *= 2;
                    startwinx *= 2;
                }
                prefs.winxsize = startwinx;
                prefs.winysize = startwiny;
                MakeWindow(true);
                if (!SetUpWorlds())
                    StopError(13, -108);
            } else if (prefs.dragSizeAllWindows)
                MagneticWindows(startwinx, startwiny);
            UpdateAnimationTimer();
        }
    }
    ResumeAutoChanges();
    PutPrefs();    // Unneccesary unless PixelToy crashes
    if (prefs.fullScreen)
        MyHideMenuBar();
}

// Show the items appropriate for the current tab
void PrefsTabDisplay(DialogPtr theDialog, ControlHandle tabControl) {
    Rect aRect;
    ControlHandle aControl;
    int tabNumber = GetControlValue(tabControl);
    int pass, i;

    for (pass = 0; pass <= 1; pass++) {
        for (i = 4; i < 40; i++) {
            Boolean belongsOnThisTab = ((i < 25) == (tabNumber == 1));
            GetDialogItemAsControl(theDialog, i, &aControl);
            GetControlBounds(aControl, &aRect);
            if (!belongsOnThisTab && pass == 0) {
                HideControl(aControl);
                DeactivateControl(aControl);
                if (aRect.left < 2000)
                    MoveControl(aControl, aRect.left + 2048, aRect.top);
            }
            if (belongsOnThisTab && pass == 1) {
                ShowControl(aControl);
                ActivateControl(aControl);
                if (aRect.left > 2000)
                    MoveControl(aControl, aRect.left - 2048, aRect.top);
            }
        }
    }
}

void PrefsEnableDisableControls(DialogPtr theDialog) {
    short temp;
    ControlHandle ctrl;
    OSErr err;

    if (prefs.fullScreen || gQTExporting) {   // disable window size editing
        GetDialogItemAsControl(theDialog, ID_PR_XSIZE, &ctrl);
        DeactivateControl(ctrl);
        GetDialogItemAsControl(theDialog, ID_PR_YSIZE, &ctrl);
        DeactivateControl(ctrl);
    }

    if (!prefs.speedLimited) {
        GetDialogItemAsControl(theDialog, ID_PR_FPSLIMIT, &ctrl);
        DeactivateControl(ctrl);
    }

    // Playthru checkbox
    GetDialogItemAsControl(theDialog, ID_PR_PLAYTHRU, &ctrl);
    if (gCapableSoundIn) {
        err = SPBGetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &temp);
        SetControlValue(ctrl, temp);
    }
    if (gCapableSoundIn && gSIPlaythruChangeable)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);

    // Automatic gain control
    GetDialogItemAsControl(theDialog, ID_PR_AUTOGAIN, &ctrl);
    if (gCapableSoundIn) {
        err = SPBGetDeviceInfo(gSoundInRefNum, siAGCOnOff, &temp);
        SetControlValue(ctrl, temp);
    }
    if (gCapableSoundIn && gSIAGCChangeable)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);

    // Hardware gain control
    GetDialogItemAsControl(theDialog, ID_PR_HWGAINSLIDER, &ctrl);
    if (gCapableSoundIn && gSISupportsHWGain)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);

    // Full-screen resolution/color pop-ups
    if (!prefs.changeResolution) {
        GetDialogItemAsControl(theDialog, ID_PR_RESMENU, &ctrl);
        DeactivateControl(ctrl);
    }
}

CGDirectDisplayID DisplayIDForMainWindowScreen(void) {
    Rect myRect;
    GetWindowPortBounds(gMainWindow, &myRect);
    OffsetRect(&myRect, prefs.windowPosition[WINMAIN].h, prefs.windowPosition[WINMAIN].v);
    CGPoint windowUpperLeft;
    windowUpperLeft.x = (float)myRect.left;
    windowUpperLeft.y = (float)myRect.top;

    // Get display ID for that point
    CGDirectDisplayID dispID;
    CGDisplayCount numDisps;
    CGDisplayErr dispErr = CGGetDisplaysWithPoint(windowUpperLeft, 1, &dispID, &numDisps);
    if (dispErr != noErr || numDisps < 1)
        return 0;
    return dispID;
}

// sort rlinfo array, then fill in mon resource list whilst populating the pop-up
void SetResolutionsMenuPopup(ControlHandle ctrl /*, struct monitorcontextinfo* mon*/) {
    Boolean errorHappened = false;
    CGDirectDisplayID dispID = DisplayIDForMainWindowScreen();
    CFArrayRef modesArray = nil;
    if (dispID) {    // Get all modes for this display
        modesArray = CGDisplayAvailableModes(dispID);
        errorHappened = (!modesArray);
    }

    MenuRef resMenu = GetControlPopupMenuHandle(ctrl);
    int count = CountMenuItems(resMenu);
    int i;
    for (i = 1; i <= count; i++) {
        DeleteMenuItem(resMenu, 1);
    }
    SetControlMaximum(ctrl, 32767);
    if (errorHappened) {
        AppendMenu(resMenu, "\pError");
        DisableMenuItem(resMenu, CountMenuItems(resMenu) - 1);
        return;
    }

    CFArrayRef displayModeStrings, displayModes;
    GetDisplayModesForDisplay(dispID, &displayModeStrings, &displayModes);

    if (possibleDisplaySettings)
        CFRelease(possibleDisplaySettings);
    possibleDisplaySettings = displayModes;
    CFRetain(possibleDisplaySettings);

    count = CFArrayGetCount(displayModeStrings);
    int markEntry = 1;
    CFDictionaryRef existingSetting = CFPreferencesCopyAppValue(CFSTR("displayMode"), kCFPreferencesCurrentApplication);
    for (i = 0; i < count; i++) {
        CFStringRef aString = CFArrayGetValueAtIndex(displayModeStrings, i);
        CFDictionaryRef aModeDict = CFArrayGetValueAtIndex(displayModes, i);
        if (existingSetting && CFEqual(existingSetting, aModeDict))
            markEntry = i + 1;
        AppendMenuItemTextWithCFString(resMenu, aString, 0, 0, NULL);
    }
    if (existingSetting)
        CFRelease(existingSetting);
    CFRelease(displayModes);
    SetControlValue(ctrl, markEntry);
}

#pragma mark -

OSErr SwitchInResolution(void) {
    CGError resChangeErr;
    CGDirectDisplayID dispID = DisplayIDForMainWindowScreen();
    gSavedDisplayMode = CGDisplayCurrentMode(dispID);
    CFArrayRef displayModeStrings, displayModes;
    GetDisplayModesForDisplay(dispID, &displayModeStrings, &displayModes);
    CFDictionaryRef destMode = CFPreferencesCopyAppValue(CFSTR("displayMode"), kCFPreferencesCurrentApplication);
    if (!destMode)
        destMode = CFArrayGetValueAtIndex(displayModes, 0);
    destMode = MatchingModeForDisplay(destMode, displayModes);
    if (destMode) {
        // Change the resolution
        CGDisplayConfigRef cref;
        resChangeErr = CGBeginDisplayConfiguration(&cref);
        if (resChangeErr == kCGErrorSuccess) {
            resChangeErr = CGConfigureDisplayMode(cref, dispID, destMode);
            resChangeErr = CGCompleteDisplayConfiguration(cref, kCGConfigureForAppOnly);
        }
    }
    return resChangeErr;
}

OSErr SwitchOutResolution(void) {
    CGDirectDisplayID dispID = DisplayIDForMainWindowScreen();
    CGDisplayConfigRef cref;
    CGError err = CGBeginDisplayConfiguration(&cref);
    if (err == kCGErrorSuccess) {
        err = CGConfigureDisplayMode(cref, dispID, gSavedDisplayMode);
        err = CGCompleteDisplayConfiguration(cref, kCGConfigureForAppOnly);
    }
    return err;
}

void SetLivePrefsSliderValue(short item, short value) {
    if (item == ID_PR_GAINSLIDER)
        prefs.soundInputSoftwareGain = value;
}

#define ID_QO_SAVE 1
#define ID_QO_CANCEL 2
#define ID_QO_FIXED 3
#define ID_QO_REAL 4
#define ID_QO_AUDIO 5
Boolean QuickTimeOptionsDialog(void) {
    Boolean dialogDone, storeDoodle, returnVal;
    DialogPtr theDialog;
    short itemHit;
    ControlHandle ctrl;

    SaveContext();
    StopAutoChanges();
    storeDoodle = curSet.action[ACTDOODLE];
    curSet.action[ACTDOODLE] = false;
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_QUICKTIMEOPTS, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    ResetCursor();
    // initialize defaults (kinda bad to do it here, but backward compatible)
    if (prefs.QTMode == 0) {
        prefs.QTMode = 1;    // fixed rate
        prefs.QTIncludeAudio = false;
    }
    // set display
    GetDialogItemAsControl(theDialog, ID_QO_FIXED, &ctrl);
    SetControlValue(ctrl, (prefs.QTMode == 1));
    GetDialogItemAsControl(theDialog, ID_QO_REAL, &ctrl);
    SetControlValue(ctrl, (prefs.QTMode == 2));
    GetDialogItemAsControl(theDialog, ID_QO_AUDIO, &ctrl);
    SetControlValue(ctrl, prefs.QTIncludeAudio);
    if (prefs.QTMode != 2)
        DeactivateControl(ctrl);

    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_QO_SAVE);
    dialogDone = false;
    ConfigureFilter(ID_QO_SAVE, ID_QO_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_QO_FIXED:
                GetDialogItemAsControl(theDialog, ID_QO_FIXED, &ctrl);
                SetControlValue(ctrl, true);
                GetDialogItemAsControl(theDialog, ID_QO_REAL, &ctrl);
                SetControlValue(ctrl, false);
                GetDialogItemAsControl(theDialog, ID_QO_AUDIO, &ctrl);
                DeactivateControl(ctrl);
                prefs.QTMode = 1;
                break;
            case ID_QO_REAL:
                GetDialogItemAsControl(theDialog, ID_QO_REAL, &ctrl);
                SetControlValue(ctrl, true);
                GetDialogItemAsControl(theDialog, ID_QO_FIXED, &ctrl);
                SetControlValue(ctrl, false);
                GetDialogItemAsControl(theDialog, ID_QO_AUDIO, &ctrl);
                ActivateControl(ctrl);
                prefs.QTMode = 2;
                break;
            case ID_QO_AUDIO:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                SetControlValue(ctrl, !GetControlValue(ctrl));
                break;
            case ID_QO_SAVE:
                GetDialogItemAsControl(theDialog, ID_QO_REAL, &ctrl);
                prefs.QTMode = 1 + GetControlValue(ctrl);
                GetDialogItemAsControl(theDialog, ID_QO_AUDIO, &ctrl);
                prefs.QTIncludeAudio = GetControlValue(ctrl);
                if (prefs.QTMode == 1)
                    prefs.QTIncludeAudio = false;
                returnVal = true;
                dialogDone = true;
                break;
            case ID_QO_CANCEL:
                returnVal = false;
                dialogDone = true;
                break;
            default: break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    curSet.action[ACTDOODLE] = storeDoodle;
    ResumeAutoChanges();
    if (prefs.fullScreen)
        MyHideMenuBar();
    return returnVal;
}

#define ID_MO_SAVE 1
#define ID_MO_CANCEL 2
#define ID_MO_DEFAULTS 3
#define ID_MO_NUMLINES 9
#define ID_MO_LINESIZE 10
#define ID_MO_LINESIZEV 11
#define ID_MO_LINESOUND 12
#define ID_MO_NUMBALLS 13
#define ID_MO_BALLSIZE 14
#define ID_MO_BALLSIZEV 15
#define ID_MO_BALLSOUND 16
#define ID_MO_BALLKEEPON 17
#define ID_MO_NUMBEES 18
#define ID_MO_BEESIZE 19
#define ID_MO_BEEREACTTIME 20
#define ID_MO_BEESOUND 21
#define ID_MO_BEESFOLLOW 22
#define ID_MO_BEEHIDEQUEEN 23
#define ID_MO_NUMDROPS 24
#define ID_MO_DROPSIZE 25
#define ID_MO_DROPSIZEV 26
#define ID_MO_DROPSOUND 27
#define ID_MO_DROPKEEPON 28
#define ID_MO_DOODLESIZE 29
#define ID_MO_DOODLESOUND 30

void MiscOptionsDialog(void) {
    Boolean dialogDone, storeDoodle;
    DialogPtr theDialog;
    Rect myRect;
    short itemHit, temp;
    ControlHandle ctrl;
    long number;
    Str255 tempStr;
    struct setinformation backup;

    SaveContext();
    StopAutoChanges();
    storeDoodle = curSet.action[ACTDOODLE];
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_MISCOPTIONS, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    if (prefs.windowPosition[WINMISCOPTS].h != 0 || prefs.windowPosition[WINMISCOPTS].v != 0) {
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINMISCOPTS].h, prefs.windowPosition[WINMISCOPTS].v, false);
    }

    SetPortDialogPort(theDialog);
    ResetCursor();

    ShowMiscOptionsValues(theDialog, true);
    SetSliderActions(theDialog, DLG_MISCOPTIONS);
    SelectDialogItemText(theDialog, ID_MO_NUMLINES, 0, 32767);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));

    SetDialogDefaultItem(theDialog, ID_MO_SAVE);
    UpdateSliderDisplay(theDialog);
    dialogDone = false;
    ConfigureFilter(ID_MO_SAVE, ID_MO_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        UpdateSliderDisplay(theDialog);
        switch (itemHit) {
            case ID_MO_SAVE:
                CurSetTouched();
                dialogDone = true;
                break;
            case ID_MO_CANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case ID_MO_DEFAULTS:
                SetMiscOptionsDefaults();
                ShowMiscOptionsValues(theDialog, true);
                UpdateSliderDisplay(theDialog);
                break;
            // EditTexts
            case ID_MO_NUMLINES:
            case ID_MO_NUMBALLS:
            case ID_MO_NUMBEES:
            case ID_MO_NUMDROPS:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, tempStr);
                StringToNum(tempStr, &number);
                if (itemHit == ID_MO_NUMLINES)
                    curSet.actCount[ACTLINES] = number;
                if (itemHit == ID_MO_NUMBALLS)
                    curSet.actCount[ACTBALLS] = number;
                if (itemHit == ID_MO_NUMBEES)
                    curSet.actCount[ACTSWARM] = number;
                if (itemHit == ID_MO_NUMDROPS)
                    curSet.actCount[ACTDROPS] = number;
                if (curSet.actCount[ACTLINES] > maxlines)
                    curSet.actCount[ACTLINES] = maxlines;
                if (curSet.actCount[ACTLINES] < 1)
                    curSet.actCount[ACTLINES] = 1;
                if (curSet.actCount[ACTBALLS] > maxballs)
                    curSet.actCount[ACTBALLS] = maxballs;
                if (curSet.actCount[ACTBALLS] < 1)
                    curSet.actCount[ACTBALLS] = 1;
                if (curSet.actCount[ACTSWARM] > maxbees)
                    curSet.actCount[ACTSWARM] = maxbees;
                if (curSet.actCount[ACTSWARM] < 1)
                    curSet.actCount[ACTSWARM] = 1;
                if (curSet.actCount[ACTDROPS] > maxdrops)
                    curSet.actCount[ACTDROPS] = maxdrops;
                if (curSet.actCount[ACTDROPS] < 1)
                    curSet.actCount[ACTDROPS] = 1;
                break;
            // Checkboxes
            case ID_MO_BEESFOLLOW:
            case ID_MO_LINESOUND:
            case ID_MO_BALLSOUND:
            case ID_MO_BEESOUND:
            case ID_MO_DROPSOUND:
            case ID_MO_DOODLESOUND:
            case ID_MO_DROPKEEPON:
            case ID_MO_BALLKEEPON:
            case ID_MO_BEEHIDEQUEEN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = !GetControlValue(ctrl);
                if (itemHit == ID_MO_BEESFOLLOW) {
                    if (temp)
                        curSet.actMisc1[ACTSWARM] |= 1;
                    else
                        curSet.actMisc1[ACTSWARM] &= ~1;
                }
                if (itemHit == ID_MO_BEEHIDEQUEEN) {
                    if (temp)
                        curSet.actMisc1[ACTSWARM] |= 2;
                    else
                        curSet.actMisc1[ACTSWARM] &= ~2;
                }
                if (itemHit == ID_MO_LINESOUND)
                    curSet.actReactSnd[ACTLINES] = temp;
                if (itemHit == ID_MO_BALLSOUND)
                    curSet.actReactSnd[ACTBALLS] = temp;
                if (itemHit == ID_MO_BEESOUND)
                    curSet.actReactSnd[ACTSWARM] = temp;
                if (itemHit == ID_MO_DROPSOUND)
                    curSet.actReactSnd[ACTDROPS] = temp;
                if (itemHit == ID_MO_BALLKEEPON)
                    curSet.actMisc1[ACTBALLS] = temp;
                if (itemHit == ID_MO_DROPKEEPON)
                    curSet.actMisc1[ACTDROPS] = temp;
                if (itemHit == ID_MO_DOODLESOUND)
                    curSet.actReactSnd[ACTDOODLE] = temp;
                ShowMiscOptionsValues(theDialog, false);
                break;
        }
        UpdateCurrentSetNeedsSoundInputMessage();
    }
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINMISCOPTS].h = myRect.left;
    prefs.windowPosition[WINMISCOPTS].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINMISCOPTS]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    curSet.action[ACTDOODLE] = storeDoodle;
    ResumeAutoChanges();
    if (prefs.fullScreen) { /*MyShowMenuBar();*/
        MyHideMenuBar();
    }
}

void SetLiveMiscOptionSliderValue(short item, short value) {
    if (item == ID_MO_LINESIZE)
        curSet.actSiz[ACTLINES] = value;
    if (item == ID_MO_LINESIZEV)
        curSet.actSizeVar[ACTLINES] = value;
    if (item == ID_MO_BALLSIZE)
        curSet.actSiz[ACTBALLS] = value;
    if (item == ID_MO_BALLSIZEV)
        curSet.actSizeVar[ACTBALLS] = value;
    if (item == ID_MO_BEESIZE)
        curSet.actSiz[ACTSWARM] = value;
    if (item == ID_MO_BEEREACTTIME)
        curSet.actSizeVar[ACTSWARM] = value;
    if (item == ID_MO_DROPSIZE)
        curSet.actSiz[ACTDROPS] = value;
    if (item == ID_MO_DROPSIZEV)
        curSet.actSizeVar[ACTDROPS] = value;
    if (item == ID_MO_DOODLESIZE)
        curSet.actSiz[ACTDOODLE] = value;
}

void ShowMiscOptionsValues(DialogPtr theDialog, Boolean clear) {
    static short sn[4];
    ControlHandle ctrl;
    Str255 tempStr;

    // set up EditTexts
    if (curSet.actCount[ACTLINES] != sn[0] || clear) {
        sn[0] = curSet.actCount[ACTLINES];
        NumToString(curSet.actCount[ACTLINES], tempStr);
        GetDialogItemAsControl(theDialog, ID_MO_NUMLINES, &ctrl);
        SetDialogItemText((Handle)ctrl, tempStr);
    }
    if (curSet.actCount[ACTLINES] != sn[1] || clear) {
        sn[1] = curSet.actCount[ACTLINES];
        NumToString(curSet.actCount[ACTBALLS], tempStr);
        GetDialogItemAsControl(theDialog, ID_MO_NUMBALLS, &ctrl);
        SetDialogItemText((Handle)ctrl, tempStr);
    }
    if (curSet.actCount[ACTLINES] != sn[2] || clear) {
        sn[2] = curSet.actCount[ACTLINES];
        NumToString(curSet.actCount[ACTSWARM], tempStr);
        GetDialogItemAsControl(theDialog, ID_MO_NUMBEES, &ctrl);
        SetDialogItemText((Handle)ctrl, tempStr);
    }
    if (curSet.actCount[ACTLINES] != sn[3] || clear) {
        sn[3] = curSet.actCount[ACTLINES];
        NumToString(curSet.actCount[ACTDROPS], tempStr);
        GetDialogItemAsControl(theDialog, ID_MO_NUMDROPS, &ctrl);
        SetDialogItemText((Handle)ctrl, tempStr);
    }

    // set up checkboxes
    GetDialogItemAsControl(theDialog, ID_MO_BEESFOLLOW, &ctrl);
    SetControlValue(ctrl, (curSet.actMisc1[ACTSWARM] & 1) > 0);
    GetDialogItemAsControl(theDialog, ID_MO_BEEHIDEQUEEN, &ctrl);
    SetControlValue(ctrl, (curSet.actMisc1[ACTSWARM] & 2) > 0);
    GetDialogItemAsControl(theDialog, ID_MO_LINESOUND, &ctrl);
    SetControlValue(ctrl, curSet.actReactSnd[ACTLINES]);
    GetDialogItemAsControl(theDialog, ID_MO_BALLSOUND, &ctrl);
    SetControlValue(ctrl, curSet.actReactSnd[ACTBALLS]);
    GetDialogItemAsControl(theDialog, ID_MO_BEESOUND, &ctrl);
    SetControlValue(ctrl, curSet.actReactSnd[ACTSWARM]);
    GetDialogItemAsControl(theDialog, ID_MO_DROPSOUND, &ctrl);
    SetControlValue(ctrl, curSet.actReactSnd[ACTDROPS]);
    GetDialogItemAsControl(theDialog, ID_MO_DOODLESOUND, &ctrl);
    SetControlValue(ctrl, curSet.actReactSnd[ACTDOODLE]);
    GetDialogItemAsControl(theDialog, ID_MO_BALLKEEPON, &ctrl);
    SetControlValue(ctrl, curSet.actMisc1[ACTBALLS]);
    GetDialogItemAsControl(theDialog, ID_MO_DROPKEEPON, &ctrl);
    SetControlValue(ctrl, curSet.actMisc1[ACTDROPS]);

    // set up sliders
    GetDialogItemAsControl(theDialog, ID_MO_LINESIZE, &ctrl);
    SetControlValue(ctrl, curSet.actSiz[ACTLINES]);
    if (curSet.actReactSnd[ACTLINES]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_LINESIZEV, &ctrl);
    SetControlValue(ctrl, curSet.actSizeVar[ACTLINES]);
    if (curSet.actReactSnd[ACTLINES]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_BALLSIZE, &ctrl);
    SetControlValue(ctrl, curSet.actSiz[ACTBALLS]);
    if (curSet.actReactSnd[ACTBALLS]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_BALLSIZEV, &ctrl);
    SetControlValue(ctrl, curSet.actSizeVar[ACTBALLS]);
    if (curSet.actReactSnd[ACTBALLS]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_BEESIZE, &ctrl);
    SetControlValue(ctrl, curSet.actSiz[ACTSWARM]);
    if (curSet.actReactSnd[ACTSWARM]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_BEEREACTTIME, &ctrl);
    SetControlValue(ctrl, curSet.actSizeVar[ACTSWARM]);
    GetDialogItemAsControl(theDialog, ID_MO_DROPSIZE, &ctrl);
    SetControlValue(ctrl, curSet.actSiz[ACTDROPS]);
    if (curSet.actReactSnd[ACTDROPS]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_DROPSIZEV, &ctrl);
    SetControlValue(ctrl, curSet.actSizeVar[ACTDROPS]);
    if (curSet.actReactSnd[ACTDROPS]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    GetDialogItemAsControl(theDialog, ID_MO_DOODLESIZE, &ctrl);
    SetControlValue(ctrl, curSet.actSiz[ACTDOODLE]);
    if (curSet.actReactSnd[ACTDOODLE]) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    UpdateSliderDisplay(theDialog);
}

void SetMiscOptionsDefaults(void) {
    curSet.actCount[ACTLINES] = 20;
    curSet.actReactSnd[ACTLINES] = false;
    curSet.actSiz[ACTLINES] = 20;
    curSet.actSizeVar[ACTLINES] = 10;

    curSet.actCount[ACTBALLS] = 20;
    curSet.actReactSnd[ACTBALLS] = false;
    curSet.actSiz[ACTBALLS] = 25;
    curSet.actSizeVar[ACTBALLS] = 8;
    curSet.actMisc1[ACTBALLS] = 0;    // keep on screen

    curSet.actCount[ACTSWARM] = 100;
    curSet.actReactSnd[ACTSWARM] = false;
    curSet.actSiz[ACTSWARM] = 20;
    curSet.actSizeVar[ACTSWARM] = 40;    // bee reaction time
    curSet.actMisc1[ACTSWARM] = 0;       // no follow mouse or hide queen

    curSet.actCount[ACTDROPS] = 50;
    curSet.actReactSnd[ACTDROPS] = false;
    curSet.actSiz[ACTDROPS] = 25;
    curSet.actSizeVar[ACTDROPS] = 60;
    curSet.actMisc1[ACTDROPS] = 0;    // keep on screen

    curSet.actSiz[ACTDOODLE] = 15;
    curSet.actReactSnd[ACTDOODLE] = false;
}

void MySetWindowTitle(void) {
    if (!gQTExporting)
        SetWTitle(gMainWindow, curSet.windowTitle);
}

void SanitizeString(Str255 str) {
    short c = 1;

    while (c <= str[0]) {
        while (c <= str[0] &&
               (str[c] == ';' || str[c] == '^' || str[c] == '!' || str[c] == '<' || str[c] == '/' || str[c] == '(')) {
            if (c < str[0])
                BlockMoveData(str + c + 1, str + c, str[0] - c);
            str[0]--;
        }
        c++;
    }
}

Boolean CommandIsPressed(void) {
    return (GetCurrentKeyModifiers() & cmdKey) != 0;
}    // 256

Boolean ShiftIsPressed(void) {
    return (GetCurrentKeyModifiers() & shiftKey) != 0;
}    // 512

Boolean OptionIsPressed(void) {
    return (GetCurrentKeyModifiers() & optionKey) != 0;
}    // 2048

Boolean ControlIsPressed(void) {
    return (GetCurrentKeyModifiers() & controlKey) != 0;
}    // 4096

Boolean CheckSystemRequirements(void) {   // return true if PixelToy may run
    OSErr err;
    long result;
    short versionMicro;
    Str32 str;

    // Is Appearance Manager present?  Stop if it's not!
    err = Gestalt(gestaltAppearanceAttr, &result);
    if (err || !(result & 1)) {
        StopAlert(BASERES + 29, nil);
        return false;
    }
    // If not Mac OS X, is CarbonLib 1.2 or newer present?  Stop if it's not!
    if (!IsMacOSX()) {
        err = Gestalt(gestaltCarbonVersion, &result);
        if (!err) {
            gSysVersionMajor = ((result & 0x0000FF00) >> 8);
            gSysVersionMinor = ((result & 0x000000F0) >> 4);
            versionMicro = ((result & 0x0000000F));
        }
        if (err || (gSysVersionMajor < 2 && gSysVersionMinor < 2)) {
            str[0] = 3;
            str[1] = '0' + gSysVersionMajor;
            str[2] = '.';
            str[3] = '0' + gSysVersionMinor;
            if (versionMicro > 0) {
                str[++str[0]] = '.';
                str[++str[0]] = '0' + versionMicro;
            }
            ParamText(str, 0, 0, 0);
            StopAlert(BASERES + 17, nil);
            return false;
        }
    }
    // Is QuickTime 3+ present?  Stop if it's not!
    err = Gestalt(gestaltQuickTimeVersion, &result);
    if (!err) {
        gSysVersionMajor = ((result & 0xFF000000) >> 24);
    }
    if (err || gSysVersionMajor < 3) {
        DoStandardAlert(kAlertStopAlert, 24);
        return false;
    }
    EnterMovies();
    return true;
}

// Nothin' much to do in carbonland.
void SetUpMacStuff(void) {
    InitCursor();
}

static pascal Boolean VerifyResDialogFilter(DialogPtr theDlg, EventRecord *event, DialogItemIndex *itemHit) {
    ControlHandle ctrl;
    Boolean buttonPressed = false;
    static long endTime = -1;

    switch (event->what) {
        case keyDown:
            endTime = TickCount() + 8;
            switch ((event->message) & charCodeMask) {
                case 0x0D:
                case 0x03:
                    *itemHit = 1;
                    buttonPressed = true;
                    break;
                case 0x1B:
                    *itemHit = 2;
                    buttonPressed = true;
                    break;
                case '.':
                    if (event->modifiers & cmdKey) {
                        *itemHit = 2;
                        buttonPressed = true;
                        break;
                    }
            }
            if (buttonPressed) {
                GetDialogItemAsControl(theDlg, *itemHit, &ctrl);
                if (IsControlActive(ctrl)) {
                    HiliteControl(ctrl, kControlDisabledPart);    // kControlButtonPart
                    endTime = TickCount() + 8;
                    while (TickCount() < endTime) {
                    }
                    HiliteControl(ctrl, kControlNoPart);
                } else {
                    *itemHit = 0;
                    buttonPressed = false;
                }
            }
            if (buttonPressed)
                endTime = -1;
            return buttonPressed;
        default:
            if (endTime == -1)
                endTime = TickCount() + 60 * 5;
            if (TickCount() > endTime) {
                *itemHit = 2;
                return true;
            }
            return false;
            break;
    }
}

void ConfigureFilter(short setOK, short setCancel) {
    filterOK = setOK;
    filterCancel = setCancel;
}

void CreateDialogFilter(void) {
    DialogFilterProc = NewModalFilterUPP(DialogFilter);
}

/*static*/ pascal Boolean DialogFilter(DialogPtr theDlg, EventRecord *event, DialogItemIndex *itemHit) {
    long endTime;
    Boolean buttonPressed = false;
    short thePart, y, winNum;
    ControlHandle ctrl;
    Rect rect;
    WindowPtr whichWindow;
    Point winPos;
    RgnHandle rgn;

    switch (event->what) {
        case mouseDown:
            thePart = FindWindow(event->where, &whichWindow);
            if (whichWindow == GetDialogWindow(theDlg)) {
                switch (thePart) {
                    case inDrag:
                        SaveContext();
                        SetPortWindowPort(whichWindow);
                        rect = gDragRect;
                        DragWindow(whichWindow, event->where, &rect);
                        GetWindowPortBounds(whichWindow, &rect);
                        winPos.h = rect.left;
                        winPos.v = rect.top;
                        LocalToGlobal(&winPos);
                        RestoreContext();
                        return true;
                        break;
                }
            }
            return false;
            break;
        case keyDown:
            switch ((event->message) & charCodeMask) {
                case 0x0D:
                case 0x03:
                    *itemHit = filterOK;
                    buttonPressed = true;
                    break;
                case 0x1B:
                    *itemHit = filterCancel;
                    buttonPressed = true;
                    break;
                case '.':
                    if (event->modifiers & cmdKey) {
                        *itemHit = filterCancel;
                        buttonPressed = true;
                        break;
                    }
            }
            if (buttonPressed) {
                GetDialogItemAsControl(theDlg, *itemHit, &ctrl);
                if (IsControlActive(ctrl)) {
                    HiliteControl(ctrl, kControlDisabledPart);    // kControlButtonPart
                    endTime = TickCount() + 8;
                    while (TickCount() < endTime) {
                        LetTimersBreathe();
                    }
                    HiliteControl(ctrl, kControlNoPart);
                } else {
                    *itemHit = 0;
                    buttonPressed = false;
                }
            }
            return buttonPressed;
            break;
        case updateEvt:
            winNum = -1;
            for (y = 0; y < MAXWINDOW; y++) {
                if (prefs.windowOpen[y]) {
                    if ((WindowPtr)event->message == gWindow[y])
                        winNum = y;
                }
            }
            if (winNum != -1) {   // It's a palette window.
                BeginUpdate(gWindow[winNum]);
                EndUpdate(gWindow[winNum]);
                PTUpdateWindow(winNum);
                PTDrawWindow(winNum);
                return true;
            }
            if (!IsMacOSX()) {
                if ((WindowPtr)event->message == GetDialogWindow(theDlg)) {    // theDlg was curDialog
                    BeginUpdate(GetDialogWindow(theDlg));
                    rgn = NewRgn();
                    GetPortVisibleRegion(GetDialogPort(theDlg), rgn);
                    UpdateDialog(theDlg, rgn);
                    DisposeRgn(rgn);
                    UpdateSliderDisplay(theDlg);
                    EndUpdate(GetDialogWindow(theDlg));
                    return true;
                }
            }
            return false;
            break;
        default:
            if (!IsMacOSX()) {
                LetTimersBreathe();
            }
            return false;
            break;
    }
}

void UpdateSliderDisplay(DialogPtr theDialog) {
    ControlHandle ctrl;
    short item = 1;

    if (theDialog == curDialog) {
        while (item < 128) {
            GetDialogItemAsControl(theDialog, item, &ctrl);
            if (IsControlVisible(ctrl)) {
                if (GetControlReference(ctrl) == item){   // it's a slider
                                                          // the 42 lets it know this is just for drawing purposes
                    InvokeControlActionUPP(ctrl, 42, SliderFeedbackProc);
                }
            }
            item++;
        }
    }
}

void SetSliderActions(DialogPtr theDialog, short dialogID) {
    short itemType, item = 1;
    ControlHandle ctrl;
    Rect rect;
    OSErr err = 0;
    Str255 ctrlName;

    curDialog = theDialog;
    curDialogID = dialogID;
    while (!err && item < 128) {
        GetDialogItem(theDialog, item, &itemType, &ctrl, &rect);
        if (ctrl) {
            GetControlTitle(ctrl, ctrlName);
            if (EqualString(ctrlName, "\pSlider", false, false)) {
                SetControlReference(ctrl, item);
                SetControlAction(ctrl, SliderFeedbackProc);
            }
        }
        item++;
    }
}

pascal void MySliderProc(ControlHandle ctrl, ControlPartCode partCode) {
    short itemType, value, c;
    short item;
    Handle handle;
    Rect rect, labelRect;
    Str255 str;

    value = GetControlValue(ctrl);
    // find out ctrl's item #
    item = GetControlReference(ctrl);
    if (item > 0 && item < 128) {
        GetDialogItem(curDialog, item, &itemType, &handle, &rect);
        NumToString(value, str);
        while (str[0] < 5) {
            for (c = str[0]; c > 0; c--) {
                str[c + 1] = str[c];
            }
            str[1] = ' ';
            str[0]++;
        }
        TextSize(11);
        if (IsMacOSX()) {   // we erase and draw text ORed
            static const Str32 maxStr = "\p999%";
            static short maxWidth = -1;

            if (maxWidth < 0)
                maxWidth = UThemePascalStringWidth(maxStr, kThemeCurrentPortFont);
            labelRect.left = rect.right - maxWidth;
            labelRect.right = rect.right;
            labelRect.bottom = rect.top - 4;
            labelRect.top = labelRect.bottom - 11;
            EraseRect(&labelRect);
            TextMode(srcOr);
        } else {   // we just draw text copy'ed
            TextMode(srcCopy);
        }
        str[++str[0]] = '%';
        short strwid = UThemePascalStringWidth(str, kThemeCurrentPortFont);
        MoveTo(rect.right - strwid, rect.top - 5);
        if (IsControlActive(ctrl))
            ForeColor(blackColor);
        else {
            RGBColor grey;
            grey.red = grey.green = grey.blue = 32767;
            RGBForeColor(&grey);
        }
        UDrawThemePascalString(str, kThemeCurrentPortFont);
        ForeColor(blackColor);
        if (partCode != 42)
            SetLiveSliderValue(curDialogID, item, value);
    }
}

void SetLiveSliderValue(short dialogID, short item, short value) {
    switch (dialogID) {
        case DLG_MISCOPTIONS: SetLiveMiscOptionSliderValue(item, value); break;
        case DLG_PARTICLEDITOR: SetLiveParticleSliderValue(item, value); break;
        case DLG_TEXTEDITOR: SetLiveTextSliderValue(item, value); break;
        case DLG_PREFERENCES: SetLivePrefsSliderValue(item, value); break;
        case DLG_IMAGEEDITOR: SetLiveImageSliderValue(item, value); break;
        case DLG_WAVEEDITOR: SetLiveWaveSliderValue(item, value); break;
    }
}

void MyHideMenuBar(void) {
    RgnHandle screenRgn;
    Rect screenRect, sRect;
    BitMap qdbm;

    GetQDGlobalsScreenBits(&qdbm);
    sRect = qdbm.bounds;
    GetWindowDeviceRect(gMainWindow, &screenRect);
    if (!gGrayRgn) {
        gBarHeight = GetMBarHeight();
        // store old grayRgn
        gGrayRgn = NewRgn();
        CopyRgn(GetGrayRgn(), gGrayRgn);
        // get Rect of gMainWindow's owner device
        screenRgn = NewRgn();
        RectRgn(screenRgn, &screenRect);
        UnionRgn(gGrayRgn, screenRgn, GetGrayRgn());
        DisposeRgn(screenRgn);
    }
    if (EqualRect(&screenRect, &sRect))
        HideMenuBar();
}

void MyShowMenuBar(void) {
    if (gGrayRgn) {
        SectRgn(gGrayRgn, GetGrayRgn(), GetGrayRgn());
        DisposeRgn(gGrayRgn);
        gGrayRgn = 0;
        ShowMenuBar();
        DrawMenuBar();
    }
}

#define ID_NOPAL_OK 1
#define ID_NOPAL_NOTAGAIN 2
void NotifyNoPaletteTransition(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone;
    ControlHandle ctrl;

    if (prefs.dontcomplainpaltrans)
        return;
    theDialog = GetNewDialog(DLG_NOPALTRANS, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_NOPAL_OK: dialogDone = true; break;
            case ID_NOPAL_NOTAGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainpaltrans = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainpaltrans);
                break;
        }
    }
    DisposeDialog(theDialog);
}

#define ID_RETURNFS_OK 1
#define ID_RETURNFS_CANCEL 2
#define ID_RETURNFS_NOTAGAIN 3
Boolean NotifyReturningFullScreen(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone, returnVal = true;
    ControlHandle ctrl;

    if (prefs.dontcomplainfsstart)
        return true;
    theDialog = GetNewDialog(DLG_RETURNFULLSCREEN, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_RETURNFS_OK: dialogDone = true; break;
            case ID_RETURNFS_CANCEL:
                returnVal = false;
                dialogDone = true;
                break;
            case ID_RETURNFS_NOTAGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainfsstart = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainfsstart);
                break;
        }
    }
    DisposeDialog(theDialog);
    //PopPal();
    return returnVal;
}

#define ID_VMWARN_OK 1
#define ID_VMWARN_NOTAGAIN 2
void NotifyVirtualMemorySucks(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone;
    ControlHandle ctrl;
    OSErr err;
    long result;

    if (prefs.dontcomplainaboutvm || IsMacOSX())
        return;
    err = Gestalt(gestaltVMAttr, &result);
    if (err)
        return;
    if (!(result & 1))
        return;
    theDialog = GetNewDialog(DLG_VMWARNING, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_VMWARN_OK: dialogDone = true; break;
            case ID_VMWARN_NOTAGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainaboutvm = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainaboutvm);
                break;
        }
    }
    DisposeDialog(theDialog);
}

// Warns the user of the possibility of a freeze if they're
// using a New World G4 (which includes the Cube) with Mac OS 9.
#define ID_CUBEWARN_OK 1
#define ID_CUBEWARN_NOTAGAIN 2
void WarnAboutCubeSound(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone;
    ControlHandle ctrl;
    OSErr err;
    long result;

    if (prefs.dontcomplainaboutcube || IsMacOSX())
        return;
    err = Gestalt(gestaltMachineType, &result);
    if (result != 406)
        return;    // New World
    err = Gestalt(gestaltNativeCPUtype, &result);
    if (result != 268)
        return;    // G4 in Cube
    theDialog = GetNewDialog(DLG_CUBEWARNING, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_CUBEWARN_OK: dialogDone = true; break;
            case ID_CUBEWARN_NOTAGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainaboutcube = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainaboutcube);
                break;
        }
    }
    DisposeDialog(theDialog);
}

#define ID_OSXSOUNDINPUT_OK 1
#define ID_OSXSOUNDINPUT_NOTAGAIN 2
void WarnAboutOSXSoundInput(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone;
    ControlHandle ctrl;

    if (prefs.dontcomplainaboutosxsi || !IsMacOSX())
        return;
    theDialog = GetNewDialog(DLG_OSXSOUNDINPUT, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_OSXSOUNDINPUT_OK: dialogDone = true; break;
            case ID_OSXSOUNDINPUT_NOTAGAIN:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainaboutosxsi = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainaboutosxsi);
                break;
        }
    }
    DisposeDialog(theDialog);
}

#define ID_WARN256_QUIT 1
#define ID_WARN256_CONTINUE 2
#define ID_WARN256_DONTASK 3
// return TRUE = quit, FALSE = continue
Boolean WarnAbout256Colors(void) {
    short itemHit, depth;
    DialogPtr theDialog;
    Boolean dialogDone, returnVal = false;
    ControlHandle ctrl;

    if (prefs.dontcomplainabout256)
        return false;
    depth = GetWindowDevice(gMainWindow);
    if (depth > 8)
        return false;
    theDialog = GetNewDialog(DLG_WARN256COLORS, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);
        switch (itemHit) {
            case ID_WARN256_QUIT:
                dialogDone = true;
                returnVal = true;
                break;
            case ID_WARN256_CONTINUE: dialogDone = true; break;
            case ID_WARN256_DONTASK:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                prefs.dontcomplainabout256 = !GetControlValue(ctrl);
                SetControlValue(ctrl, prefs.dontcomplainabout256);
                break;
        }
    }
    DisposeDialog(theDialog);
    return returnVal;
}

#define ID_REPLACE_CANCEL 1
#define ID_REPLACE_REPLACE 2
Boolean ReplaceDialog(void) {
    short itemHit;
    DialogPtr theDialog;
    Boolean dialogDone, returnVal;

    theDialog = GetNewDialog(DLG_REPLACE, nil, (WindowPtr)-1);
    SetPortDialogPort(theDialog);
    InitCursor();
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_REPLACE_CANCEL);
    dialogDone = false;
    while (!dialogDone && !gDone) {
        ModalDialog(nil, &itemHit);    // DialogFilterProc
        switch (itemHit) {
            case ID_REPLACE_CANCEL:
                returnVal = false;
                dialogDone = true;
                break;
            case ID_REPLACE_REPLACE:
                returnVal = true;
                dialogDone = true;
                break;
        }
    }
    DisposeDialog(theDialog);
    return returnVal;
}

SInt16 DoStandardAlert(AlertType type, short id) {
    Str255 str1, str2;
    SInt16 outItemHit;
    AlertStdAlertParamRec ap;

    ResetCursor();
    GetIndString(str1, BASERES + 4, ((id - 1) * 2) + 1);
    GetIndString(str2, BASERES + 4, ((id - 1) * 2) + 2);
    ap.movable = true;
    ap.helpButton = false;
    ap.filterProc = DialogFilterProc;
    ap.defaultText = (StringPtr)-1;
    ap.cancelText = nil;
    ap.otherText = nil;
    ap.defaultButton = kAlertStdAlertOKButton;
    ap.cancelButton = 0;
    ap.position = kWindowDefaultPosition;
    StandardAlert(type, str1, str2, &ap, &outItemHit);
    return outItemHit;
}

Boolean EqualFSS(FSSpec *fss1, FSSpec *fss2) {
    Boolean same;

    same = (fss1->vRefNum == fss2->vRefNum);
    if (same)
        same = (fss1->parID == fss2->parID);
    if (same)
        same = EqualString(fss1->name, fss2->name, false, false);
    return same;
}

Boolean IsMacOSX(void) {
    static Boolean doneGestalt = false, isX;
    long result;

    if (!doneGestalt) {
        Gestalt(gestaltMenuMgrAttr, &result);
        isX = (result & gestaltMenuMgrAquaLayoutMask);
    }
    return isX;
}

// Draws text into current port at given coords, at given brightness.
// *** ASSUMES PORT TO DRAW IN IS 8 OR 32 BITS! ***
// Pass -42 in BOTH tx and ty in order to use QuickDraw's current pen location
void MyDrawText(Str255 str, short tx, short ty, short brite) {
    static Rect textRect;
    static CGrafPtr textWorld = 0;
    OSErr err;
    Rect thisRect, curRect, toRect;
    CGrafPtr curWorld;
    GDHandle curDevice;
    char bit, is32bit;
    Point pen;
    short bx, x = 0, y = 0, xOff, yOff, yBegin, yEnd;
    short size, font;
    StyleField face;
    RGBColor color;
    PixMapHandle textpm, curpm;
    Ptr textptr, curptr, toptr;
    long textcount, curcount;

    if (brite < 0)
        brite = 0;
    if (brite > 255)
        brite = 255;
    GetGWorld(&curWorld, &curDevice);
    GetPortBounds(curWorld, &curRect);
    if (tx == -42 && ty == -42) {
        GetPortPenLocation(curWorld, &pen);
        tx = pen.h;
        ty = pen.v;
    }
    size = GetPortTextSize(curWorld);
    font = GetPortTextFont(curWorld);
    face = GetPortTextFace(curWorld);
    thisRect.left = thisRect.top = 0;
    thisRect.right = StringWidth(str) + 20;
    thisRect.bottom = size * 1.6;
    xOff = 10;
    yOff = size * 1.1;
    toRect = thisRect;
    OffsetRect(&toRect, tx - xOff, ty - yOff);
    if (!textWorld || thisRect.right > textRect.right || thisRect.bottom > textRect.bottom) {
        if (textWorld)
            DisposeGWorld(textWorld);
        textRect = thisRect;
        err = NewGWorld(&textWorld, 1, &textRect, nil, nil, 0);
        if (err)
            goto dt_out;
    }
    // Set up textWorld raw data
    textpm = GetGWorldPixMap(textWorld);
    if (!PTLockPixels(textpm)) {
        DisposeGWorld(textWorld);
        textWorld = 0;
        goto dt_out;
    }
    textptr = GetPixBaseAddr(textpm);
    textcount = (0x7FFF & (**textpm).rowBytes);
    // Set up curWorld raw data
    curpm = GetGWorldPixMap(curWorld);
    is32bit = (((PixMapPtr)*curpm)->pixelSize == 32);
    if (!PTLockPixels(curpm)) {
        DisposeGWorld(textWorld);
        textWorld = 0;
        goto dt_out;
    }
    curptr = GetPixBaseAddr(curpm);
    curcount = (0x7FFF & (**curpm).rowBytes);
    // Draw text into textWorld
    SetGWorld(textWorld, nil);
    ForeColor(blackColor);
    BackColor(whiteColor);
    EraseRect(&thisRect);
    MoveTo(xOff, yOff);
    TextFont(font);
    TextSize(size);
    TextFace(face);
    DrawString(str);
    SetGWorld(curWorld, curDevice);
    // Blit textWorld to curWorld
    toptr = curptr + (toRect.top * curcount);
    if (is32bit)
        toptr += (toRect.left * 4);
    else
        toptr += toRect.left;
    yBegin = 0;
    if (toRect.top < 0) {
        yBegin = -toRect.top;
        toptr += yBegin * curcount;
        textptr += yBegin * textcount;
    }
    yEnd = thisRect.bottom;
    if (toRect.bottom > curRect.bottom)
        yEnd -= (toRect.bottom - curRect.bottom);
    if (!is32bit) {   // 1 bit -> 8 bit 'srcOr' blit
        for (y = yBegin; y < yEnd; y++) {
            bit = 7;
            bx = 0;
            for (x = 0; x < thisRect.right; x++) {
                if (toRect.left + x >= 0 && toRect.left + x < curRect.right) {
                    if (((unsigned char)textptr[bx] & (1 << bit)))
                        toptr[x] = brite;
                }
                bit--;
                if (bit < 0) {
                    bx++;
                    bit = 7;
                }
            }
            toptr += curcount;
            textptr += textcount;
        }
    } else {   // 1 bit -> 32 bit 'srcOr' blit (brite ignored)
        for (y = yBegin; y < yEnd; y++) {
            bit = 7;
            bx = 0;
            for (x = 0; x < thisRect.right; x++) {
                if (toRect.left + x >= 0 && toRect.left + x < curRect.right) {
                    if (((unsigned char)textptr[bx] & (1 << bit))) {
                        toptr[x * 4 + 1] = 255;
                        toptr[x * 4 + 2] = 255;
                        toptr[x * 4 + 3] = 255;
                    }
                }
                bit--;
                if (bit < 0) {
                    bx++;
                    bit = 7;
                }
            }
            toptr += curcount;
            textptr += textcount;
        }
    }
    return;
dt_out:    // Draw text the unpredictable old fashioned way.
    Index2Color(brite, &color);
    RGBForeColor(&color);
    MoveTo(tx, ty);
    DrawString(str);
}

Boolean MyIsMenuItemEnabled(MenuRef menu, MenuItemIndex item) {
#if TARGET_CPU_68K
    return ((*menu)->enableFlags & (1 << (item + 1)));
#else
    return IsMenuItemEnabled(menu, item);
#endif
}

void MyDisableMenuItem(MenuRef theMenu, short item) {
#if TARGET_CPU_68K
    DisableItem(theMenu, item);
#else
    DisableMenuItem(theMenu, item);
#endif
}

void MyEnableMenuItem(MenuRef theMenu, short item) {
#if TARGET_CPU_68K
    EnableItem(theMenu, item);
#else
    EnableMenuItem(theMenu, item);
#endif
}

OSStatus MyValidWindowRect(WindowRef window, const Rect *bounds) {
#if TARGET_CPU_68K
    ValidRect(bounds);
#else
    ValidWindowRect(window, bounds);
#endif
    return noErr;
}

OSStatus MyInvalWindowRect(WindowRef window, const Rect *bounds) {
#if TARGET_CPU_68K
    InvalRect(bounds);
#else
    InvalWindowRect(window, bounds);
#endif
    return noErr;
}

Boolean GoodHandle(Handle h) {
#if TARGET_CPU_68K
    return (h != 0);
#else
    return IsHandleValid(h);
#endif
}

void DebugString(Str255 str) {
    ParamText(str, 0, 0, 0);
    NoteAlert(402, nil);
}
