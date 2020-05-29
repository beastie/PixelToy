// PixelToyAutoPilot.c

#include "PixelToyAutoPilot.h"

#include "Defs&Structs.h"
#include "PIxelToyMain.h"
#include "PIxelToyBitmap.h"
#include "PixelToyFilters.h"
#include "PixelToySets.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToyParticles.h"
#include "PixelToyText.h"
#include "PixelToyWaves.h"

extern struct preferences prefs;
extern struct setinformation curSet;
extern ModalFilterUPP DialogFilterProc;
extern PaletteHandle winPalette;
extern MenuHandle gColorsMenu;
extern char particleGenerator[];
extern Boolean gDone;

Boolean storeAutopilot, storeAutosetchange;
short fontList[1024], numFonts;

#define MAXEXCLUSIONS 64
void BuildFontList(void) {
    Boolean isExcl;
    short e, i, index, numExclFonts;
    MenuHandle textFontMenu;
    Handle exclFonts;
    Str255 str;
    Str63 exclFont[MAXEXCLUSIONS];

    // Determine # of excluded fonts
    exclFonts = GetResource('STR#', BASERES + 6);
    LoadResource(exclFonts);
    numExclFonts = (unsigned char)(*(*exclFonts + 1));
    ReleaseResource(exclFonts);
    if (numExclFonts > MAXEXCLUSIONS)
        numExclFonts = MAXEXCLUSIONS;
    // Load them
    for (i = 0; i < numExclFonts; i++) {
        GetIndString(exclFont[i], BASERES + 6, i + 1);
    }
    // Build list of font IDs on this system, excluding abovementioned fonts
    textFontMenu = GetMenu(1002);
    AppendResMenu(textFontMenu, 'FOND');
    numFonts = CountMenuItems(textFontMenu);
    if (numFonts > 1024)
        numFonts = 1024;
    index = 0;
    for (i = 1; i <= numFonts; i++) {
        GetMenuItemText(textFontMenu, i, str);
        isExcl = false;
        for (e = 0; e < numExclFonts; e++) {
            if (EqualString(exclFont[e], str, false, false))
                isExcl = true;
        }
        if (!isExcl)
            GetFNum(str, &fontList[index++]);
    }
    numFonts = index;
}

#define ID_CA_SAVE 1
#define ID_CA_CANCEL 2
#define ID_CA_DEFAULTS 3
#define ID_CA_NUMSECONDS 5
#define ID_CA_ACTIONS 6
#define ID_CA_ACTOPTIONS 7
#define ID_CA_ACTSOUND 8
#define ID_CA_ACTALWAYSSOUND 14
#define ID_CA_FILTERS 9
#define ID_CA_MIRRORS 15
#define ID_CA_RANDCOLORS 10
#define ID_CA_SAVEDCOLORS 13
#define ID_CA_ACTFIRST 16
#define ID_CA_ACTOPTFIRST 25

