// PixelToyPalette.c

#include "PixelToyPalette.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToySound.h"
#include "PixelToyWindows.h"
#include "CocoaBridge.h"

extern struct preferences prefs;
extern struct setinformation curSet;
extern ModalFilterUPP DialogFilterProc;
extern short oldWidth, oldHeight, fromByteCount;
extern short gSysVersionMajor, gSysVersionMinor;
extern CGrafPtr offWorld, doubleWorld, blurWorld;    //, mainWorld;
extern GDHandle windowDevice;
extern CTabHandle mainColorTable, systemColorTable, aboutColorTable;
extern Boolean gDone, gPause, gEditWindowActive, gNoPalTrans;
extern WindowPtr gMainWindow, gAboutWindow, gWindow[MAXWINDOW];
extern Ptr fromBasePtr;
extern MenuHandle gColorsMenu;
extern Rect gImageRect;

GDHandle palTransDevice;
Boolean showingSystem = false;
unsigned char saveRed[256], saveGreen[256], saveBlue[256];
RGBColor fromPalette[256], currentColor[256];
PaletteHandle winPalette;
FSSpec curPalettesFSS;
Handle resChangeBufferHandle;
short resChangeWidth, resChangeHeight;
short numPalettes, lastPaletteMenuItem, palTransRemain = -1;
CGrafPtr palEditWorld;
WindowPtr saveFront;
CTabHandle regAboutColors, unregAboutColors;
struct PTPalette *ptpal;

// Private Functions
void DoInsertThisPalette(Str255 paletteName);
void DoDeletePalette(Str255 killColorName);
short ChoosePalette(Str255 str);

void GetDefaultPalettes(void) {
    FSSpec appSuppPalettesSpec;
    OSErr getPalettesErr = GetAppSupportSpecForFilename(CFSTR("Palettes.cpal"), &appSuppPalettesSpec, true);
    if (fnfErr == getPalettesErr)
        getPalettesErr = CopyResourceFileToSpec(CFSTR("Default.cpal"), &appSuppPalettesSpec);
    if (noErr == getPalettesErr)
        GetPalettes(&appSuppPalettesSpec);
}

OSErr GetPalettes(FSSpecPtr fss) {
    short fRefNum;
    long inOutCount;
    OSErr err;
    int palSize = sizeof(struct PTPalette);

    err = FSpOpenDF(fss, fsRdPerm, &fRefNum);    // open it
    if (err)
        return err;
    err = GetEOF(fRefNum, &inOutCount);
    if (err == noErr && inOutCount >= palSize) {
        numPalettes = 0;
        ptpal = (struct PTPalette *)NewPtrClear(inOutCount);
        if (!ptpal)
            return memFullErr;
        err = FSRead(fRefNum, &inOutCount, (Ptr)ptpal);
        if (err) {
            DisposePtr((Ptr)ptpal);
            return err;
        }
    }
    FSClose(fRefNum);
    numPalettes = inOutCount / palSize;
    BlockMoveData(fss, &curPalettesFSS, sizeof(FSSpec));
    int i;
    for (i = 0; i < numPalettes; i++) {
        RemapPaletteToN(&ptpal[i]);
    }
    PopulateColorsMenu();
    if (gWindow[WINCOLORS]) {
        PTDrawWindow(WINCOLORS);
        PTUpdateWindow(WINCOLORS);
    }
    return noErr;
}

void PutPalettes(void) {
    short fRefNum;
    long inOutCount;
    OSErr err;

    err = FSpOpenDF(&curPalettesFSS, fsRdWrPerm, &fRefNum);    // open it
    if (err == noErr) {
        inOutCount = GetPtrSize((Ptr)ptpal);
        err = FSWrite(fRefNum, &inOutCount, (Ptr)ptpal);
        if (err)
            NoteError(11, err);
        if (!err)
            SetEOF(fRefNum, inOutCount);
        FSClose(fRefNum);
        DisposePtr((Ptr)ptpal);
    } else
        NoteError(11, err);
}

void InitializePalette(void) {
    mainColorTable = GetCTable(BASERES + 2);    // Grey
    systemColorTable = GetCTable(BASERES + 9);
    regAboutColors = GetCTable(BASERES - 1);
    unregAboutColors = GetCTable(BASERES - 2);
    winPalette = NewPalette(254, nil, pmTolerant, 0);
}

