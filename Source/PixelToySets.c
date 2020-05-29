// PixelToy Set functions

#define SETRECORDSIZE 4096

#include "PixelToySets.h"

#include "Defs&Structs.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyBitmap.h"
#include "PixelToyIF.h"
#include "PixelToyFilters.h"
#include "PixelToyMain.h"
#include "PixelToyPalette.h"
#include "PixelToyParticles.h"
#include "PixelToyText.h"
#include "PixelToyWaves.h"
#include "PixelToyWindows.h"
#include "EndianRemapping.h"
#include "CocoaBridge.h"

extern struct setinformation curSet;
extern struct preferences prefs;
extern struct PTParticle *ptparticle;
extern ModalFilterUPP DialogFilterProc;
extern WindowPtr gMainWindow;
extern PaletteHandle winPalette;
extern MenuHandle gSetsMenu;
extern Boolean gCapableSoundIn, gDone, gShowNeedsSoundInput;
extern Boolean gCurSetsReadOnly;
extern FSSpec curSetsSpec;
extern Ptr ptSets;
extern short gNumSets;
extern long maxparticles;

Boolean setsModified, setInMemory = false;
long nextSetChange;
short lastSetLoaded = -1;

Boolean GetAutoInterval(void) {
    Boolean dialogDone;
    DialogPtr theDialog;
    Str255 str;
    ControlHandle ctrl;
    Rect myRect;
    short itemHit;
    long value;

    StopAutoChanges();
    SaveContext();
    ResetCursor();
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_GETINTERVAL, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    if (prefs.windowPosition[WINTIMEDSET].h != 0 || prefs.windowPosition[WINTIMEDSET].v != 0) {
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINTIMEDSET].h, prefs.windowPosition[WINTIMEDSET].v, false);
    }
    SetPortDialogPort(theDialog);
    prefs.windowPosition[WINTIMEDSET];
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItemAsControl(theDialog, 3, &ctrl);
    NumToString(prefs.autoSetInterval, str);
    SetDialogItemText((Handle)ctrl, str);
    SelectDialogItemText(theDialog, 3, 0, 32767);
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    ConfigureFilter(1, 2);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case 1:    // OK
                GetDialogItemAsControl(theDialog, 3, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                StringToNum(str, &value);
                prefs.autoSetInterval = value;
                dialogDone = true;
                nextSetChange = TickCount() + (prefs.autoSetInterval * 60);
                break;
            case 2:    // Cancel
                dialogDone = true;
                break;
        }
    }
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINTIMEDSET].h = myRect.left;
    prefs.windowPosition[WINTIMEDSET].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINTIMEDSET]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    return (itemHit == 1);
}

void TimedSetChange(void) {
    if (prefs.autoSetChange) {
        if (TickCount() > nextSetChange) {
            UsePixelToySet(-2);
            nextSetChange = TickCount() + (prefs.autoSetInterval * 60);
        }
    }
}

#define ID_RRENAME 1
#define ID_RCANCEL 2
#define ID_RSETPOPUP 3
#define ID_RNEWNAME 4