void ConfigureAutopilotDialog(void) {
    Boolean dialogDone;
    DialogPtr theDialog;
    ControlHandle ctrl;
    Rect myRect;
    Str255 str;
    long number;
    short itemHit, temp;
    struct preferences backup;

    StopAutoChanges();
    BlockMoveData(&prefs, &backup, sizeof(struct preferences));
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_AUTOPILOT, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    if (prefs.windowPosition[WINAUTOPILOT].h != 0 || prefs.windowPosition[WINAUTOPILOT].v != 0)
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINAUTOPILOT].h, prefs.windowPosition[WINAUTOPILOT].v, false);
    SetPortDialogPort(theDialog);
    ResetCursor();
    ShowAutoPilotSettings(theDialog);
    SelectDialogItemText(theDialog, ID_CA_NUMSECONDS, 0, 32767);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_CA_SAVE);
    dialogDone = false;
    ConfigureFilter(ID_CA_SAVE, ID_CA_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_CA_SAVE: dialogDone = true; break;
            case ID_CA_CANCEL:
                BlockMoveData(&backup, &prefs, sizeof(struct preferences));
                dialogDone = true;
                break;
            case ID_CA_DEFAULTS:
                SetAutoPilotDefaults();
                SelectDialogItemText(theDialog, ID_CA_NUMSECONDS, 0, 32767);
                ShowAutoPilotSettings(theDialog);
                break;
            case ID_CA_NUMSECONDS:
                GetDialogItemAsControl(theDialog, ID_CA_NUMSECONDS, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                StringToNum(str, &number);
                if (number < 1)
                    number = 1;
                if (number > 999)
                    number = 999;
                prefs.autoPilotValue[APN_NUMSECONDS] = (short)number;
                break;
            // Action buttons
            case ID_CA_ACTFIRST:
            case ID_CA_ACTFIRST + 1:
            case ID_CA_ACTFIRST + 2:
            case ID_CA_ACTFIRST + 3:
            case ID_CA_ACTFIRST + 4:
            case ID_CA_ACTFIRST + 5:
            case ID_CA_ACTFIRST + 6:
            case ID_CA_ACTFIRST + 7:
            case ID_CA_ACTFIRST + 8:
                temp = itemHit - ID_CA_ACTFIRST;
                SetAPAction(temp, !GetAPAction(temp));
                ShowAutoPilotSettings(theDialog);
                break;
            // Action Option buttons
            case ID_CA_ACTOPTFIRST:
            case ID_CA_ACTOPTFIRST + 1:
            case ID_CA_ACTOPTFIRST + 2:
            case ID_CA_ACTOPTFIRST + 3:
            case ID_CA_ACTOPTFIRST + 4:
            case ID_CA_ACTOPTFIRST + 5:
            case ID_CA_ACTOPTFIRST + 6:
            case ID_CA_ACTOPTFIRST + 7:
            case ID_CA_ACTOPTFIRST + 8:
                temp = itemHit - ID_CA_ACTOPTFIRST;
                SetAPOption(temp, !GetAPOption(temp));
                ShowAutoPilotSettings(theDialog);
                break;
            // Checkboxes
            case ID_CA_ACTIONS:
            case ID_CA_ACTOPTIONS:
            case ID_CA_ACTSOUND:
            case ID_CA_ACTALWAYSSOUND:
            case ID_CA_FILTERS:
            case ID_CA_MIRRORS:
            case ID_CA_RANDCOLORS:
            case ID_CA_SAVEDCOLORS:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                temp = !GetControlValue(ctrl);
                if (itemHit == ID_CA_ACTIONS)
                    prefs.autoPilotOn[APB_ACTIONS] = temp;
                if (itemHit == ID_CA_ACTOPTIONS)
                    prefs.autoPilotOn[APB_OPTIONS] = temp;
                if (itemHit == ID_CA_ACTSOUND)
                    prefs.autoPilotOn[APB_ACTSOUND] = temp;
                if (itemHit == ID_CA_ACTALWAYSSOUND)
                    prefs.autoPilotOn[APB_ALWAYSSOUND] = temp;
                if (itemHit == ID_CA_FILTERS)
                    prefs.autoPilotOn[APB_FILTERS] = temp;
                if (itemHit == ID_CA_MIRRORS)
                    prefs.autoPilotOn[APB_MIRRORS] = temp;
                if (itemHit == ID_CA_RANDCOLORS)
                    prefs.autoPilotOn[APB_RANDCOLORS] = temp;
                if (itemHit == ID_CA_SAVEDCOLORS)
                    prefs.autoPilotOn[APB_SAVEDCOLORS] = temp;
                ShowAutoPilotSettings(theDialog);
                GetDialogItemAsControl(theDialog, ID_CA_SAVE, &ctrl);
                if (!prefs.autoPilotOn[APB_ACTIONS] && !prefs.autoPilotOn[APB_OPTIONS] && !prefs.autoPilotOn[APB_FILTERS] &&
                    !prefs.autoPilotOn[APB_MIRRORS] && !prefs.autoPilotOn[APB_RANDCOLORS] && !prefs.autoPilotOn[APB_SAVEDCOLORS]) {
                    DeactivateControl(ctrl);
                } else {
                    ActivateControl(ctrl);
                }
        }
    }
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINAUTOPILOT].h = myRect.left;
    prefs.windowPosition[WINAUTOPILOT].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINAUTOPILOT]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen)
        MyHideMenuBar();
}