void RandomPalette(Boolean singleHue) {
    short stretch;
    short lowPos, hiPos, curPos, mark;
    short rFrac, gFrac, bFrac;
    RGBColor lowColor, hiColor;
    Boolean palDone;

    lowPos = 0;
    lowColor.red = lowColor.green = lowColor.blue = 0;
    curPos = 0;
    palDone = false;
    curSet.palentry[0].red = curSet.palentry[0].green = curSet.palentry[0].blue = 0;
    while (!palDone) {
        hiPos = lowPos + (Random() & 0x007F) + 8;
        if (prefs.roughPalette) {
            if (lowPos > 0 && Random() & 1) {
                hiPos = lowPos + 4;
            }
        }
        if (singleHue)
            hiPos = 255;
        if (hiPos > 250) {
            hiPos = 255;
            palDone = true;
        }
        stretch = hiPos - lowPos;
        hiColor.red = Random();
        hiColor.green = Random();
        hiColor.blue = Random();

        rFrac = (hiColor.red - lowColor.red) / stretch;
        gFrac = (hiColor.green - lowColor.green) / stretch;
        bFrac = (hiColor.blue - lowColor.blue) / stretch;

        for (curPos = lowPos + 1; curPos <= hiPos; curPos++) {
            mark = curPos - lowPos;
            curSet.palentry[curPos].red = (lowColor.red + (rFrac * mark));
            curSet.palentry[curPos].green = (lowColor.green + (gFrac * mark));
            curSet.palentry[curPos].blue = (lowColor.blue + (bFrac * mark));
        }
        lowPos = hiPos;
        lowColor = hiColor;
    }
    mark = CountMenuItems(gColorsMenu);
    for (curPos = FIRSTPALETTEID; curPos <= mark; curPos++) {
        CheckMenuItem(gColorsMenu, curPos, false);
    }
    PTInvalidateWindow(WINCOLORS);
    if (gPause)
        UpdateToMain(false);
}

void EnsureSystemPalette(void) {   // use PopPal to reinstate previous palette afterward
    PushPal();
}

void PushPal(void) {
    short c;

    for (c = 0; c <= 255; c++) {
        saveRed[c] = curSet.palentry[c].red / 256;
        saveGreen[c] = curSet.palentry[c].green / 256;
        saveBlue[c] = curSet.palentry[c].blue / 256;
    }
}

void PopPal(void) {
    short c;

    for (c = 0; c < 256; c++) {
        curSet.palentry[c].red = saveRed[c] * 256;
        curSet.palentry[c].green = saveGreen[c] * 256;
        curSet.palentry[c].blue = saveBlue[c] * 256;
    }
    showingSystem = false;
    EmployPalette(true);
}

void EmployPalette(Boolean instant) {
    short c, depth;
    ColorSpec csa[256];

    if (prefs.palTransFrames < 1)
        instant = true;
    int index = 4;
    UInt16 *values = (UInt16 *)(*mainColorTable);
    for (c = 0; c < 256; c++) {
        index++;
        fromPalette[c].red = values[index++];
        fromPalette[c].green = values[index++];
        fromPalette[c].blue = values[index++];
        csa[c].rgb.red = fromPalette[c].red;
        csa[c].rgb.green = fromPalette[c].green;
        csa[c].rgb.blue = fromPalette[c].blue;
    }
    palTransRemain = prefs.palTransFrames - 1;
    depth = GetWindowDevice(gMainWindow);
    palTransDevice = windowDevice;
    if (depth < 16 || gNoPalTrans)
        instant = true;    //  && !prefs.fullScreen
    if (instant && gMainWindow) {
        palTransRemain = 0;
        PaletteTransitionUpdate();
        if (depth < 16)
            SelectWindow(gMainWindow);
        SetEntries(0, 255, csa);
        SetPalette(gMainWindow, winPalette, false);
        ActivatePalette(gMainWindow);
    }
    if (prefs.windowOpen[WINCOLORED] && gWindow[WINCOLORED]) {
        PTInvalidateWindow(WINCOLORED);
    }
}