void RenameOldSet(short preselected) {   // preselected<0 = none selected
    short itemHit, temp, c;
    struct setinformation si;
    MenuHandle setListMenu;
    ControlHandle ctrl;
    Rect myRect;
    DialogPtr theDialog;
    Boolean dialogDone;
    Str255 setName;

    if (gNumSets < 1) {
        SysBeep(1);
        return;
    }
    StopAutoChanges();
    EnsureSystemPalette();
    SaveContext();
    theDialog = GetNewDialog(DLG_RENAMESET, nil, (WindowPtr)-1);
    // Get pop-up menu's handle & remove any items in it
    GetDialogItemAsControl(theDialog, ID_RSETPOPUP, &ctrl);
    setListMenu = GetControlPopupMenuHandle(ctrl);
    temp = CountMenuItems(setListMenu);
    for (c = 1; c <= temp; c++) {
        DeleteMenuItem(setListMenu, 1);
    }
    // Put in "Select a set to rename" and add all set names
    GetIndString(setName, BASERES, 12);
    AppendMenu(setListMenu, setName);
    MyDisableMenuItem(setListMenu, 1);
    for (c = 0; c < gNumSets; c++) {
        LoadPixelToySet(&si, c);
        AppendMenuItemText(setListMenu, si.windowTitle);
    }
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    GetDialogItemAsControl(theDialog, ID_RNEWNAME, &ctrl);
    DeactivateControl(ctrl);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItem(theDialog, 1, &temp, &ctrl, &myRect);
    DeactivateControl(ctrl);
    SetDialogDefaultItem(theDialog, ID_RRENAME);
    dialogDone = false;
    ConfigureFilter(ID_RRENAME, ID_RCANCEL);
    while (!dialogDone && !gDone) {
        if (preselected > -1) {
            GetDialogItemAsControl(theDialog, ID_RSETPOPUP, &ctrl);
            SetControlValue(ctrl, preselected + 2);
            itemHit = ID_RSETPOPUP;
            preselected = -1;
        } else {
            ModalDialog(DialogFilterProc, &itemHit);
        }
        switch (itemHit) {
            case ID_RRENAME:
                // do rename
                GetDialogItemAsControl(theDialog, ID_RSETPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    GetDialogItemAsControl(theDialog, ID_RNEWNAME, &ctrl);
                    GetDialogItemText((Handle)ctrl, setName);
                    if (setName[0] > 63) {
                        setName[0] = 63;
                        setName[63] = '.';
                        setName[62] = '.';
                        setName[61] = '.';
                    }
                    LoadPixelToySet(&si, c - 2);
                    BlockMoveData(setName, si.windowTitle, setName[0] + 1);
                    DoDeleteOldSet(c - 2);
                    DoInsertNewSet(&si);
                    lastSetLoaded = -1;
                    PTInvalidateWindow(WINSETS);
                    dialogDone = true;
                }
                break;
            case ID_RCANCEL: dialogDone = true; break;
            case ID_RSETPOPUP:
                GetDialogItemAsControl(theDialog, ID_RSETPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    // activate Rename button
                    GetDialogItemAsControl(theDialog, ID_RRENAME, &ctrl);
                    ActivateControl(ctrl);
                    // activate Edit field
                    GetDialogItemAsControl(theDialog, ID_RNEWNAME, &ctrl);
                    ActivateControl(ctrl);
                    LoadPixelToySet(&si, c - 2);
                    SetDialogItemText((Handle)ctrl, si.windowTitle);
                    SelectDialogItemText(theDialog, ID_RNEWNAME, 0, 32767);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
}

#define ID_ECSAVE 1
#define ID_ECCANCEL 2
#define ID_ECPOPUP 3
#define ID_ECTEXT 4
void EditSetComments(void) {
    short savelastloaded, itemHit, temp, c;
    struct setinformation si;
    MenuHandle setListMenu;
    ControlHandle ctrl;
    Rect myRect;
    DialogPtr theDialog;
    Boolean dialogDone;
    Str255 str;

    if (gNumSets < 1) {
        SysBeep(1);
        return;
    }
    StopAutoChanges();
    savelastloaded = lastSetLoaded;
    EnsureSystemPalette();
    SaveContext();
    theDialog = GetNewDialog(DLG_EDITSETCOMM, nil, (WindowPtr)-1);
    // Get pop-up menu's handle & remove any items in it
    GetDialogItemAsControl(theDialog, ID_ECPOPUP, &ctrl);
    setListMenu = GetControlPopupMenuHandle(ctrl);
    temp = CountMenuItems(setListMenu);
    for (c = 1; c <= temp; c++) {
        DeleteMenuItem(setListMenu, 1);
    }
    // Put in "Select a set"... and add set names
    GetIndString(str, BASERES, 13);
    AppendMenu(setListMenu, str);
    MyDisableMenuItem(setListMenu, 1);
    for (c = 0; c < gNumSets; c++) {
        LoadPixelToySet(&si, c);
        AppendMenuItemText(setListMenu, si.windowTitle);
    }
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    GetDialogItemAsControl(theDialog, ID_ECTEXT, &ctrl);
    DeactivateControl(ctrl);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItem(theDialog, 1, &temp, &ctrl, &myRect);
    DeactivateControl(ctrl);
    SetDialogDefaultItem(theDialog, ID_ECSAVE);
    dialogDone = false;
    ConfigureFilter(ID_ECSAVE, ID_ECCANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_ECSAVE:
                // do edit
                GetDialogItemAsControl(theDialog, ID_ECPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    GetDialogItemAsControl(theDialog, ID_ECTEXT, &ctrl);
                    GetDialogItemText((Handle)ctrl, str);
                    LoadPixelToySet(&si, c - 2);
                    BlockMoveData(str, si.setComment, str[0] + 1);
                    DoDeleteOldSet(c - 2);
                    DoInsertNewSet(&si);
                    lastSetLoaded = savelastloaded;
                    PTInvalidateWindow(WINSETS);
                    dialogDone = true;
                }
                break;
            case ID_ECCANCEL: dialogDone = true; break;
            case ID_ECPOPUP:
                GetDialogItemAsControl(theDialog, ID_ECPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    // activate Save button
                    GetDialogItemAsControl(theDialog, ID_ECSAVE, &ctrl);
                    ActivateControl(ctrl);
                    // activate Edit field
                    GetDialogItemAsControl(theDialog, ID_ECTEXT, &ctrl);
                    ActivateControl(ctrl);
                    LoadPixelToySet(&si, c - 2);
                    SetDialogItemText((Handle)ctrl, si.setComment);
                    SelectDialogItemText(theDialog, ID_ECTEXT, 0, 32767);
                }
                break;
            case ID_ECTEXT:
                GetDialogItemAsControl(theDialog, ID_ECTEXT, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 240) {
                    str[0] = 240;
                    SetDialogItemText((Handle)ctrl, str);
                    SysBeep(1);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
}

void ZeroSettings(void) {
    short i;

    ClearFromAndBase();
    // force to grey
    for (i = 0; i < 256; i++) {
        curSet.palentry[i].red = i * 256;
        curSet.palentry[i].green = i * 256;
        curSet.palentry[i].blue = i * 256;
    }
    EmployPalette(true);
    MarkCurrentPalette();
    for (i = 0; i < MAXACTION; i++) {
        curSet.action[i] = false;
    }
    ClearAllFilters();
    curSet.horizontalMirror = false;
    curSet.verticalMirror = false;
    SetMiscOptionsDefaults();
    for (i = 0; i < MAXTEXTS; i++) {
        curSet.textGen[i].behave = 0;
    }
    CreateNewText(true);
    for (i = 0; i < MAXGENERATORS; i++) {
        curSet.pg_pbehavior[i] = 0;
    }
    CreateNewGenerator();
    for (i = 0; i < MAXIMAGES; i++) {
        ResetBMWorld(i, SB_BOTH);
        curSet.image[i].active = 0;
    }
    CreateNewImage();
    for (i = 0; i < MAXWAVES; i++) {
        curSet.wave[i].active = 0;
    }
    CreateNewWave();
    GetIndString(curSet.windowTitle, BASERES, 27);    // Untitled
    MySetWindowTitle();
    SetDefaultCustomFilter();
    curSet.postFilter = false;
    curSet.emboss = false;
    curSet.sound = false;
    curSet.paletteSoundMode = PSMODE_NONE;
    CurSetTouched();
    ShowSettings();
}

void GetAndUsePixelToySets(void) {
    FSSpec theFSS;
    OSErr theErr = CocoaChooseFile(CFSTR("ChooseSetsFile"), CFSTR("STNG"), &theFSS);
    if (noErr == theErr)
        MyOpenFile(theFSS);
}

void LoadPixelToySet(void *mem, short set) {
    struct setinformation *si = mem;
    long offset;

    offset = (set * sizeof(struct setinformation));
    BlockMoveData(ptSets + offset, si, sizeof(struct setinformation));
}

void UsePixelToySet(short set) {   // -1 back up, -2 go forward
    short t, numMissingFonts = 0, i;
    Boolean thisFontMissing;
    Str63 fontName[16];
    Str255 sysFontName, allNames, alertMessage;
    short fontNum, m;
    SInt16 outItemHit;

    if (gNumSets < 1 || set >= gNumSets)
        return;
    if (set < 0) {
        if (lastSetLoaded == -1) {
            set = 0;
        } else {
            if (set == -1) {
                set = lastSetLoaded - 1;
            } else {
                set = lastSetLoaded + 1;
            }
        }
    }
    if (set < 0)
        set = gNumSets - 1;
    if (set >= gNumSets)
        set = 0;
    lastSetLoaded = set;
    LoadPixelToySet(&curSet, set);
    ValidateParticleGeneratorBrightnesses(&curSet);
    if (!curSet.action[ACTIMAGES]) {
        for (i = 0; i < MAXIMAGES; i++) {
            ResetBMWorld(i, SB_BOTH);
        }
    }
    t = 0;
    for (i = 0; i < MAXIMAGES; i++) {
        t += (curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE);
    }
    if (t == 0)
        CreateNewImage();
    if (curSet.action[ACTSWAVE] && curSet.actMisc2[ACTSWAVE] != 42) {   // old sound wave form
        if (curSet.reservedBool1) {   // create a horizontal wave
            t = CreateNewWave();
            SetRect(&curSet.wave[t].place, 200, 200, 16284, 16284);
            curSet.wave[t].thickness = curSet.actSiz[ACTSWAVE];
        }
        if (curSet.reservedBool2) {   // create a vertical wave
            t = CreateNewWave();
            SetRect(&curSet.wave[t].place, 200, 200, 16284, 16284);
            curSet.wave[t].thickness = curSet.actMisc1[ACTSWAVE];
            curSet.wave[t].vertical = 1;
        }
        curSet.reservedBool1 = false;
        curSet.reservedBool2 = false;
        curSet.actMisc2[ACTSWAVE] = 42;    // Marked
    }
    t = 0;
    for (i = 0; i < MAXWAVES; i++) {
        t += curSet.wave[i].active;
    }
    if (t == 0)
        CreateNewWave();
    GetFontName(0, sysFontName);
    if (prefs.notifyFontMissing && curSet.action[ACTTEXT]) {
        for (t = 0; t < MAXTEXTS; t++) {
            if (curSet.textGen[t].behave > 0) {
                GetFNum(curSet.textGen[t].fontName, &fontNum);
                curSet.textGen[t].fontID = fontNum;
                if (fontNum == 0 && !EqualString(sysFontName, curSet.textGen[t].fontName, false, false)) {
                    thisFontMissing = true;
                    if (numMissingFonts) {   // make sure we haven't already got it listed
                        for (i = 0; i < numMissingFonts; i++) {
                            if (EqualString(curSet.textGen[t].fontName, fontName[i], false, false))
                                thisFontMissing = false;
                        }
                    }
                    if (thisFontMissing)
                        BlockMoveData(curSet.textGen[t].fontName, fontName[numMissingFonts++], curSet.textGen[t].fontName[0] + 1);
                }
            }
        }
        if (numMissingFonts > 0) {
            allNames[0] = 0;
            for (i = 0; i < numMissingFonts; i++) {
                if (allNames[0] < 200) {
                    if (allNames[0]) {
                        allNames[++allNames[0]] = ',';
                        allNames[++allNames[0]] = ' ';
                    }
                    BlockMoveData(fontName[i] + 1, allNames + allNames[0] + 1, fontName[i][0]);
                    allNames[0] += fontName[i][0];
                }
            }
            GetIndString(alertMessage, BASERES, 11);
            StandardAlert(kAlertNoteAlert, alertMessage, allNames, nil, &outItemHit);
        }
    }
    EmployPalette(false);
    MySetWindowTitle();
    MarkCurrentPalette();
    ShowSettings();
    for (m = 0; m < maxparticles; m++)
        ptparticle[m].gen = 0;
    if (curSet.setComment[0] > 0 && prefs.showComments) {
        if (!IsMacOSX())
            StopAnimationTimer();
        StandardAlert(kAlertNoteAlert, curSet.windowTitle, curSet.setComment, nil, &outItemHit);
        if (!IsMacOSX())
            UpdateAnimationTimer();
    }
    PTInvalidateWindow(WINCOLORS);
    PTInvalidateWindow(WINSETS);
    PTInvalidateWindow(WINFILTERS);
}

#define ID_NSSAVE 1
#define ID_NSCANCEL 2
#define ID_NSNAME 3
#define ID_NSCOMMENT 4

void InsertNewSetDialog(void) {
    Boolean dialogDone, nameMatch;
    DialogPtr theDialog;
    short c, matchnum, itemHit;
    ControlHandle ctrl;
    Str255 str;
    struct setinformation si;

    StopAutoChanges();
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_NAMESET, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    SetPortDialogPort(theDialog);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItemAsControl(theDialog, ID_NSNAME, &ctrl);
    SetDialogItemText((Handle)ctrl, curSet.windowTitle);
    SelectDialogItemText(theDialog, ID_NSNAME, 0, 32767);
    SetDialogDefaultItem(theDialog, ID_NSSAVE);
    dialogDone = false;
    ConfigureFilter(ID_NSSAVE, ID_NSCANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_NSSAVE:    // OK
                GetDialogItemAsControl(theDialog, ID_NSNAME, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 63) {
                    str[0] = 63;
                    str[63] = '.';
                    str[62] = '.';
                    str[61] = '.';
                }
                nameMatch = false;
                if (gNumSets > 0) {
                    for (c = 0; c < gNumSets; c++) {
                        LoadPixelToySet(&si, c);
                        if (EqualString(str, si.windowTitle, false, false)) {
                            matchnum = c;
                            nameMatch = true;
                        }
                    }
                }
                if (nameMatch) {
                    if (ReplaceDialog()) {
                        nameMatch = false;
                        DoDeleteOldSet(matchnum);
                    }
                    ConfigureFilter(ID_NSSAVE, ID_NSCANCEL);
                }

                if (!nameMatch) {
                    BlockMoveData(str, curSet.windowTitle, str[0] + 1);
                    GetDialogItemAsControl(theDialog, ID_NSCOMMENT, &ctrl);
                    GetDialogItemText((Handle)ctrl, str);
                    BlockMoveData(str, curSet.setComment, str[0] + 1);
                    PopPal();
                    DoInsertNewSet(&curSet);
                    MySetWindowTitle();
                    dialogDone = true;
                }
                break;
            case ID_NSCANCEL:    // Cancel
                dialogDone = true;
                break;
            case ID_NSNAME:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 63) {
                    str[0] = 63;
                    SetDialogItemText((Handle)ctrl, str);
                    SysBeep(1);
                }
                break;
            case ID_NSCOMMENT:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 240) {
                    str[0] = 240;
                    SetDialogItemText((Handle)ctrl, str);
                    SysBeep(1);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
}

void DoInsertNewSet(void *mem) {
    Boolean nameMatch;
    short c;
    struct setinformation si;
    long offset, recordSize, newSize, oldSize;
    Ptr tempPtr;

    struct setinformation *newSet = mem;
    if (gNumSets > 254) {
        SysBeep(1);
        return;
    }
    recordSize = sizeof(struct setinformation);
    newSize = (gNumSets * 2 + 1) * recordSize;
    if (MaxBlock() < newSize) {
        DoStandardAlert(kAlertStopAlert, 20);
        GetPixelToySets(curSetsSpec);
        return;
    }
    setsModified = true;
    if (gNumSets > 0) {
        c = 0;
        nameMatch = false;
        while (!nameMatch && c < gNumSets) {
            LoadPixelToySet(&si, c);
            offset = c * recordSize;
            if (RelString(newSet->windowTitle, si.windowTitle, false, false) == -1) {
                nameMatch = true;
            } else {
                c++;
            }
        }
        if (!nameMatch)
            offset = c * recordSize;
        // insert new set at record c
        oldSize = GetPtrSize(ptSets);
        tempPtr = NewPtr(oldSize);
        if (tempPtr == nil)
            StopError(42, memFullErr);
        BlockMoveData(ptSets, tempPtr, oldSize);
        DisposePtr(ptSets);
        newSize = (gNumSets + 1) * recordSize;
        ptSets = NewPtr(newSize);
        if (ptSets == nil)
            StopError(42, memFullErr);
        // replace any preceding data
        if (offset > 0)
            BlockMoveData(tempPtr, ptSets, offset);
        // replace any following data
        if (c < gNumSets)
            BlockMoveData(tempPtr + offset, ptSets + offset + recordSize, (gNumSets - c) * recordSize);
        if (gSetsMenu)
            InsertMenuItem(gSetsMenu, newSet->windowTitle, c + (NEXTSETID + 1));
        lastSetLoaded = c;
        DisposePtr(tempPtr);
    } else {
        newSize = (gNumSets + 1) * recordSize;
        ptSets = NewPtrClear(newSize);
        if (ptSets == nil)
            StopError(42, memFullErr);
        c = CountMenuItems(gSetsMenu);
        if (gSetsMenu)
            MyEnableMenuItem(gSetsMenu, c);
        if (gSetsMenu)
            SetMenuItemText(gSetsMenu, c, newSet->windowTitle);
        offset = 0;
        lastSetLoaded = c - (NEXTSETID + 1);
    }
    gNumSets++;
    // newSize = size of entire set of sets
    // offset  = beginning of this new record
    BlockMoveData(newSet, ptSets + offset, recordSize);
    SaveCurrentPixelToySets();
    CreateSetsMenu();
    PTInvalidateWindow(WINSETS);
}

void DeleteOldSetDialog(void) {
    short set;
    Boolean done = false;

    if (gNumSets < 1) {
        SysBeep(1);
        return;
    }
    if (gNumSets < 2) {
        DoStandardAlert(kAlertStopAlert, 10);
        return;
    }
    while (!done) {
        set = ChooseSet();
        if (set >= 0) {
            DoDeleteOldSet(set);
            GetPixelToySets(curSetsSpec);
            PTDrawWindow(WINSETS);
            PTUpdateWindow(WINSETS);
        } else {
            done = true;
        }
    }
}

void DoDeleteOldSet(short set) {
    Boolean notFound;
    long newSize, oldSize, delOffset, recordSize;
    short c, numItems;
    Ptr temp;
    Str255 str, menuStr;
    struct setinformation si;

    recordSize = sizeof(struct setinformation);
    if (set >= 0) {
        setsModified = true;
        if (gNumSets > 1) {
            LoadPixelToySet(&si, set);
            // first remove from menu
            numItems = CountMenuItems(gSetsMenu);
            c = NEXTSETID + 2;
            notFound = true;
            while (c <= numItems && notFound) {
                if (gSetsMenu)
                    GetMenuItemText(gSetsMenu, c, menuStr);
                if (EqualString(si.windowTitle, menuStr, false, false)) {
                    if (gSetsMenu)
                        DeleteMenuItem(gSetsMenu, c);
                    notFound = false;
                }
                c++;
            }
            // now delete actual set data
            oldSize = GetPtrSize(ptSets);
            newSize = (gNumSets - 1) * recordSize;
            delOffset = set * recordSize;
            temp = NewPtr(oldSize);
            if (temp == nil)
                StopError(42, memFullErr);
            BlockMoveData(ptSets, temp, oldSize);
            DisposePtr(ptSets);
            ptSets = NewPtr(newSize);
            if (ptSets == nil)
                StopError(42, memFullErr);
            // move any set(s) before deleted spot
            if (set > 0)
                BlockMoveData(temp, ptSets, delOffset);
            // move any set(s) after deleted spot
            numItems = (gNumSets - 1) - set;
            if (numItems > 0)
                BlockMoveData(temp + delOffset + recordSize, ptSets + delOffset, numItems * recordSize);
            DisposePtr(temp);
        } else {
            numItems = CountMenuItems(gSetsMenu);
            GetIndString(str, BASERES, 3);
            if (gSetsMenu)
                SetMenuItemText(gSetsMenu, numItems, str);
            if (gSetsMenu)
                MyDisableMenuItem(gSetsMenu, numItems);
            DisposePtr(ptSets);
        }
        gNumSets--;
        lastSetLoaded = -1;
        PTInvalidateWindow(WINSETS);
    }
}

void LoadDefaultSets(void) {
    FSSpec setsSpec;
    OSErr getSetsErr = GetAppSupportSpecForFilename(CFSTR("Sets.stng"), &setsSpec, true);
    if (fnfErr == getSetsErr)
        getSetsErr = CopyResourceFileToSpec(CFSTR("Default.stng"), &setsSpec);
    if (noErr == getSetsErr)
        GetPixelToySets(setsSpec);
    else
        CreateSetsMenu();
}

void GetPixelToySets(FSSpec spec) {
    char version;
    OSErr err;
    short c, setsFRef, recordSize, curRecordSize;
    long inOutCount;
    Ptr tempPtr;

    if (setInMemory && setsModified)
        PutPixelToySets();
    gCurSetsReadOnly = false;
    err = FSpOpenDF(&spec, fsRdWrPerm, &setsFRef);    // open old sets
    if (err)                                          // maybe it needs to be read-only.
    {
        err = FSpOpenDF(&spec, fsRdPerm, &setsFRef);
        if (!err)
            gCurSetsReadOnly = true;
    }
    if (!err)
        err = GetEOF(setsFRef, &inOutCount);
    if (!err) {
        if (inOutCount == 0) {   // an empty file
            FSClose(setsFRef);
            BlockMoveData(&spec, &curSetsSpec, sizeof(FSSpec));
            gNumSets = 0;
            CreateSetsMenu();
            setInMemory = true;
            setsModified = false;
            lastSetLoaded = -1;
            return;
        }
        tempPtr = NewPtrClear(inOutCount);
        if (tempPtr == nil)
            StopError(43, memFullErr);
    }
    if (!err)
        err = FSRead(setsFRef, &inOutCount, tempPtr);
    if (!err)
        err = FSClose(setsFRef);
    if (!err) {
        version = tempPtr[0];
        gNumSets = (unsigned char)tempPtr[1];
        recordSize = (tempPtr[2] * 256) + (unsigned char)tempPtr[3];
        curRecordSize = sizeof(struct setinformation);
        ptSets = NewPtrClear(curRecordSize * gNumSets);
        if (version == PIXELTOYVERSIONBIGENDIAN || version == 2) {   // current or previous version
            for (c = 0; c < gNumSets; c++) {
                void *aSet = ptSets + (c * curRecordSize);
                BlockMoveData(tempPtr + (c * recordSize) + 4, aSet, recordSize);
                RemapSetBtoN(aSet);
            }
        } else {   // unrecognized version
            err = 1;
            DoStandardAlert(kAlertNoteAlert, 16);
        }
        DisposePtr(tempPtr);
    }
    if (err)
        gNumSets = 0;
    BlockMoveData(&spec, &curSetsSpec, sizeof(FSSpec));
    CreateSetsMenu();
    setInMemory = true;
    setsModified = false;
    lastSetLoaded = -1;
    PTInvalidateWindow(WINSETS);
}

void PutPixelToySets(void) {
    if (gNumSets > 0) {
        SaveCurrentPixelToySets();
        DisposePtr(ptSets);
    }
}

void SaveCurrentPixelToySets(void) {
    OSErr err;
    short setsFRef, recordSize;
    long inOutCount;
    Ptr outPtr;

    if (!setsModified)
        return;
    if (gNumSets > 0) {
        recordSize = sizeof(struct setinformation);
        outPtr = NewPtrClear(4);    // just 4 stinking bytes
        if (outPtr == nil)
            StopError(45, memFullErr);
        outPtr[0] = PIXELTOYVERSIONBIGENDIAN;
        outPtr[1] = gNumSets;
        outPtr[2] = (unsigned char)(recordSize / 256);
        outPtr[3] = (unsigned char)(recordSize & 0x00FF);
        err = FSpOpenDF(&curSetsSpec, fsRdWrPerm, &setsFRef);    // open sets
        if (err) {
            NoteError(46, err);
        } else {
            inOutCount = GetPtrSize(outPtr);
            err = FSWrite(setsFRef, &inOutCount, outPtr);    // write id bytes
            if (err)
                StopError(49, err);
            int c;
            for (c = 0; c < gNumSets; c++) {
                struct setinformation *aSet = ptSets + (c * recordSize);
                RemapSetNtoB(aSet);
            }
            inOutCount = gNumSets * recordSize;
            err = FSWrite(setsFRef, &inOutCount, ptSets);    // write sets
            for (c = 0; c < gNumSets; c++) {
                struct setinformation *aSet = ptSets + (c * recordSize);
                RemapSetBtoN(aSet);
            }
            if (err)
                StopError(49, err);

            err = SetEOF(setsFRef, (gNumSets * recordSize) + 4);
            err = FSClose(setsFRef);
            setsModified = false;
        }
        DisposePtr(outPtr);
    }
}

void CreateSetsMenu(void) {
    Str255 str;
    static short orgNumItems;
    short s;
    struct setinformation si;

    if (!gSetsMenu) {   // haven't loaded the basis yet
        gSetsMenu = GetMenuHandle(SETSMENU);
        orgNumItems = CountMenuItems(gSetsMenu);
    } else {
        s = CountMenuItems(gSetsMenu);
        while (s > orgNumItems) {
            DeleteMenuItem(gSetsMenu, s);
            s = CountMenuItems(gSetsMenu);
        }
    }

    // Add Set entries at bottom of menu
    if (gNumSets < 1) {
        GetIndString(str, BASERES, 3);
        s = CountMenuItems(gSetsMenu);
        AppendMenu(gSetsMenu, str);
        MyDisableMenuItem(gSetsMenu, s + 1);
    } else {
        int itemNum = CountMenuItems(gSetsMenu);
        if (gSetsMenu)
            MyEnableMenuItem(gSetsMenu, 0);
        for (s = 0; s < gNumSets; s++) {
            LoadPixelToySet(&si, s);
            AppendMenuItemText(gSetsMenu, si.windowTitle);
            if (s < 26) {
                SetItemCmd(gSetsMenu, ++itemNum, 'A' + s);
                SetMenuItemModifiers(gSetsMenu, itemNum, (kMenuControlModifier | kMenuNoCommandModifier));
            }
        }
    }

    // Enable/Disable some commands based upon gCurSetsReadOnly status
    gCurSetsReadOnly ? DisableMenuItem(gSetsMenu, ADDSETID) : EnableMenuItem(gSetsMenu, ADDSETID);
    gCurSetsReadOnly ? DisableMenuItem(gSetsMenu, DELSETID) : EnableMenuItem(gSetsMenu, DELSETID);
    gCurSetsReadOnly ? DisableMenuItem(gSetsMenu, RENAMESETID) : EnableMenuItem(gSetsMenu, RENAMESETID);
    gCurSetsReadOnly ? DisableMenuItem(gSetsMenu, EDITCOMMENTSID) : EnableMenuItem(gSetsMenu, EDITCOMMENTSID);

    PTDrawWindow(WINSETS);
}

short ChooseSet(void) {   // returns set (0-x), -1=cancelled
    Boolean dialogDone;
    DialogPtr theDialog;
    MenuHandle setListMenu;
    ControlHandle ctrl;
    short c, itemHit, result = -1;
    struct setinformation si;

    if (gNumSets < 1)
        return -1;
    StopAutoChanges();
    EnsureSystemPalette();
    SaveContext();
    theDialog = GetNewDialog(DLG_DELSET, nil, (WindowPtr)-1);
    // Set up menu
    GetDialogItemAsControl(theDialog, 3, &ctrl);
    setListMenu = GetControlPopupMenuHandle(ctrl);
    if (!setListMenu)
        StopError(8, ResError());
    for (c = 0; c < gNumSets; c++) {
        LoadPixelToySet(&si, c);
        AppendMenuItemText(setListMenu, si.windowTitle);
    }
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    // Finish setting up menu
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, 1);
    GetDialogItemAsControl(theDialog, 2, &ctrl);
    DeactivateControl(ctrl);
    dialogDone = false;
    ConfigureFilter(1, 1);    // return && esc both cancel
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case 1:    // Cancel
                dialogDone = true;
                break;
            case 2:    // Delete
                GetDialogItemAsControl(theDialog, 3, &ctrl);
                result = GetControlValue(ctrl) - 3;
                dialogDone = true;
                break;
            default:    // Probably changed the pop-up menu
                GetDialogItemAsControl(theDialog, 3, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 2) {
                    GetDialogItemAsControl(theDialog, 2, &ctrl);    // activate delete
                    if (!IsControlActive(ctrl))
                        ActivateControl(ctrl);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    return result;
}

void CurSetTouched(void) {
    if ((unsigned char)curSet.windowTitle[curSet.windowTitle[0]] != 0xC9) { // '…'
        curSet.windowTitle[++curSet.windowTitle[0]] = 0xC9;
        MySetWindowTitle();
        PTInvalidateWindow(WINSETS);
    }
}

void SaveSetsAs(void) {
    FSSpec fss;
    OSErr err;

    ResetCursor();
    // saved file name was "Untitled Sets"
    err = CocoaSaveFile(CFSTR("SaveSetsTitle"), CFSTR("SaveSetsMessage"), CFSTR("stng"), (OSType)'BtP3', (OSType)'STNG', &fss);
    if (noErr == err) {
        BlockMoveData(&fss, &curSetsSpec, sizeof(FSSpec));
        setsModified = true;    // force it to save
        PutPixelToySets();
        GetPixelToySets(fss);
    }
}

Boolean SetUsesSoundInput(void *mem) {
    int i;
    struct setinformation *theSet = mem;
    if (theSet->action[ACTSWAVE])
        return true;
    if (theSet->action[ACTLINES] && theSet->actReactSnd[ACTLINES])
        return true;
    if (theSet->action[ACTBALLS] && theSet->actReactSnd[ACTBALLS])
        return true;
    if (theSet->action[ACTSWARM] && theSet->actReactSnd[ACTSWARM])
        return true;
    if (theSet->action[ACTDROPS] && theSet->actReactSnd[ACTDROPS])
        return true;
    if (theSet->action[ACTDOODLE] && theSet->actReactSnd[ACTDOODLE])
        return true;
    if (theSet->paletteSoundMode != PSMODE_NONE)
        return true;
    // Check text items
    if (theSet->action[ACTTEXT]) {
        for (i = 0; i < MAXTEXTS; i++) {
            if (theSet->textGen[i].behave) {
                if (theSet->textGen[i].brightReactSound)
                    return true;
                if (theSet->textGen[i].sizeReactSound)
                    return true;
            }
        }
    }
    // Check particle generators
    if (theSet->action[ACTPARTICLES]) {
        for (i = 0; i < MAXGENERATORS; i++) {
            if (theSet->pg_pbehavior[i]) {
                if (theSet->pg_genaction[i] == 4)
                    return true;
            }
        }
    }
    // Check images
    if (theSet->action[ACTIMAGES]) {
        for (i = 0; i < MAXIMAGES; i++) {
            if (theSet->image[i].active == IMAGEACTIVE || theSet->image[i].active == MOVIEACTIVE) {
                if (theSet->image[i].depth == 1) {
                    if (theSet->image[i].britemode == 2)
                        return true;
                }
            }
        }
    }
    return false;
}

void UpdateCurrentSetNeedsSoundInputMessage(void) {
    gShowNeedsSoundInput = false;
    if (!gCapableSoundIn || prefs.soundInputDevice == 0 || prefs.soundInputSource == 0) {
        gShowNeedsSoundInput = SetUsesSoundInput(&curSet);
    }
}