void ShowAutoPilotSettings(DialogPtr theDialog) {
    short i;
    Boolean active;
    ControlHandle ctrl;
    Str255 str;
    ControlButtonContentInfo cbci;

    GetDialogItemAsControl(theDialog, ID_CA_NUMSECONDS, &ctrl);
    NumToString(prefs.autoPilotValue[APN_NUMSECONDS], str);
    SetDialogItemText((Handle)ctrl, str);

    // Actions on/off master check box
    GetDialogItemAsControl(theDialog, ID_CA_ACTIONS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_ACTIONS]);
    active = GetControlValue(ctrl);

    for (i = ACTLINES; i <= ACTIMAGES; i++) {
        GetDialogItemAsControl(theDialog, ID_CA_ACTFIRST + i, &ctrl);
        cbci.contentType = kControlContentPictRes;
        cbci.u.resID = 500 + i + (GetAPAction(i) * 100);
        SetBevelButtonContentInfo(ctrl, &cbci);
        if (active) {
            ActivateControl(ctrl);
        } else {
            DeactivateControl(ctrl);
        }
    }

    // Actions Options master check box
    GetDialogItemAsControl(theDialog, ID_CA_ACTOPTIONS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_OPTIONS]);
    active = GetControlValue(ctrl);

    for (i = ACTLINES; i <= ACTIMAGES; i++) {
        GetDialogItemAsControl(theDialog, ID_CA_ACTOPTFIRST + i, &ctrl);
        cbci.contentType = kControlContentPictRes;
        cbci.u.resID = 500 + i + (GetAPOption(i) * 100);
        SetBevelButtonContentInfo(ctrl, &cbci);
        if (active) {
            ActivateControl(ctrl);
        } else {
            DeactivateControl(ctrl);
        }
    }

    // Other misc checkboxes
    GetDialogItemAsControl(theDialog, ID_CA_ACTSOUND, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_ACTSOUND]);
    active = GetControlValue(ctrl);

    GetDialogItemAsControl(theDialog, ID_CA_ACTALWAYSSOUND, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_ALWAYSSOUND]);
    if (active) {
        ActivateControl(ctrl);
    } else {
        DeactivateControl(ctrl);
    }

    GetDialogItemAsControl(theDialog, ID_CA_FILTERS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_FILTERS]);
    GetDialogItemAsControl(theDialog, ID_CA_MIRRORS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_MIRRORS]);
    GetDialogItemAsControl(theDialog, ID_CA_RANDCOLORS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_RANDCOLORS]);
    GetDialogItemAsControl(theDialog, ID_CA_SAVEDCOLORS, &ctrl);
    SetControlValue(ctrl, prefs.autoPilotOn[APB_SAVEDCOLORS]);
}