void PaletteTransitionUpdate(void) {
    static short lastPaletteSoundMode = 0;
    short volume, depth, step, frac, offset, c;
    Rect cRect;
    RGBColor rotateColor[256];

    // Setup
    if (curSet.paletteSoundMode == PSMODE_NONE && lastPaletteSoundMode != PSMODE_NONE)
        palTransRemain = 0;
    lastPaletteSoundMode = curSet.paletteSoundMode;
    if (palTransRemain < 0 && curSet.paletteSoundMode == PSMODE_NONE) {
        SetForeAndBackColors();
        return;
    }
    step = prefs.palTransFrames - palTransRemain;
    volume = -1;
    if (curSet.paletteSoundMode != PSMODE_NONE) {
        volume = RecentVolume();
        depth = GetWindowDevice(gMainWindow);
    }

    // Build current palette, mid-transition if necessary
    for (c = 0; c < 256; c++) {
        MyGetColor(&currentColor[c], c);

        if (palTransRemain >= 0) {
            frac = (currentColor[c].red - fromPalette[c].red) / prefs.palTransFrames;
            currentColor[c].red = fromPalette[c].red + (frac * step);

            frac = (currentColor[c].green - fromPalette[c].green) / prefs.palTransFrames;
            currentColor[c].green = fromPalette[c].green + (frac * step);

            frac = (currentColor[c].blue - fromPalette[c].blue) / prefs.palTransFrames;
            currentColor[c].blue = fromPalette[c].blue + (frac * step);
        }
        SetEntryColor(winPalette, c, &currentColor[c]);
    }

    // Apply audio effect if necessary
    if (volume > -1 && depth > 8) {
        if (curSet.paletteSoundMode == PSMODE_BLACK || curSet.paletteSoundMode == PSMODE_WHITE ||
            curSet.paletteSoundMode == PSMODE_INVERT) {
            for (c = 0; c < 256; c++) {
                // volume 0 to 128 = black to current palette
                if (curSet.paletteSoundMode == PSMODE_BLACK) {
                    frac = currentColor[c].red / 128;
                    currentColor[c].red = frac * volume;
                    frac = currentColor[c].green / 128;
                    currentColor[c].green = frac * volume;
                    frac = currentColor[c].blue / 128;
                    currentColor[c].blue = frac * volume;
                }
                // volume 0 to 128 = current palette to white
                if (curSet.paletteSoundMode == PSMODE_WHITE) {
                    frac = (65535 - currentColor[c].red) / 128;
                    currentColor[c].red += frac * volume;
                    frac = (65535 - currentColor[c].green) / 128;
                    currentColor[c].green += frac * volume;
                    frac = (65535 - currentColor[c].blue) / 128;
                    currentColor[c].blue += frac * volume;
                }
                // volume 0 to 128 = current palette to inverted version
                if (curSet.paletteSoundMode == PSMODE_INVERT) {
                    frac = ((65535 - currentColor[c].red) - currentColor[c].red) / 128;
                    currentColor[c].red += frac * volume;
                    frac = ((65535 - currentColor[c].green) - currentColor[c].green) / 128;
                    currentColor[c].green += frac * volume;
                    frac = ((65535 - currentColor[c].blue) - currentColor[c].blue) / 128;
                    currentColor[c].blue += frac * volume;
                }
            }
        }
        // entire palette "rotated" based on volume
        if (curSet.paletteSoundMode == PSMODE_ROTATE) {
            BlockMoveData(currentColor, rotateColor, sizeof(RGBColor) * 256);
            for (c = 0; c < 256; c++) {
                offset = (volume * 2) + c;
                while (offset > 255) {
                    offset -= 256;
                }
                currentColor[c] = rotateColor[offset];
            }
        }
    }

    // Install this palette    in mainColorTable
    int index = 4;
    UInt16 *values = (UInt16 *)(*mainColorTable);
    for (c = 0; c < 256; c++) {
        ++index;
        values[index++] = currentColor[c].red;
        values[index++] = currentColor[c].green;
        values[index++] = currentColor[c].blue;
    }
    (*(Handle)mainColorTable)[0]++;    //=Random(); // change ctSeed
    GetPortBounds(offWorld, &cRect);
    UpdateGWorld(&offWorld, 8, &cRect, mainColorTable, nil, clipPix);
    GetPortBounds(blurWorld, &cRect);
    UpdateGWorld(&blurWorld, 8, &cRect, mainColorTable, nil, clipPix);
    GetPortBounds(doubleWorld, &cRect);
    if (prefs.lowQualityMode)
        UpdateGWorld(&doubleWorld, 8, &cRect, mainColorTable, nil, clipPix);
    SetForeAndBackColors();
    palTransRemain--;
}

void UseColorPreset(short theItem, Boolean instantly) {   // -1 means back up, 1 means go forward, else a menu item
    Str255 menuName;
    short i, c, numItems;

    numItems = CountMenuItems(gColorsMenu);
    if (theItem == -1)
        theItem = lastPaletteMenuItem - 1;
    if (theItem == 1)
        theItem = lastPaletteMenuItem + 1;
    if (theItem > numItems)
        theItem = FIRSTPALETTEID;
    if (theItem < (FIRSTPALETTEID))
        theItem = numItems;
    lastPaletteMenuItem = theItem;
    if (gColorsMenu)
        GetMenuItemText(gColorsMenu, theItem, menuName);
    for (i = 0; i < numPalettes; i++) {
        if (EqualString(ptpal[i].palname, menuName, false, false)) {
            for (c = 0; c < 256; c++) {
                curSet.palentry[c].red = ptpal[i].palentry[c].red;
                curSet.palentry[c].green = ptpal[i].palentry[c].green;
                curSet.palentry[c].blue = ptpal[i].palentry[c].blue;
            }
        }
    }
    EmployPalette(instantly);
    MarkCurrentPalette();
    PTInvalidateWindow(WINCOLORS);
    if (gPause)
        UpdateToMain(false);
}

void PopulateColorsMenu(void) {
    short i, numItems;

    if (!gColorsMenu)
        gColorsMenu = GetMenuHandle(COLORSMENU);
    if (!gColorsMenu || !ptpal)
        return;
    numItems = CountMenuItems(gColorsMenu);

    // Clear it out
    for (i = FIRSTPALETTEID; i <= numItems; i++)
        DeleteMenuItem(gColorsMenu, FIRSTPALETTEID);

    // Set it up
    for (i = 0; i < numPalettes; i++) {
        Str255 thePaletteName;
        BlockMove(ptpal[i].palname, thePaletteName, ptpal[i].palname[0] + 1);
        AppendMenuItemText(gColorsMenu, thePaletteName);
    }
    MarkCurrentPalette();
}

void MarkCurrentPalette(void) {
    Boolean match;
    Str255 str;
    short c, col, i, numItems;

    if (!gColorsMenu || !ptpal)
        return;
    numItems = CountMenuItems(gColorsMenu);
    for (i = FIRSTPALETTEID; i <= numItems; i++) {
        CheckMenuItem(gColorsMenu, i, false);
    }
    for (c = 0; c < numPalettes; c++) {
        match = true;
        col = 0;
        while (col < 256 && match) {
            if (curSet.palentry[col].red != ptpal[c].palentry[col].red)
                match = false;
            if (curSet.palentry[col].green != ptpal[c].palentry[col].green)
                match = false;
            if (curSet.palentry[col].blue != ptpal[c].palentry[col].blue)
                match = false;
            col++;
        }
        if (match) {
            numItems = CountMenuItems(gColorsMenu);
            for (i = FIRSTPALETTEID; i <= numItems; i++) {
                GetMenuItemText(gColorsMenu, i, str);
                if (EqualString(str, ptpal[c].palname, false, false)) {
                    CheckMenuItem(gColorsMenu, i, true);
                    lastPaletteMenuItem = i;
                }
            }
        }
    }
}