void DoAutoPilot(Boolean force) {
    Boolean didSomething = false, gotOne = false;
    short i, c, s, tries, numActive, stageWidth, stageHeight;
    static long nextAuto = 0;
    float scaler;
    Rect rect;

    if (!prefs.autoPilotActive)
        return;

    if (force || TickCount() > nextAuto) {
        stageWidth = prefs.winxsize;
        stageHeight = prefs.winysize;
        if (curSet.verticalMirror && curSet.constrainMirror)
            stageWidth = (prefs.winxsize / 2) + 1;
        if (curSet.horizontalMirror && curSet.constrainMirror)
            stageHeight = (prefs.winysize / 2) + 1;
        nextAuto = TickCount() + (prefs.autoPilotValue[APN_NUMSECONDS] * 60);
        CurSetTouched();
        numActive = 0;
        for (i = 0; i < MAXACTION; i++) {
            if (i != ACTDOODLE)
                numActive += curSet.action[i];
        }
        tries = 20;
        while (!didSomething && tries--) {
            i = RandNum(0, 6);
            if (numActive == 0 && prefs.autoPilotOn[APB_ACTIONS])
                i = 0;
            switch (i) {
                case 0:    // Action
                    if (!prefs.autoPilotOn[APB_ACTIONS])
                        break;
                    s = 20;
                    gotOne = false;
                    while (!gotOne && s--) {
                        i = RandNum(0, ACTIMAGES + 1);
                        gotOne = true;
                        if (numActive > 2 && !curSet.action[i])
                            gotOne = false;
                        if (!GetAPAction(i))
                            gotOne = false;
                        if (i == ACTSWAVE && !prefs.autoPilotOn[APB_ACTSOUND])
                            gotOne = false;
                    }
                    if (gotOne) {
                        didSomething = true;
                        curSet.action[i] = !curSet.action[i];
                        if (i == ACTPARTICLES && curSet.action[i])
                            ResetParticles();
                    }
                    break;
                case 1:    // Action Options
                    if (!prefs.autoPilotOn[APB_OPTIONS])
                        break;
                    s = 20;
                    gotOne = false;
                    while (!gotOne && s--) {
                        i = RandNum(0, ACTIMAGES + 1);
                        gotOne = (curSet.action[i]);
                        if (!GetAPOption(i))
                            gotOne = false;
                    }
                    if (!gotOne)
                        break;
                    switch (i) {
                        case ACTLINES:
                        case ACTBALLS:
                        case ACTDROPS:
                        case ACTSWARM:
                            didSomething = true;
                            switch (RandNum(0, 3)) {
                                case 0: curSet.actCount[i] = RandNum(1, 100 - (curSet.actSiz[i] * 2)); break;
                                case 1: curSet.actSiz[i] = RandNum(1, 100 - (curSet.actCount[i] * 2)); break;
                                case 2: curSet.actSizeVar[i] = RandNum(1, 100); break;
                            }
                            curSet.actReactSnd[i] = false;
                            if (prefs.autoPilotOn[APB_ACTSOUND])
                                curSet.actReactSnd[i] = RandNum(0, 2);
                            break;
                        case ACTSWAVE:
                            didSomething = true;
                            c = 0;
                            for (i = 0; i < MAXWAVES; i++) {
                                c += curSet.wave[i].active;
                            }
                            if (c == 0)
                                CreateNewWave();
                            i = -1;
                            while (i == -1) {
                                i = RandNum(0, MAXWAVES);
                                if (!curSet.wave[i].active)
                                    i = -1;
                            }
                            switch (RandNum(0, 10)) {
                                case 0:
                                    if (RandNum(0, 2) == 0)
                                        curSet.wave[i].action = RandNum(0, 5);    // not including mouse
                                    else
                                        curSet.wave[i].action = 0;    // 50% of the time just do stationary
                                    break;
                                case 1: curSet.wave[i].type = RandNum(0, 4); break;
                                case 2: curSet.wave[i].vertical = !curSet.wave[i].vertical; break;
                                case 3: curSet.wave[i].horizflip = !curSet.wave[i].horizflip; break;
                                case 4: curSet.wave[i].vertflip = !curSet.wave[i].vertflip; break;
                                case 5: curSet.wave[i].solid = !curSet.wave[i].solid; break;
                                case 6: curSet.wave[i].brightness = RandNum(20, 101); break;
                                case 7: curSet.wave[i].spacing = RandNum(1, 101); break;
                                case 8: curSet.wave[i].thickness = RandNum(1, 40); break;
                                case 9:    // size/shape
                                    SetRect(&rect, RandNum(0, 16384), RandNum(0, 16384), RandNum(0, 16384), RandNum(0, 16384));
                                    if (rect.right < rect.left) {
                                        c = rect.left;
                                        rect.left = rect.right;
                                        rect.right = c;
                                    }
                                    if (rect.bottom < rect.top) {
                                        c = rect.top;
                                        rect.top = rect.bottom;
                                        rect.bottom = c;
                                    }
                                    if (rect.right < (rect.left + 3000))
                                        rect.right = rect.left + 3000;
                                    if (rect.bottom < (rect.top + 3000))
                                        rect.bottom = rect.top + 3000;
                                    curSet.wave[i].place = rect;
                                    break;
                            }
                            // keep vertical things tall, otherwise wide
                            rect = curSet.wave[i].place;
                            gotOne = false;
                            if ((rect.right - rect.left) > (rect.bottom - rect.top)) {
                                if (curSet.wave[i].vertical)
                                    gotOne = true;
                            } else {
                                if (!curSet.wave[i].vertical)
                                    gotOne = true;
                            }
                            if (gotOne) {
                                c = rect.right - rect.left;
                                s = rect.bottom - rect.top;
                                rect.right = rect.left + s;
                                rect.bottom = rect.top + c;
                            }
                            // force this visual's rect onscreen
                            if (rect.left < 0)
                                OffsetRect(&rect, -rect.left, 0);
                            if (rect.top < 0)
                                OffsetRect(&rect, 0, -rect.top);
                            if (rect.right > 16384)
                                OffsetRect(&rect, -(rect.right - 16384), 0);
                            if (rect.bottom > 16384)
                                OffsetRect(&rect, 0, -(rect.bottom - 16384));
                            curSet.wave[i].place = rect;
                            break;
                        case ACTTEXT:
                            didSomething = true;
                            c = 0;
                            for (i = 0; i < MAXTEXTS; i++) {
                                c += (curSet.textGen[i].behave > 0);
                            }
                            if (c == 0)
                                CreateNewText(false);
                            i = -1;
                            while (i == -1) {
                                i = RandNum(0, MAXTEXTS);
                                if (!curSet.textGen[i].behave)
                                    i = -1;
                            }
                            switch (RandNum(0, 5)) {
                                case 0:
                                    curSet.textGen[i].xpos = RandNum(0, 32000);
                                    curSet.textGen[i].ypos = RandNum(0, 32000);
                                    ForceTextVisible(&curSet.textGen[i], FTV_MOVE);
                                    break;
                                case 1:
                                    s = 6;    // move by mouse, not allowed
                                    while (s == 6) {
                                        s = RandNum(1, 8);
                                    }
                                    curSet.textGen[i].behave = s;
                                    break;
                                case 2:
                                    curSet.textGen[i].fontID = fontList[RandNum(0, numFonts)];
                                    GetFontName(curSet.textGen[i].fontID, curSet.textGen[i].fontName);
                                    ForceTextVisible(&curSet.textGen[i], FTV_MOVE);
                                    break;
                                case 3:
                                    TextFont(curSet.textGen[i].fontID);
                                    s = 12;
                                    scaler = 100.0 / stageHeight;
                                    TextSize(s / scaler);
                                    tries = -1;
                                    while (tries == -1) {
                                        c = StringWidth(curSet.textGen[i].string);
                                        if (c < stageWidth) {
                                            s += 4;
                                        } else {
                                            tries++;
                                        }
                                        if (s > 200) {
                                            s = 200;
                                            tries++;
                                        }
                                        TextSize(s / scaler);
                                    }
                                    curSet.textGen[i].size = RandNum(8, s);
                                    curSet.textGen[i].sizeReactSound = false;
                                    if (prefs.autoPilotOn[APB_ACTSOUND] && RandNum(0, 2))
                                        curSet.textGen[i].sizeReactSound = true;
                                    ForceTextVisible(&curSet.textGen[i], FTV_MOVE);
                                    break;
                                case 4:
                                    curSet.textGen[i].brightPulse = false;
                                    curSet.textGen[i].brightReactSound = false;
                                    switch (RandNum(0, 2 + prefs.autoPilotOn[APB_ACTSOUND])) {
                                        case 0: curSet.textGen[i].brightness = RandNum(0, 101); break;
                                        case 1: curSet.textGen[i].brightPulse = true; break;
                                        case 2: curSet.textGen[i].brightReactSound = true; break;
                                    }
                                    break;
                            }
                            break;
                        case ACTPARTICLES:
                            didSomething = true;
                            c = 0;
                            for (i = 0; i < MAXGENERATORS; i++) {
                                c += (curSet.pg_pbehavior[i] > 0);
                            }
                            if (c == 0)
                                CreateNewGenerator();
                            i = -1;
                            while (i == -1) {
                                i = RandNum(0, MAXGENERATORS);
                                if (!curSet.pg_pbehavior[i])
                                    i = -1;
                            }
                            switch (RandNum(0, 12)) {
                                case 0:
                                    c = -1;
                                    while (c == -1) {
                                        c = RandNum(1, 4);
                                        if (c == 3 /* && RandNum(0,3)*/)
                                            c = -1;
                                    }
                                    if (c == 3 && curSet.pg_rate[i] > 10)
                                        curSet.pg_rate[i] = 10;
                                    curSet.pg_pbehavior[i] = c;
                                    break;
                                case 1:
                                    c = -1;
                                    while (c == -1) {
                                        c = RandNum(1, 5);
                                        if (c == 2)
                                            c = -1;    // no follow mouse
                                        if (!prefs.autoPilotOn[APB_ACTSOUND] && c == 4)
                                            c = -1;
                                    }
                                    curSet.pg_genaction[i] = c;
                                    break;
                                case 2:
                                    if (curSet.pg_pbehavior[i] != 4)
                                        curSet.pg_solid[i] = 255 - RandNum(0, 2);
                                    break;
                                case 3: curSet.pg_rate[i] = RandNum(1, 100); break;
                                case 4: curSet.pg_size[i] = RandNum(0, 20); break;
                                case 5: curSet.pg_gravity[i] = RandNum(1, 100); break;
                                case 6: curSet.pg_walls[i] = RandNum(0, 2); break;
                                case 7:
                                    curSet.pg_xloc[i] = RandNum(4000, 28000);
                                    curSet.pg_yloc[i] = RandNum(4000, 28000);
                                    break;
                                case 8:
                                    curSet.pg_xlocv[i] = RandNum(0, 20);
                                    curSet.pg_ylocv[i] = RandNum(0, 20);
                                    break;
                                case 9:
                                    curSet.pg_dx[i] = RandVariance(1000);
                                    curSet.pg_dy[i] = RandVariance(1000);
                                    break;
                                case 10: curSet.pg_dxv[i] = curSet.pg_dyv[i] = RandNum(0, 100); break;
                                case 11: curSet.pg_brightness[i] = RandNum(10, 100); break;
                            }
                            break;

                        case ACTIMAGES:
                            c = 0;
                            for (i = 0; i < MAXIMAGES; i++) {
                                c += (curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE);
                            }
                            if (c == 0)
                                CreateNewImage();
                            i = -1;
                            while (i == -1) {
                                i = RandNum(0, MAXIMAGES);
                                if (!(curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE))
                                    i = -1;
                            }
                            switch (RandNum(0, 8)) {
                                case 0:
                                    didSomething = true;
                                    s = 5;    // move by mouse, not allowed
                                    while (s == 5) {
                                        s = RandNum(0, 7);
                                    }
                                    curSet.image[i].action = s;
                                    break;
                                case 1:
                                    didSomething = true;
                                    curSet.image[i].invert = !curSet.image[i].invert;
                                    break;
                                case 2:
                                    didSomething = true;
                                    curSet.image[i].drawAfterActions = !curSet.image[i].drawAfterActions;
                                    break;
                                case 3:
                                    didSomething = true;
                                    if (curSet.image[i].depth == 8) {
                                        curSet.image[i].depth = 1;
                                    } else {
                                        curSet.image[i].depth = 8;
                                    }
                                    break;
                                case 4:
                                    if (curSet.image[i].depth == 1) {
                                        didSomething = true;
                                        curSet.image[i].britemode = RandNum(0, 2 + prefs.autoPilotOn[APB_ACTSOUND]);
                                    }
                                    break;
                                case 5:
                                    if (curSet.image[i].depth == 1) {
                                        didSomething = true;
                                        curSet.image[i].brightness = RandNum(10, 101);
                                    }
                                    break;
                                // placement
                                case 6:
                                    didSomething = true;
                                    rect = curSet.image[i].place;
                                    OffsetRect(&rect, -rect.left, -rect.top);
                                    OffsetRect(&rect, RandNum(0, 16384 - rect.right), RandNum(0, 16384 - rect.bottom));
                                    curSet.image[i].place = rect;
                                    break;
                                // size
                                case 7:
                                    if (curSet.image[i].orgheight > 0 && stageWidth > 0 && stageHeight > 0) {
                                        long newHeight;
                                        short maxWidth = 16384 - curSet.image[i].place.left;
                                        short maxHeight = 16384 - curSet.image[i].place.top;
                                        didSomething = true;
                                        scaler = (float)curSet.image[i].orgwidth / (float)curSet.image[i].orgheight;
                                        scaler /= ((float)stageWidth / (float)stageHeight);
                                        rect.left = rect.top = 0;
                                        rect.right = RandNum(1000, maxWidth);
                                        if (prefs.imagesNotProportional)
                                            newHeight = RandNum(1000, maxHeight);
                                        else
                                            newHeight = (float)rect.right / scaler;
                                        if (newHeight > maxHeight) {
                                            newHeight = maxHeight;
                                            rect.right = newHeight * scaler;
                                        }
                                        rect.bottom = (short)newHeight;
                                        OffsetRect(&rect, curSet.image[i].place.left, curSet.image[i].place.top);
                                        curSet.image[i].place = rect;
                                    }
                                    break;
                            }
                            break;
                    }
                    break;
                case 2:    // Filter
                    if (!prefs.autoPilotOn[APB_FILTERS])
                        break;
                    didSomething = true;
                    i = RandNum(0, 23);
                    ClearAllFilters();
                    ToggleFilter(i);
                    curSet.postFilter = (i == 17);    // after-effect if kaleidoscope
                    if (i == 11) {                    // zoom out, make balls & drops stay on screen
                        curSet.actMisc1[ACTBALLS] = true;
                        curSet.actMisc1[ACTDROPS] = true;
                    }
                    break;
                case 3:    // Mirrors
                    if (!prefs.autoPilotOn[APB_MIRRORS])
                        break;
                    didSomething = true;
                    c = RandNum(0, 2 + (curSet.horizontalMirror || curSet.verticalMirror));
                    ToggleMirror(c);
                    if (!curSet.horizontalMirror && !curSet.verticalMirror && curSet.constrainMirror)
                        ToggleMirror(2);
                    break;
                case 4:    // Random Color
                    if (!prefs.autoPilotOn[APB_RANDCOLORS])
                        break;
                    didSomething = true;
                    RandomPalette(curSet.emboss);
                    EmployPalette(false);
                    break;
                case 5:    // Saved Color
                    if (!prefs.autoPilotOn[APB_SAVEDCOLORS])
                        break;
                    numActive = CountMenuItems(gColorsMenu) - FIRSTPALETTEID;
                    if (numActive < 2)
                        break;
                    didSomething = true;
                    c = RandNum(0, numActive + 1);
                    UseColorPreset(c + FIRSTPALETTEID, false);
                    break;
            }
            if (!prefs.autoPilotOn[APB_ACTSOUND]) {  // turn all react to sound options OFF
                curSet.actReactSnd[ACTLINES] = false;
                curSet.actReactSnd[ACTBALLS] = false;
                curSet.actReactSnd[ACTDROPS] = false;
                curSet.actReactSnd[ACTSWARM] = false;
                curSet.actReactSnd[ACTDOODLE] = false;
                curSet.action[ACTSWAVE] = false;
                for (i = 0; i < MAXTEXTS; i++) {
                    curSet.textGen[i].sizeReactSound = false;
                    curSet.textGen[i].brightReactSound = false;
                }
                for (i = 0; i < MAXGENERATORS; i++) {
                    while (curSet.pg_genaction[i] == 4) {
                        curSet.pg_genaction[i] = RandNum(1, 5);
                    }
                }
                for (i = 0; i < MAXIMAGES; i++) {
                    while (curSet.image[i].britemode == 2)
                        curSet.image[i].britemode = RandNum(0, 2);
                }
            }
            if (prefs.autoPilotOn[APB_ACTSOUND] && prefs.autoPilotOn[APB_ALWAYSSOUND]) {   // turn all react to sound options ON
                curSet.actReactSnd[ACTLINES] = true;
                curSet.actReactSnd[ACTBALLS] = true;
                curSet.actReactSnd[ACTDROPS] = true;
                curSet.actReactSnd[ACTSWARM] = true;
                curSet.actReactSnd[ACTDOODLE] = true;
                for (i = 0; i < MAXTEXTS; i++) {
                    if (!curSet.textGen[i].sizeReactSound && !curSet.textGen[i].brightReactSound) {
                        switch (RandNum(0, 2)) {
                            case 0: curSet.textGen[i].sizeReactSound = true; break;
                            case 1: curSet.textGen[i].brightReactSound = true; break;
                        }
                    }
                }
                for (i = 0; i < MAXGENERATORS; i++) {
                    curSet.pg_genaction[i] = 4;
                }
                for (i = 0; i < MAXIMAGES; i++) {
                    curSet.image[i].britemode = 2;
                }
            }
        }
        ShowSettings();
    }
}