void DoAddPaletteDialog(void) {
    Boolean dialogDone, nameMatch;
    DialogPtr theDialog;
    short c, itemHit, matchNum;
    ControlHandle ctrl;
    Str255 colorsName;

    EnsureSystemPalette();
    StopAutoChanges();
    theDialog = GetNewDialog(DLG_NAMEPALETTE, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    SetPortDialogPort(theDialog);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SelectDialogItemText(theDialog, 3, 0, 32767);
    SetDialogDefaultItem(theDialog, 1);
    dialogDone = false;
    ConfigureFilter(1, 2);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case 1:    // OK
                GetDialogItemAsControl(theDialog, 3, &ctrl);
                GetDialogItemText((Handle)ctrl, colorsName);
                nameMatch = false;
                for (c = 0; c < numPalettes; c++) {
                    if (EqualString(colorsName, ptpal[c].palname, false, false)) {
                        matchNum = c;
                        nameMatch = true;
                    }
                }
                if (nameMatch) {
                    if (ReplaceDialog()) {
                        nameMatch = false;
                        DoDeletePalette(ptpal[matchNum].palname);
                    }
                }
                if (!nameMatch) {
                    DoInsertThisPalette(colorsName);
                    PTInvalidateWindow(WINCOLORS);
                    dialogDone = true;
                }
                break;
            case 2:    // Cancel
                dialogDone = true;
                break;
            default: break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
}

void DoInsertThisPalette(Str255 paletteName) {
    Boolean here = false;
    short i = 0, c;
    struct PTPalette *temp;
    long size;

    while (i < numPalettes && !here) {
        here = (RelString(paletteName, ptpal[i].palname, false, false) == -1);
        if (!here)
            i++;
    }
    // now insert before palette i
    size = GetPtrSize((Ptr)ptpal);
    temp = (struct PTPalette *)NewPtr(size);
    if (!temp) {
        SysBeep(1);
        return;
    }
    BlockMoveData((Ptr)ptpal, (Ptr)temp, size);
    DisposePtr((Ptr)ptpal);
    ptpal = (struct PTPalette *)NewPtrClear((numPalettes + 1) * sizeof(struct PTPalette));
    if (!ptpal)
        StopError(71, memFullErr);
    if (i > 0) {    // move palettes preceding new palette
        for (c = 0; c < i; c++) {
            BlockMoveData(&temp[c], &ptpal[c], sizeof(struct PTPalette));
        }
    }
    BlockMoveData(paletteName, ptpal[i].palname, paletteName[0] + 1);
    for (c = 0; c < 256; c++) {
        ptpal[i].palentry[c].red = curSet.palentry[c].red;
        ptpal[i].palentry[c].green = curSet.palentry[c].green;
        ptpal[i].palentry[c].blue = curSet.palentry[c].blue;
    }
    if (i < numPalettes) {    // move palettes that were after new palette
        for (c = i; c < numPalettes; c++) {
            BlockMoveData(&temp[c], &ptpal[c + 1], sizeof(struct PTPalette));
        }
    }
    numPalettes++;
    PopulateColorsMenu();
}

#define ID_PRENAME 1
#define ID_PCANCEL 2
#define ID_PPALETTEPOPUP 3
#define ID_PNEWNAME 4

void DoRenamePaletteDialog(short preselected) {   // preselected<0 = none selected
    short itemHit, temp, c;
    MenuHandle paletteListMenu;
    ControlHandle ctrl;
    Rect myRect;
    DialogPtr theDialog;
    Boolean dialogDone;
    Str255 paletteName;

    if (numPalettes < 1) {
        SysBeep(1);
        return;
    }
    StopAutoChanges();
    EnsureSystemPalette();
    SaveContext();
    theDialog = GetNewDialog(DLG_RENAMEPALETTE, nil, (WindowPtr)-1);
    // Get pop-up menu's handle & remove any items in it
    GetDialogItemAsControl(theDialog, ID_PPALETTEPOPUP, &ctrl);
    paletteListMenu = GetControlPopupMenuHandle(ctrl);
    temp = CountMenuItems(paletteListMenu);
    for (c = 1; c <= temp; c++) {
        DeleteMenuItem(paletteListMenu, 1);
    }
    // Put in "Select a palette to rename" and add all set names
    GetIndString(paletteName, BASERES, 44);
    AppendMenu(paletteListMenu, paletteName);
    MyDisableMenuItem(paletteListMenu, 1);
    for (c = 0; c < numPalettes; c++) {
        AppendMenuItemText(paletteListMenu, ptpal[c].palname);
    }
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    GetDialogItemAsControl(theDialog, ID_PNEWNAME, &ctrl);
    DeactivateControl(ctrl);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    GetDialogItem(theDialog, 1, &temp, &ctrl, &myRect);
    DeactivateControl(ctrl);
    SetDialogDefaultItem(theDialog, ID_PRENAME);
    dialogDone = false;
    ConfigureFilter(ID_PRENAME, ID_PCANCEL);
    while (!dialogDone && !gDone) {
        if (preselected > -1) {
            GetDialogItemAsControl(theDialog, ID_PPALETTEPOPUP, &ctrl);
            SetControlValue(ctrl, preselected + 2);
            itemHit = ID_PPALETTEPOPUP;
            preselected = -1;
        } else {
            ModalDialog(DialogFilterProc, &itemHit);
        }
        switch (itemHit) {
            case ID_PRENAME:
                // do rename
                GetDialogItemAsControl(theDialog, ID_PPALETTEPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    GetDialogItemAsControl(theDialog, ID_PNEWNAME, &ctrl);
                    GetDialogItemText((Handle)ctrl, paletteName);
                    if (paletteName[0] > 63) {
                        paletteName[0] = 63;
                        paletteName[63] = 0xC9; // É
                    }
                    BlockMoveData(paletteName, ptpal[c - 2].palname, paletteName[0] + 1);
                    PopulateColorsMenu();
                    PTInvalidateWindow(WINCOLORS);
                    dialogDone = true;
                }
                break;
            case ID_PCANCEL: dialogDone = true; break;
            case ID_PPALETTEPOPUP:
                GetDialogItemAsControl(theDialog, ID_PPALETTEPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 1) {
                    // activate Rename button
                    GetDialogItemAsControl(theDialog, ID_PRENAME, &ctrl);
                    ActivateControl(ctrl);
                    // activate Edit field
                    GetDialogItemAsControl(theDialog, ID_PNEWNAME, &ctrl);
                    ActivateControl(ctrl);
                    SetDialogItemText((Handle)ctrl, ptpal[c - 2].palname);
                    PopulateColorsMenu();
                    SelectDialogItemText(theDialog, ID_PNEWNAME, 0, 32767);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
}

void DeletePaletteDialog(void) {
    short pal = 0;
    Str255 str;

    while (pal != -1) {
        pal = ChoosePalette(str);
        if (pal > 0) {
            DoDeletePalette(str);
            PTDrawWindow(WINCOLORS);
            PTUpdateWindow(WINCOLORS);
        }
    }
}

void DoDeletePalette(Str255 killColorName) {
    short c, numItems;
    struct PTPalette *thisPalette;
    long size;

    if (!killColorName[0])
        return;
    // remove it from ptpal structure
    size = GetPtrSize((Ptr)ptpal);
    thisPalette = (struct PTPalette *)NewPtr(size);
    if (!thisPalette) {
        SysBeep(1);
        return;
    }
    BlockMoveData((Ptr)ptpal, (Ptr)thisPalette, size);
    DisposePtr((Ptr)ptpal);
    ptpal = (struct PTPalette *)NewPtrClear((numPalettes - 1) * sizeof(struct PTPalette));
    if (!ptpal)
        StopError(71, memFullErr);
    numItems = 0;
    for (c = 0; c < numPalettes; c++) {
        if (!EqualString(killColorName, thisPalette[c].palname, false, false)) {
            BlockMoveData(&thisPalette[c], &ptpal[numItems++], sizeof(struct PTPalette));
        }
    }
    numPalettes = numItems;
    DisposePtr((Ptr)thisPalette);

    PopulateColorsMenu();
    PTInvalidateWindow(WINCOLORS);
}

#define ID_CPCANCEL 1
#define ID_CPSELECT 2
#define ID_CPPOPUP 3
#define ID_CPTITLE 4

short ChoosePalette(Str255 str) {   // returns palette #(1-x), -1 = cancelled
    Boolean dialogDone;
    DialogPtr theDialog;
    ControlHandle ctrl;
    MenuHandle colorListMenu;
    short c, itemHit, returnValue;

    returnValue = -1;
    StopAutoChanges();
    EnsureSystemPalette();
    SaveContext();
    theDialog = GetNewDialog(DLG_SELCOLORSET, nil, (WindowPtr)-1);
    // set up menu
    GetDialogItemAsControl(theDialog, ID_CPPOPUP, &ctrl);
    colorListMenu = GetControlPopupMenuHandle(ctrl);
    if (!colorListMenu)
        StopError(12, ResError());
    for (c = 0; c < numPalettes; c++) {
        AppendMenuItemText(colorListMenu, ptpal[c].palname);
        if (EqualString(ptpal[c].palname, "\pGrey", false, false)) {
            MyDisableMenuItem(colorListMenu, c + 3);
        }
    }
    SetControlMaximum(ctrl, 32767);
    SetControlValue(ctrl, 1);
    // finished setting up menu
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    GetDialogItemAsControl(theDialog, ID_CPSELECT, &ctrl);
    DeactivateControl(ctrl);
    SetPortDialogPort(theDialog);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_CPCANCEL);
    dialogDone = false;
    ConfigureFilter(ID_CPCANCEL, ID_CPCANCEL);    // return & esc both cancel
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_CPCANCEL: dialogDone = true; break;
            case ID_CPSELECT:
                GetDialogItemAsControl(theDialog, ID_CPPOPUP, &ctrl);
                returnValue = GetControlValue(ctrl);
                GetMenuItemText(colorListMenu, returnValue, str);
                dialogDone = true;
                break;
            case ID_CPPOPUP:
                GetDialogItemAsControl(theDialog, ID_CPPOPUP, &ctrl);
                c = GetControlValue(ctrl);
                if (c > 2) {
                    GetDialogItemAsControl(theDialog, ID_CPSELECT, &ctrl);
                    ActivateControl(ctrl);
                }
                break;
        }
    }
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    return returnValue;
}

void ResChangeBitsPush(void) {
    short y;
    long numBytes, offset1, offset2;
    Rect offRect;

    if (LockAllWorlds()) {
        GetPortBounds(offWorld, &offRect);
        resChangeWidth = offRect.right - offRect.left;
        resChangeHeight = offRect.bottom - offRect.top;
        if (!resChangeBufferHandle) {
            numBytes = resChangeWidth * resChangeHeight;
            resChangeBufferHandle = NewHandle(numBytes);
            if (!resChangeBufferHandle) {
                OSErr err;
                resChangeBufferHandle = TempNewHandle(numBytes, &err);
                if (!resChangeBufferHandle || err != noErr) {
                    return;
                }
            }
            HLock(resChangeBufferHandle);
            offset1 = 0;
            offset2 = 0;
            for (y = 0; y < resChangeHeight; y++) {
                BlockMoveData(fromBasePtr + offset1, (*resChangeBufferHandle) + offset2, resChangeWidth);
                offset1 += fromByteCount;
                offset2 += resChangeWidth;
            }
        }
    }
    UnlockAllWorlds();
}

void ResChangeBitsPop(void) {
    float sizeX, sizeY;
    short x, y;
    long offset1, offset2;

    if (LockAllWorlds()) {
        if (resChangeBufferHandle) {   // buffer exists, so ...
            if ((resChangeWidth == prefs.winxsize) && (resChangeHeight == prefs.winysize)) {   // same size, just copy data
                offset1 = 0;
                offset2 = 0;
                for (y = 0; y < prefs.winysize; y++) {
                    BlockMoveData((*resChangeBufferHandle) + offset2, fromBasePtr + offset1, prefs.winxsize);
                    offset1 += fromByteCount;
                    offset2 += prefs.winxsize;
                }
            } else {   // different size, stretch/squish to new size
                sizeX = (float)resChangeWidth / (float)prefs.winxsize;
                sizeY = (float)resChangeHeight / (float)prefs.winysize;
                offset1 = 0;
                for (y = 0; y < prefs.winysize; y++) {
                    offset2 = (long)(y * sizeY) * resChangeWidth;
                    for (x = 0; x < prefs.winxsize; x++) {
                        fromBasePtr[offset1 + x] = (*resChangeBufferHandle)[(long)(offset2 + (long)(x * sizeX))];
                    }
                    offset1 += fromByteCount;
                }
            }
            HUnlock(resChangeBufferHandle);
            DisposeHandle(resChangeBufferHandle);
            TempFreeMem();
            resChangeBufferHandle = nil;
        } else {   // no buffer exists, so just clear out new pixels.
            offset1 = 0;
            for (y = 0; y < prefs.winysize; y++) {
                for (x = 0; x < prefs.winxsize; x++) {
                    fromBasePtr[offset1 + x] = 0;
                }
                offset1 += fromByteCount;
            }
        }
    }
    UnlockAllWorlds();
}

void ResChangeBitsRelease(void) {
    if (resChangeBufferHandle) {
        DisposeHandle(resChangeBufferHandle);
        resChangeBufferHandle = nil;
    }
}