void SetAutoPilotDefaults(void) {
    char i;

    prefs.autoPilotValue[APN_VERSION] = AUTOPILOTVERSION;
    prefs.autoPilotValue[APN_NUMSECONDS] = 5;
    prefs.autoPilotOn[APB_ACTIONS] = true;
    for (i = 0; i <= ACTIMAGES; i++)
        SetAPAction(i, true);
    SetAPAction(ACTDOODLE, false);
    prefs.autoPilotOn[APB_OPTIONS] = true;
    for (i = 0; i <= ACTIMAGES; i++)
        SetAPOption(i, true);
    SetAPOption(ACTDOODLE, false);
    prefs.autoPilotOn[APB_ACTSOUND] = false;
    prefs.autoPilotOn[APB_ALWAYSSOUND] = false;
    prefs.autoPilotOn[APB_FILTERS] = true;
    prefs.autoPilotOn[APB_MIRRORS] = true;
    prefs.autoPilotOn[APB_RANDCOLORS] = true;
    prefs.autoPilotOn[APB_SAVEDCOLORS] = true;
}

void SetAPAction(char action, Boolean value) {
    unsigned short mask;

    mask = 1 << action;
    if (value) {
        prefs.autoPilotValue[APN_ACTIONSCHANGE] |= mask;
    } else {
        prefs.autoPilotValue[APN_ACTIONSCHANGE] &= ~mask;
    }
}

Boolean GetAPAction(char action) {
    unsigned short mask;

    mask = 1 << action;
    return (prefs.autoPilotValue[APN_ACTIONSCHANGE] & mask) > 0;
}

void SetAPOption(char action, Boolean value) {
    unsigned short mask;

    mask = 1 << action;
    if (value) {
        prefs.autoPilotValue[APN_OPTIONSCHANGE] |= mask;
    } else {
        prefs.autoPilotValue[APN_OPTIONSCHANGE] &= ~mask;
    }
}

Boolean GetAPOption(char action) {
    unsigned short mask;

    mask = 1 << action;
    return (prefs.autoPilotValue[APN_OPTIONSCHANGE] & mask) > 0;
}

void StopAutoChanges(void) {
    storeAutopilot = prefs.autoPilotActive;
    prefs.autoPilotActive = false;

    storeAutosetchange = prefs.autoSetChange;
    prefs.autoSetChange = false;
}

void ResumeAutoChanges(void) {
    prefs.autoPilotActive = storeAutopilot;
    prefs.autoSetChange = storeAutosetchange;
}
