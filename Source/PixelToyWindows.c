// PixelToyWindows.c

#include "PixelToyWindows.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyBitmap.h"
#include "PixelToyFilters.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToyParticles.h"
#include "PixelToySets.h"
#include "PixelToyText.h"
#include "PixelToyWaves.h"

extern struct preferences prefs;
extern struct setinformation curSet;
extern MenuHandle gOptionsMenu, gColorsMenu, gSetsMenu, gWindowMenu;
extern WindowPtr gMainWindow, gAboutWindow;
extern short numPalettes, lastSetLoaded, gNumSets;
extern Boolean gCurFullScreenState, gCurSetsReadOnly;
extern FSSpec curSetsSpec;
extern Rect gDragRect;
extern struct PTPalette *ptpal;

WindowRef gWindow[MAXWINDOW];
CGrafPtr gWindowWorld[MAXWINDOW], buttonWorld, bwButtonWorld;
Boolean gWindowPush[MAXWINDOW];

// Private functions
void MoveToDefaultWindowPositions(void);

void PTInitialWindows(void) {
    OSErr error;
    Rect rect;
    PicHandle pic;
    short i;

    SaveContext();
    // get 32-bit buttons
    pic = GetPicture(BASERES + 2);
    if (pic) {
        rect.left = EndianS16_BtoN((*pic)->picFrame.left);
        rect.top = EndianS16_BtoN((*pic)->picFrame.top);
        rect.right = EndianS16_BtoN((*pic)->picFrame.right);
        rect.bottom = EndianS16_BtoN((*pic)->picFrame.bottom);
        error = NewGWorld(&buttonWorld, 32, &rect, nil, nil, pixelsLocked);
        if (error)
            StopError(66, error);
        SetGWorld(buttonWorld, nil);
        ForeColor(blackColor);
        BackColor(whiteColor);
        DrawPicture(pic, &rect);
        ReleaseResource((Handle)pic);
        for (i = 0; i < 9; i++) {
            pic = GetPicture(600 + i);
            if (pic) {
                SetRect(&rect, i * 24, 0, i * 24 + 24, 24);
                DrawPicture(pic, &rect);
                ReleaseResource((Handle)pic);
            }
            pic = GetPicture(500 + i);
            if (pic) {
                SetRect(&rect, i * 24, 24, i * 24 + 24, 48);
                DrawPicture(pic, &rect);
                ReleaseResource((Handle)pic);
            }
        }
        RestoreContext();
    } else {
        StopError(66, memFullErr);
    }
    // get 1-bit buttons
    pic = GetPicture(BASERES + 3);
    if (pic) {
        rect.left = EndianS16_BtoN((*pic)->picFrame.left);
        rect.top = EndianS16_BtoN((*pic)->picFrame.top);
        rect.right = EndianS16_BtoN((*pic)->picFrame.right);
        rect.bottom = EndianS16_BtoN((*pic)->picFrame.bottom);
        error = NewGWorld(&bwButtonWorld, 1, &rect, nil, nil, pixelsLocked);
        if (error)
            StopError(67, error);
        SetGWorld(bwButtonWorld, nil);
        //if (!PTLockPixels(GetGWorldPixMap(buttonWorld))) StopError(67, 0);
        ForeColor(blackColor);
        BackColor(whiteColor);
        DrawPicture(pic, &rect);
        RestoreContext();
        ReleaseResource((Handle)pic);
    } else {
        StopError(67, memFullErr);
    }

    // create initially open windows
    for (i = 0; i < MAXWINDOW; i++) {
        if (prefs.windowOpen[i]) {
            PTCreateWindow(i);
            PTDrawWindow(i);
            PTUpdateWindow(i);
            QDFlushPortBuffer(GetWindowPort(gWindow[i]), nil);
        } else {
            gWindow[i] = nil;
        }
    }
}

void PTTerminateWindows(void) {
    short i;

    for (i = 0; i < MAXWINDOW; i++) {
        if (prefs.windowOpen[i]) {
            PTDestroyWindow(i);
            prefs.windowOpen[i] = true;
        }
    }
}

void PushWindows(void) {
    short i;

    for (i = 0; i < MAXWINDOW; i++) {
        gWindowPush[i] = prefs.windowOpen[i];
        if (prefs.windowOpen[i])
            PTDestroyWindow(i);
    }
}

void PopWindows(void) {
    short i;

    for (i = 0; i < MAXWINDOW; i++) {
        if (gWindowPush[i] && !prefs.windowOpen[i])
            PTCreateWindow(i);
    }
}

void HandleWindowChoice(short theItem) {   // selected a window menu item
    Boolean somethingOpen = false;
    short i;

    switch (theItem) {
        case SHOWHIDEALLID:
            for (i = 0; i <= WINOPTIONS; i++) {
                if (prefs.windowOpen[i])
                    somethingOpen = true;
            }
            if (somethingOpen) {   // hide em
                for (i = 0; i <= WINOPTIONS; i++) {
                    if (prefs.windowOpen[i])
                        PTDestroyWindow(i);
                }
            } else {   // show everything
                for (i = 0; i <= WINOPTIONS; i++) {
                    if (!prefs.windowOpen[i])
                        PTCreateWindow(i);
                }
            }
            break;
        case MOVESIZEGROUP:
            prefs.dragSizeAllWindows = !prefs.dragSizeAllWindows;
            ShowSettings();
            break;
        case DEFAULTPOSNS: MoveToDefaultWindowPositions(); break;
        default:
            i = theItem - ACTIONWINID;
            if (prefs.windowOpen[i])
                PTDestroyWindow(i);
            else
                PTCreateWindow(i);
            break;
    }
}

void MoveToDefaultWindowPositions(void) {
    short i;
    Rect cRect, sRect;
    BitMap qdbm;

    GetQDGlobalsScreenBits(&qdbm);
    sRect = qdbm.bounds;
    PTTerminateWindows();
    PTResetWindows();
    GetWindowPortBounds(gMainWindow, &cRect);
    prefs.windowPosition[WINMAIN].h = ((sRect.right) / 2 - (cRect.right) / 2) + 20;     // was -1
    prefs.windowPosition[WINMAIN].v = ((sRect.bottom) / 2 - (cRect.bottom) / 2) - 8;    // was -1, then - 20
    if (prefs.windowPosition[WINMAIN].v < (cRect.top + 30))
        prefs.windowPosition[WINMAIN].v = cRect.top + 30;
    MoveWindow(gMainWindow, prefs.windowPosition[WINMAIN].h, prefs.windowPosition[WINMAIN].v, false);
    for (i = 0; i <= MAXWINDOW; i++) {
        if (prefs.windowOpen[i])
            PTCreateWindow(i);
    }
}

void HandleWindowMouseDown(short winNum) {
    SaveContext();
    SetPortWindowPort(gWindow[winNum]);
    switch (winNum) {
        case WINACTIONS: HandleActionMouseDown(); break;
        case WINFILTERS: HandleFilterMouseDown(); break;
        case WINCOLORS: HandleColorMouseDown(); break;
        case WINCOLORED: HandleEditMouseDown(); break;
        case WINSETS: HandleSetMouseDown(); break;
        case WINOPTIONS: HandleOptionMouseDown(); break;
    }
    RestoreContext();
}

void PTResetWindows(void) {
    short i;

    for (i = 0; i < WINMAINSTORED; i++) {
        prefs.windowPosition[i].h = -1;
        prefs.windowPosition[i].v = -1;
        prefs.windowOpen[i] = (i <= WINOPTIONS);
    }
}

void PTCreateWindow(short winNum) {
    OSErr error;
    Rect wRect, cRect;
    PicHandle pic;
    Str63 menuStr, hideStr;
    short i;

    if (winNum > WINOPTIONS)
        return;
    gWindow[winNum] = GetNewCWindow(BASERES + winNum + 1, nil, gMainWindow);
    if (gWindow[winNum] == nil) {
        error = ResError();
        StopError(65, BASERES + winNum + 1);    // should be error
    } else {
        GetWindowPortBounds(gWindow[winNum], &wRect);
        // This SizeWindow, along with the one after ShowWindow, force the titlebars to show.
        // I'm not sure why I need to do this.
        SizeWindow(gWindow[winNum], wRect.right, wRect.bottom - 1, true);
        gWindowWorld[winNum] = 0;
        OffsetRect(&wRect, -wRect.left, -wRect.top);

        error = NewGWorld(&gWindowWorld[winNum], 32, &wRect, nil, nil, pixelsLocked);
        if (error)
            gWindowWorld[winNum] = 0;

        prefs.windowOpen[winNum] = true;
        SaveContext();
        if (gWindowWorld[winNum])
            SetGWorld(gWindowWorld[winNum], nil);
        else
            SetPortWindowPort(gWindow[winNum]);
        BackColor(blackColor);
        EraseRect(&wRect);
        ForeColor(blackColor);
        BackColor(whiteColor);
        // Any necessary window type-specific setup
        switch (winNum) {
            case WINCOLORED:    // draw instructions
                pic = GetPicture(BASERES + 1);
                if (pic) {
                    wRect.left = EndianS16_BtoN((*pic)->picFrame.left);
                    wRect.top = EndianS16_BtoN((*pic)->picFrame.top);
                    wRect.right = EndianS16_BtoN((*pic)->picFrame.right);
                    wRect.bottom = EndianS16_BtoN((*pic)->picFrame.bottom);
                    OffsetRect(&wRect, 2, 72);
                    DrawPicture(pic, &wRect);
                    ReleaseResource((Handle)pic);
                }
                break;
        }
        RestoreContext();

        GetWindowPortBounds(gMainWindow, &cRect);
        if (prefs.windowPosition[winNum].v == -1 || prefs.windowPosition[winNum].v == 0) {   // default positions
            switch (winNum) {
                case WINACTIONS:    // on top of main, to the left
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h - 2;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINMAIN].v - 52;
                    break;
                case WINFILTERS:    // to left of main, aligned with actions
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h - 172;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINMAIN].v - 52;
                    break;
                case WINCOLORS:    // to right of main, below options
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h + cRect.right + 12;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINMAIN].v + 150;
                    break;
                case WINCOLORED:    // to bottom of main, to the left
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h - 2;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINMAIN].v + cRect.bottom + 24;
                    break;
                case WINSETS:    // to left of main, below filters
                    GetWindowPortBounds(gWindow[WINFILTERS], &cRect);
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h - 112;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINFILTERS].v + cRect.bottom + 22;
                    break;
                case WINOPTIONS:    // to right of main, to the top
                    prefs.windowPosition[winNum].h = prefs.windowPosition[WINMAIN].h + cRect.right + 12;
                    prefs.windowPosition[winNum].v = prefs.windowPosition[WINMAIN].v - 52;
                    break;
            }
        }
        MoveWindow(gWindow[winNum], prefs.windowPosition[winNum].h, prefs.windowPosition[winNum].v, false);
        ForceOnScreen(gWindow[winNum]);

        ShowWindow(gWindow[winNum]);
        SizeWindow(gWindow[winNum], wRect.right, wRect.bottom, true);
        if (gWindowMenu) {
            GetMenuItemText(gWindowMenu, winNum + ACTIONWINID, menuStr);
            i = 1;
            while (menuStr[i] != ' ' && i <= menuStr[0]) {
                i++;
            }
            GetIndString(hideStr, BASERES, 30);
            BlockMoveData(menuStr + i, hideStr + hideStr[0] + 1, menuStr[0] - (i - 1));
            hideStr[0] += menuStr[0] - (i - 1);
            SetMenuItemText(gWindowMenu, winNum + ACTIONWINID, hideStr);
        }
    }
}

void PTDestroyWindow(short winNum) {
    GrafPtr savePort;
    Str63 menuStr, showStr;
    short i;
    Rect cRect;

    GetPort(&savePort);
    SetPortWindowPort(gWindow[winNum]);
    GetWindowPortBounds(gWindow[winNum], &cRect);
    prefs.windowPosition[winNum].h = cRect.left;
    prefs.windowPosition[winNum].v = cRect.top;
    LocalToGlobal(&prefs.windowPosition[winNum]);
    if (savePort != GetWindowPort(gWindow[winNum])) {
        SetPort((GrafPtr)savePort);
    } else {
        SetPortWindowPort(gMainWindow);
    }
    DisposeWindow(gWindow[winNum]);
    if (gWindowWorld[winNum]) {
        DisposeGWorld(gWindowWorld[winNum]);
        gWindowWorld[winNum] = 0;
    }
    prefs.windowOpen[winNum] = false;
    gWindow[winNum] = nil;
    if (gWindowMenu) {
        GetMenuItemText(gWindowMenu, winNum + ACTIONWINID, menuStr);
        i = 1;
        while (menuStr[i] != ' ' && i <= menuStr[0]) {
            i++;
        }
        GetIndString(showStr, BASERES, 29);
        BlockMoveData(menuStr + i, showStr + showStr[0] + 1, menuStr[0] - (i - 1));
        showStr[0] += menuStr[0] - (i - 1);
        SetMenuItemText(gWindowMenu, winNum + ACTIONWINID, showStr);
    }
}

void PTInvalidateWindow(short winNum) {
    Rect winRect;
    if (prefs.windowOpen[winNum]) {
        GetWindowPortBounds(gWindow[winNum], &winRect);
        MyInvalWindowRect(gWindow[winNum], &winRect);
    }
}

void PTUpdateWindow(short winNum) {
    Rect fromRect, toRect;
    CGrafPtr toPort;

    if (gWindowWorld[winNum]) {
        SaveContext();
        GetPortBounds(gWindowWorld[winNum], &fromRect);
        GetWindowPortBounds(gWindow[winNum], &toRect);
        toPort = GetWindowPort(gWindow[winNum]);
        SetGWorld(toPort, nil);
        CopyBits(GetPortBitMapForCopyBits(gWindowWorld[winNum]), GetPortBitMapForCopyBits(toPort), &fromRect, &toRect, srcCopy,
                 nil);    // was ditherCopy
        MyValidWindowRect(gWindow[winNum], &toRect);
        RestoreContext();
    }
}

void PTDrawWindow(short winNum) {
    switch (winNum) {
        case WINACTIONS: UpdateActionButtons(); break;
        case WINFILTERS: UpdateFilterButtons(); break;
        case WINCOLORS: UpdateColorButtons(); break;
        case WINCOLORED: UpdateEditColors(-1, -1); break;
        case WINSETS: UpdateSetButtons(); break;
        case WINOPTIONS: UpdateOptionButtons(); break;
    }
}

void HandleWindowDrag(WindowPtr window, Point startPoint) {
    Point lastPoint, point;
    Rect rect;
    short i;

    SaveContext();
    Boolean special = prefs.dragSizeAllWindows;
    if (OptionIsPressed())
        special = !special;
    if (special && window != gAboutWindow) {
        // New Carbonated way, physically move everything
        for (i = 0; i < MAXWINDOW; i++) {
            if (prefs.windowOpen[i]) {
                SetPortWindowPort(gWindow[i]);
                GetWindowPortBounds(gWindow[i], &rect);
                prefs.windowPosition[i].h = rect.left;
                prefs.windowPosition[i].v = rect.top;
                LocalToGlobal(&prefs.windowPosition[i]);
            }
        }
        while (StillDown()) {
            LetTimersBreathe();
            GetMouse(&point);
            LocalToGlobal(&point);
            if (point.h != lastPoint.h || point.v != lastPoint.v) {
                lastPoint = point;
                if (!gCurFullScreenState)
                    MoveWindow(gMainWindow, prefs.windowPosition[WINMAIN].h + (point.h - startPoint.h),
                               prefs.windowPosition[WINMAIN].v + (point.v - startPoint.v), false);
                for (i = 0; i < MAXWINDOW; i++) {
                    if (prefs.windowOpen[i]) {
                        MoveWindow(gWindow[i], prefs.windowPosition[i].h + (point.h - startPoint.h),
                                   prefs.windowPosition[i].v + (point.v - startPoint.v), false);
                    }
                }
            }
        }
        SetPortWindowPort(gMainWindow);
        if (!gCurFullScreenState)
            ForceOnScreen(gMainWindow);
        GetWindowPortBounds(gMainWindow, &rect);
        prefs.windowPosition[WINMAIN].h = rect.left;
        prefs.windowPosition[WINMAIN].v = rect.top;
        LocalToGlobal(&prefs.windowPosition[WINMAIN]);
        for (i = 0; i < MAXWINDOW; i++) {
            if (prefs.windowOpen[i]) {
                SetPortWindowPort(gWindow[i]);
                ForceOnScreen(gWindow[i]);
                GetWindowPortBounds(gWindow[i], &rect);
                point.h = rect.left;
                point.v = rect.top;
                LocalToGlobal(&point);
                prefs.windowPosition[i] = point;
            }
        }
    } else {
        SetPortWindowPort(window);
        DragWindow(window, startPoint, &gDragRect);
        ForceOnScreen(window);
        if (window == gMainWindow) {
            GetWindowPortBounds(window, &rect);
            point.h = rect.left;
            point.v = rect.top;
            LocalToGlobal(&point);
            prefs.windowPosition[WINMAIN].h = point.h;
            prefs.windowPosition[WINMAIN].v = point.v;
        }
    }
    SetResizeBoxRect();
    RestoreContext();
}

void MagneticWindows(short oldx, short oldy) {
    short i;
    Point point;
    Rect cRect;

    SaveContext();
    for (i = 0; i < MAXWINDOW; i++) {
        if (prefs.windowOpen[i]) {
            SetPortWindowPort(gWindow[i]);
            GetWindowPortBounds(gWindow[i], &cRect);
            point.h = cRect.left;
            point.v = cRect.top;
            LocalToGlobal(&point);
            GetWindowPortBounds(gMainWindow, &cRect);
            if (point.h > prefs.windowPosition[WINMAIN].h + oldx)
                point.h += (cRect.right - oldx);
            if (point.v > prefs.windowPosition[WINMAIN].v + oldy)
                point.v += (cRect.bottom - oldy);
            MoveWindow(gWindow[i], point.h, point.v, false);
            ForceOnScreen(gWindow[i]);
        }
    }
    RestoreContext();
}

void HandleEditMouseDown(void) {
    short colorIndex, endColorIndex = -1, lastEnd;
    short rFrac, gFrac, bFrac, stretch, curPos, mark;
    RGBColor orgColor, newColor, hiColor, lowColor;
    Boolean firstTime = true;

    StopAutoChanges();
    colorIndex = ColorIndicated();
    if (colorIndex < 0)
        return;
    lastEnd = -1;
    while (StillDown() || firstTime) {
        firstTime = false;
        endColorIndex = ColorIndicated();
        if (endColorIndex != lastEnd) {
            UpdateEditColors(colorIndex, endColorIndex);
            PTUpdateWindow(WINCOLORED);
            lastEnd = endColorIndex;
        }
    }
    if (colorIndex == endColorIndex) {
        // change this color
        MyGetColor(&orgColor, colorIndex);
        if (ChooseRGBColor(&orgColor, &newColor)) {
            MySetColor(&newColor, colorIndex);
            CurSetTouched();
            EmployPalette(true);
            mark = CountMenuItems(gColorsMenu);
            for (curPos = FIRSTPALETTEID; curPos <= mark; curPos++) {
                if (gColorsMenu)
                    CheckMenuItem(gColorsMenu, curPos, false);
            }
        }
    } else {
        if (endColorIndex > -1 && endColorIndex < 256) {
            // blend colorIndex -> endColorIndex
            if (colorIndex > endColorIndex) {
                endColorIndex = colorIndex;
                colorIndex = lastEnd;
            }
            stretch = endColorIndex - colorIndex;
            MyGetColor(&lowColor, colorIndex);
            MyGetColor(&hiColor, endColorIndex);
            rFrac = (hiColor.red - lowColor.red) / stretch;
            gFrac = (hiColor.green - lowColor.green) / stretch;
            bFrac = (hiColor.blue - lowColor.blue) / stretch;
            for (curPos = colorIndex + 1; curPos <= endColorIndex; curPos++) {
                mark = curPos - colorIndex;
                newColor.red = lowColor.red + rFrac * mark;
                newColor.green = lowColor.green + gFrac * mark;
                newColor.blue = lowColor.blue + bFrac * mark;
                MySetColor(&newColor, curPos);
            }
            mark = CountMenuItems(gColorsMenu);
            for (curPos = FIRSTPALETTEID; curPos <= mark; curPos++) {
                if (gColorsMenu)
                    CheckMenuItem(gColorsMenu, curPos, false);
            }
            CurSetTouched();
            EmployPalette(true);
        }
    }
    ResumeAutoChanges();
    PTInvalidateWindow(WINCOLORED);
}

short ColorIndicated(void) {
    Rect rect;
    Point mouse;
    short colorIndex;

    GetMouse(&mouse);
    SetRect(&rect, 4, 4, 260, 69);
    colorIndex = -1;
    if (PtInRect(mouse, &rect))
        colorIndex = ((mouse.v - 4) / 8) * 32 + (mouse.h - 4) / 8;
    return colorIndex;
}

void MyGetColor(RGBColor *color, short index) {
    if (index < 0 || index > 255)
        return;
    color->red = curSet.palentry[index].red;
    color->green = curSet.palentry[index].green;
    color->blue = curSet.palentry[index].blue;
}

void MySetColor(RGBColor *color, short index) {
    if (index < 0 || index > 255)
        return;
    curSet.palentry[index].red = color->red;
    curSet.palentry[index].green = color->green;
    curSet.palentry[index].blue = color->blue;
}

Boolean ChooseRGBColor(RGBColor *orgColor, RGBColor *newColor) {
    Point where;
    Boolean gotColor;
    Str255 prompt;

    where.h = where.v = 0;
    EnsureSystemPalette();
    GetIndString(prompt, BASERES, 23);
    gotColor = GetColor(where, prompt, orgColor, newColor);
    PopPal();
    return gotColor;
}

void UpdateEditColors(short firstSel, short lastSel) {
    Boolean showSelection;
    RGBColor color;
    Rect rect, cRect;
    short c, x, y, size = 8;

    if (!gWindowWorld[WINCOLORED])
        return;
    showSelection = true;
    if (firstSel < 0 || lastSel < 0)
        showSelection = false;
    SaveContext();
    SetGWorld(gWindowWorld[WINCOLORED], nil);
    x = 4;
    y = 4;
    SetRect(&rect, 3, 3, 262, 70);
    BackColor(blackColor);
    EraseRect(&rect);
    color.red = color.green = color.blue = 24576;
    RGBForeColor(&color);
    FrameRect(&rect);
    GetWindowPortBounds(gWindow[WINCOLORED], &cRect);
    for (c = 0; c < 256; c++) {
        SetRect(&rect, x, y, x + size + 1, y + size + 1);
        MyGetColor(&color, c);
        RGBForeColor(&color);
        InsetRect(&rect, 1, 1);
        PaintRect(&rect);
        x += size;
        if (x >= (cRect.right - size)) {
            x = 4;
            y += size;
        }
    }
    if (showSelection) {
        x = 4;
        y = 4;
        ForeColor(whiteColor);
        if (firstSel > lastSel) {
            c = firstSel;
            firstSel = lastSel;
            lastSel = c;
        }
        for (c = 0; c < 256; c++) {
            if (c >= firstSel && c <= lastSel) {
                SetRect(&rect, x, y, x + size + 1, y + size + 1);
                FrameRect(&rect);
            }
            x += size;
            if (x >= (cRect.right - size)) {
                x = 4;
                y += size;
            }
        }
    }
    RestoreContext();
}

void UpdateActionButtons(void) {
    Boolean fail = false;
    short depth, i;
    Rect to, from;
    CGrafPtr sourceWorld, destWorld;

    // Determine source
    depth = GetWindowDevice(gWindow[WINACTIONS]);
    if (depth < 16) {
        sourceWorld = bwButtonWorld;
    } else {
        sourceWorld = buttonWorld;
    }
    if (!PTLockPixels(GetGWorldPixMap(sourceWorld)))
        fail = true;
    // Determine destination
    if (gWindowWorld[WINACTIONS]) {
        destWorld = gWindowWorld[WINACTIONS];
        if (!PTLockPixels(GetGWorldPixMap(destWorld)))
            fail = true;
    } else {
        destWorld = GetWindowPort(gWindow[WINACTIONS]);
    }
    if (!fail) {
        SaveContext();
        SetGWorld(destWorld, nil);
        ForeColor(blackColor);
        BackColor(whiteColor);
        for (i = 0; i < 9; i++) {
            from.left = i * 24;
            from.top = (!curSet.action[i]) * 24;
            from.right = from.left + 24;
            from.bottom = from.top + 24;
            to.left = i * 24;
            to.top = 0;
            to.right = to.left + 24;
            to.bottom = to.top + 24;
            CopyBits(GetPortBitMapForCopyBits(sourceWorld), GetPortBitMapForCopyBits(destWorld), &from, &to, srcCopy, nil);
        }
        RestoreContext();
    }
    UnlockPixels(GetGWorldPixMap(sourceWorld));
}

void HandleActionMouseDown(void) {
    short butNum;
    Point mouse;
    Rect butRect;
    Boolean clicked;

    GetMouse(&mouse);
    butNum = mouse.h / 24;
    butRect.left = butNum * 24;
    butRect.right = butRect.left + 24;
    butRect.top = 0;
    butRect.bottom = 24;
    clicked = ButtonPressed(&butRect);
    PTUpdateWindow(WINACTIONS);
    if (clicked) {
        if (OptionIsPressed()) {
            if (butNum == 4)
                HandleOptionsChoice(WAVEOPTSID);
            if (butNum < 6 && butNum != 4)
                HandleOptionsChoice(VARID);
            if (butNum == 6)
                HandleOptionsChoice(TEXTOPTSID);
            if (butNum == 7)
                HandleOptionsChoice(PARTICOPTID);
            if (butNum == 8)
                HandleOptionsChoice(BITMAPOPTID);
        } else {
            HandleActionsChoice(butNum + 1);
        }
    }
}

void UpdateFilterButtons(void) {
    short depth, i;
    Rect to, from;
    CGrafPtr sourceWorld;

    if (!gWindowWorld[WINFILTERS])
        return;
    depth = GetWindowDevice(gWindow[WINFILTERS]);
    SaveContext();
    if (depth < 16) {
        sourceWorld = bwButtonWorld;
    } else {
        sourceWorld = buttonWorld;
    }
    SetGWorld(gWindowWorld[WINFILTERS], nil);
    for (i = 0; i < 25; i++) {
        to.left = (i % 5) * 32;
        to.right = to.left + 32;
        to.top = (i / 5) * 32;
        to.bottom = to.top + 32;
        from = to;
        OffsetRect(&from, 0, 48);
        if (FilterActive(i))
            OffsetRect(&from, 160, 0);
        CopyBits(GetPortBitMapForCopyBits(sourceWorld), GetPortBitMapForCopyBits(gWindowWorld[WINFILTERS]), &from, &to, srcCopy,
                 nil);
    }
    // horizontal mirror
    SetRect(&to, 0, 160, 54, 176);
    from = to;
    OffsetRect(&from, 0, 48);
    if (curSet.horizontalMirror)
        OffsetRect(&from, 160, 0);
    CopyBits(GetPortBitMapForCopyBits(sourceWorld), GetPortBitMapForCopyBits(gWindowWorld[WINFILTERS]), &from, &to, srcCopy, nil);
    // vertical mirror
    SetRect(&to, 54, 160, 108, 176);
    from = to;
    OffsetRect(&from, 0, 48);
    if (curSet.verticalMirror)
        OffsetRect(&from, 160, 0);
    CopyBits(GetPortBitMapForCopyBits(sourceWorld), GetPortBitMapForCopyBits(gWindowWorld[WINFILTERS]), &from, &to, srcCopy, nil);
    // constrain mirror
    SetRect(&to, 108, 160, 160, 176);
    from = to;
    OffsetRect(&from, 0, 48);
    if (curSet.constrainMirror)
        OffsetRect(&from, 160, 0);
    CopyBits(GetPortBitMapForCopyBits(sourceWorld), GetPortBitMapForCopyBits(gWindowWorld[WINFILTERS]), &from, &to, srcCopy, nil);
    RestoreContext();
}

void HandleFilterMouseDown(void) {
    short butNum;
    Point mouse;
    Rect butRect;
    Boolean clicked;

    GetMouse(&mouse);
    butNum = (mouse.h / 32) + ((mouse.v / 32) * 5);
    butRect.left = (butNum % 5) * 32;
    butRect.right = butRect.left + 32;
    butRect.top = (butNum / 5) * 32;
    butRect.bottom = butRect.top + 32;
    if (butNum < 25) {   // a standard filter
        clicked = ButtonPressed(&butRect);
        PTUpdateWindow(WINFILTERS);
        if (clicked) {
            if (butNum == 24 && OptionIsPressed()) {   // edit custom filter
                EditCustomFilter();
            } else {
                CurSetTouched();
                if (!ShiftIsPressed())
                    ClearAllFilters();
                ToggleFilter(butNum);
                ShowSettings();
            }
        }
    } else {
        // horizontal mirror
        SetRect(&butRect, 0, 160, 54, 176);
        if (PtInRect(mouse, &butRect)) {
            if (ButtonPressed(&butRect))
                ToggleMirror(0);
        }
        // vertical mirror
        OffsetRect(&butRect, 54, 0);
        if (PtInRect(mouse, &butRect)) {
            if (ButtonPressed(&butRect))
                ToggleMirror(1);
        }
        // constrain mirror
        OffsetRect(&butRect, 54, 0);
        if (PtInRect(mouse, &butRect)) {
            if (ButtonPressed(&butRect))
                ToggleMirror(2);
        }
    }
}

void UpdateColorButtons(void) {
    OSErr error;
    short depth, x, y, fontID, i, curMark;
    Rect winRect, to, from;
    Str255 str;
    RGBColor color;
    Boolean usingThis = false;

    if (!gWindowWorld[WINCOLORS])
        return;
    depth = GetWindowDevice(gWindow[WINCOLORS]);
    GetIndString(str, BASERES, 28);
    winRect.left = winRect.top = 0;
    winRect.right = 100;
    winRect.bottom = (numPalettes + 2) * 12;
    GetWindowPortBounds(gWindow[WINCOLORS], &to);
    if (!EqualRect(&winRect, &to)) {
        SizeWindow(gWindow[WINCOLORS], winRect.right, winRect.bottom, true);
        error = UpdateGWorld(&gWindowWorld[WINCOLORS], 32, &winRect, nil, nil, 0);
        if (error)
            StopError(65, error);
        PTLockPixels(GetGWorldPixMap(gWindowWorld[WINCOLORS]));
        ForceOnScreen(gWindow[WINCOLORS]);
    }
    SaveContext();
    SetGWorld(gWindowWorld[WINCOLORS], nil);
    GetFNum(str, &fontID);
    TextFont(fontID);
    TextMode(srcOr);
    TextSize(9);
    for (i = 0; i < (numPalettes + 2); i++) {
        SetRect(&from, 216, 0, 316, 12);
        SetRect(&to, 0, i * 12, 100, i * 12 + 12);
        if (depth < 16) {
            ForeColor(blackColor);
            PaintRect(&to);
        }
        if (i == 0) {   // Sound Color modes
            SetRect(&from, 216, 24, 316, 36);
            CopyBits(GetPortBitMapForCopyBits(buttonWorld), GetPortBitMapForCopyBits(gWindowWorld[WINCOLORS]), &from, &to, srcCopy,
                     nil);
            from.top = 0;
            from.bottom = 12;
            if (curSet.paletteSoundMode == 0) {
                from.left = 0;
                from.right = 24;
            } else {
                from.left = curSet.paletteSoundMode * 19 + 6;
                from.right = from.left + 18;
            }
            PenSize(1, 1);
            color.red = 49152;
            color.green = 49152;
            color.blue = 0;
            RGBForeColor(&color);
            FrameRect(&from);
            InsetRect(&from, 1, 1);
            color.green = 0;
            RGBForeColor(&color);
            FrameRect(&from);
            ForeColor(blackColor);
        } else if (i == 1) {   // Random: Dimmed blue background
            if (gColorsMenu)
                GetMenuItemText(gColorsMenu, RANDPALETTEID, str);
            if (depth > 8) {
                OffsetRect(&from, 0, 12);
                CopyBits(GetPortBitMapForCopyBits(buttonWorld), GetPortBitMapForCopyBits(gWindowWorld[WINCOLORS]), &from, &to,
                         srcCopy, nil);
            }
        } else {   // Actual palette color background
            if (gColorsMenu)
                GetMenuItemText(gColorsMenu, i + FIRSTPALETTEID - 2, str);
            if (gColorsMenu)
                GetItemMark(gColorsMenu, i + FIRSTPALETTEID - 2, &curMark);
            usingThis = (curMark > 0);
            if (depth > 8) {
                for (x = 0; x < 100; x++) {
                    curMark = (x * 2.55);
                    color.red = ptpal[i - 2].palentry[curMark].red;
                    color.green = ptpal[i - 2].palentry[curMark].green;
                    color.blue = ptpal[i - 2].palentry[curMark].blue;
                    RGBForeColor(&color);
                    if (usingThis) {
                        MoveTo(x, to.top);
                        LineTo(x, to.bottom);
                    } else {
                        MoveTo(x, to.top);
                        LineTo(x, to.top + 1);
                        color.red /= 2;
                        color.green /= 2;
                        color.blue /= 2;
                        RGBForeColor(&color);
                        MoveTo(x, to.top + 1);
                        LineTo(x, to.bottom - 1);
                        color.red /= 2;
                        color.green /= 2;
                        color.blue /= 2;
                        RGBForeColor(&color);
                        MoveTo(x, to.bottom - 1);
                        LineTo(x, to.bottom);
                    }
                }
            } else {
                EraseRect(&to);
            }
        }
        if (i > 0) {
            while (StringWidth(str) > 96) {
                str[--str[0]] = 0xC9; // É
            }
            x = 50 - (StringWidth(str) / 2);
            y = i * 12 + 9;
            MoveTo(x + 1, y + 1);
            ForeColor(blackColor);
            DrawString(str);
            MoveTo(x, y);
            if (depth < 16) {
                ForeColor(whiteColor);
            } else {
                if (usingThis) {
                    ForeColor(whiteColor);
                } else {
                    color.red = color.green = color.blue = 49152;
                    RGBForeColor(&color);
                }
            }
            DrawString(str);
            ForeColor(blackColor);
            if (depth < 16 && usingThis)
                InvertRect(&to);
        }
    }
    RestoreContext();
}

void HandleColorMouseDown(void) {
    short i, butNum;
    Point mouse;
    Rect butRect;
    Boolean clicked = false;

    GetMouse(&mouse);
    if (mouse.v < 12) {   // Sound Color buttons
        butNum = -1;
        butRect.top = 0;
        butRect.bottom = 12;
        i = 0;
        clicked = false;
        while (i < 6 && !clicked) {
            if (i == 0) {
                butRect.left = 0;
                butRect.right = 24;
            } else {
                butRect.left = i * 19 + 6;
                butRect.right = butRect.left + 18;
            }
            if (PtInRect(mouse, &butRect)) {
                butNum = i;
                clicked = true;
            }
            i++;
        }
        if (butNum > -1)
            clicked = ButtonPressed(&butRect);
        if (clicked) {
            curSet.paletteSoundMode = butNum;
            CurSetTouched();
            ShowSettings();
        }
    } else {   // Random & saved color buttons
        butNum = (mouse.v - 12) / 12;
        butRect.left = 0;
        butRect.right = 100;
        butRect.top = butNum * 12 + 12;
        butRect.bottom = butRect.top + 12;
        clicked = ButtonPressed(&butRect);
        PTUpdateWindow(WINCOLORS);
        if (clicked) {
            if (butNum == 0) {                                         // Generate random colors
                RandomPalette(curSet.emboss || ControlIsPressed());    // ctrl for smooth palette
                EmployPalette(ControlIsPressed());                     // ctrl = do it instantly
                CurSetTouched();
                PTInvalidateWindow(WINCOLORS);
            } else {   // or just load one of the saved color sets
                if (OptionIsPressed()) {
                    DoRenamePaletteDialog(butNum - 1);
                }    // opt = rename
                else {
                    UseColorPreset(butNum + FIRSTPALETTEID - 1, ControlIsPressed());
                    //EmployPalette(ControlIsPressed()); // ctrl = do it instantly
                    CurSetTouched();
                }
            }
        }
    }
}

void UpdateSetButtons(void) {
    static Boolean titleHasBeenChanged = false;
    OSErr error;
    short depth, x, y, fontID, i, curMark, hilited;
    Rect winRect, to;
    Str255 str;
    Boolean usingThis;
    RGBColor color;
    struct setinformation si;

    if (!gWindowWorld[WINSETS])
        return;
    // If user opens non-PixelToy Sets, change window title to filename.
    // If user goes back to PixelToy Sets, go ahead and change then too.
    GetIndString(str, BASERES, 58);
    if (curSetsSpec.name[0] > 0 && (titleHasBeenChanged || !EqualString(curSetsSpec.name, str, false, false))) {
        Str255 theName;
        BlockMove(curSetsSpec.name, theName, curSetsSpec.name[0] + 1);
        if (theName[0] > 5 && theName[theName[0] - 4] == '.' && theName[theName[0] - 3] == 's' && theName[theName[0] - 2] == 't' &&
            theName[theName[0] - 1] == 'n' && theName[theName[0]] == 'g')
            theName[0] -= 5;
        SetWTitle(gWindow[WINSETS], theName);
        titleHasBeenChanged = true;
    }

    depth = GetWindowDevice(gWindow[WINSETS]);
    GetIndString(str, BASERES, 28);
    winRect.left = winRect.top = 0;
    winRect.right = 100;
    winRect.bottom = gNumSets * 12 + 4;
    if (gNumSets < 1)
        winRect.bottom = 16;
    GetWindowPortBounds(gWindow[WINSETS], &to);
    SaveContext();
    if (!EqualRect(&winRect, &to)) {
        SizeWindow(gWindow[WINSETS], winRect.right, winRect.bottom, true);
        error = UpdateGWorld(&gWindowWorld[WINSETS], 32, &winRect, nil, nil, 0);
        if (error)
            StopError(65, error);
        PTLockPixels(GetGWorldPixMap(gWindowWorld[WINSETS]));
        ForceOnScreen(gWindow[WINSETS]);
    }
    SetGWorld(gWindowWorld[WINSETS], nil);
    GetFNum(str, &fontID);
    TextFont(fontID);
    TextMode(srcOr);
    TextSize(9);
    if (gNumSets < 1) {
        GetWindowPortBounds(gWindow[WINSETS], &to);
        ForeColor(blackColor);
        BackColor(whiteColor);
        EraseRect(&to);
        GetIndString(str, BASERES, 3);
        x = 50 - (StringWidth(str) / 2);
        MoveTo(x, 11);
        TextMode(srcOr);
        TextFace(italic);
        DrawString(str);
        TextFace(0);
        InvertRect(&to);
    } else {
        hilited = -1;
        for (i = 0; i < gNumSets; i++) {
            SetRect(&to, 0, i * 12 + 2, 100, i * 12 + 14);
            if (depth < 16) {
                ForeColor(blackColor);
                PaintRect(&to);
            }
            LoadPixelToySet(&si, i);
            curMark = (lastSetLoaded == i && curSet.windowTitle[curSet.windowTitle[0]] != 0xC9); // 'É'
            usingThis = (curMark > 0);
            if (usingThis)
                hilited = i;
            if (depth > 8) {
                for (x = 0; x < 100; x++) {
                    curMark = x * 2.55;
                    color = si.palentry[curMark];
                    RGBForeColor(&color);
                    if (usingThis) {
                        MoveTo(x, to.top);
                        LineTo(x, to.bottom);
                    } else {
                        MoveTo(x, to.top);
                        LineTo(x, to.top + 1);
                        color.red /= 2;
                        color.green /= 2;
                        color.blue /= 2;
                        RGBForeColor(&color);
                        MoveTo(x, to.top + 1);
                        LineTo(x, to.bottom - 1);
                        color.red /= 2;
                        color.green /= 2;
                        color.blue /= 2;
                        RGBForeColor(&color);
                        MoveTo(x, to.bottom - 1);
                        LineTo(x, to.bottom);
                    }
                }
            }
            while (StringWidth(si.windowTitle) > 96) {
                si.windowTitle[--si.windowTitle[0]] = 0xC9;
            }    // "..."
            x = 50 - (StringWidth(si.windowTitle) / 2);
            y = i * 12 + 11;
            MoveTo(x + 1, y + 1);
            ForeColor(blackColor);
            DrawString(si.windowTitle);
            MoveTo(x, y);
            if (usingThis || depth < 16) {
                ForeColor(whiteColor);
            } else {
                color.red = color.green = color.blue = 49152;
                RGBForeColor(&color);
            }
            DrawString(si.windowTitle);
            ForeColor(blackColor);
            if (usingThis && depth < 16)
                InvertRect(&to);
        }
        color.red = 49152;
        color.green = 49152;
        color.blue = 0;
        RGBForeColor(&color);
        PenSize(2, 2);
        GetWindowPortBounds(gWindow[WINSETS], &to);
        FrameRect(&to);
        PenSize(1, 1);
        if (hilited > -1) {
            SetRect(&to, 2, hilited * 12 + 2, 98, hilited * 12 + 14);
            FrameRect(&to);
        }
    }
    RestoreContext();
}

void HandleSetMouseDown(void) {
    short butNum;
    Point mouse;
    Rect butRect;
    Boolean clicked;

    GetMouse(&mouse);
    butNum = (mouse.v - 2) / 12;
    butRect.left = 2;
    butRect.right = 98;
    butRect.top = butNum * 12 + 2;
    butRect.bottom = butRect.top + 12;
    clicked = ButtonPressed(&butRect);
    PTInvalidateWindow(WINSETS);
    if (clicked) {
        if (OptionIsPressed()) {
            if (!gCurSetsReadOnly)
                RenameOldSet(butNum);
            else
                SysBeep(1);
        } else {
            UsePixelToySet(butNum);
        }
    }
}

void UpdateOptionButtons(void) {
    OSErr error;
    short depth, x, y, fontID, i, numItems, curMark;
    Rect winRect, to, from;
    Str255 str;
    RGBColor grey;

    if (!gWindowWorld[WINOPTIONS])
        return;
    depth = GetWindowDevice(gWindow[WINSETS]);
    GetIndString(str, BASERES, 28);
    numItems = CountMenuItems(gOptionsMenu);
    if (numItems > 0) {
        winRect.left = winRect.top = 0;
        winRect.right = 100;
        winRect.bottom = numItems * 12;
        GetWindowPortBounds(gWindow[WINOPTIONS], &to);
        if (!EqualRect(&winRect, &to)) {
            SizeWindow(gWindow[WINOPTIONS], winRect.right, winRect.bottom, true);
            error = UpdateGWorld(&gWindowWorld[WINOPTIONS], 32, &winRect, nil, nil, 0);
            if (error)
                StopError(65, error);
            PTLockPixels(GetGWorldPixMap(gWindowWorld[WINOPTIONS]));
            ForceOnScreen(gWindow[WINOPTIONS]);
        }
        SaveContext();
        SetGWorld(gWindowWorld[WINOPTIONS], nil);
        GetFNum(str, &fontID);
        TextFont(fontID);
        TextMode(srcOr);
        TextSize(9);
        for (i = 0; i < numItems; i++) {
            SetRect(&from, 216, 0, 316, 12);
            if (gOptionsMenu)
                GetItemMark(gOptionsMenu, i + 1, &curMark);
            if (!curMark)
                OffsetRect(&from, 0, 12);
            SetRect(&to, 0, i * 12, 100, i * 12 + 12);
            if (depth < 16) {
                ForeColor(blackColor);
                PaintRect(&to);
            } else {
                CopyBits(GetPortBitMapForCopyBits(buttonWorld), GetPortBitMapForCopyBits(gWindowWorld[WINOPTIONS]), &from, &to,
                         srcCopy, nil);
            }
            if (gOptionsMenu)
                GetMenuItemText(gOptionsMenu, i + 1, str);
            y = i * 12 + 9;
            if (str[1] == '-') {   // divider
                ForeColor(blackColor);
                MoveTo(0, y - 2);
                LineTo(100, y - 2);
                ForeColor(whiteColor);
                MoveTo(0, y - 3);
                LineTo(100, y - 3);
            } else {
                while (StringWidth(str) > 96) {
                    str[--str[0]] = 0xC9; // É
                }
                x = 50 - (StringWidth(str) / 2);
                MoveTo(x + 1, y + 1);
                ForeColor(blackColor);
                DrawString(str);
                MoveTo(x, y);
                if (MyIsMenuItemEnabled(gOptionsMenu, i + 1)) {
                    if (curMark || depth < 16) {
                        ForeColor(whiteColor);
                    } else {
                        grey.red = grey.green = grey.blue = 49152;
                        RGBForeColor(&grey);
                    }
                } else {   // totally disabled
                    grey.red = grey.green = grey.blue = 32768;
                    RGBForeColor(&grey);
                }
                DrawString(str);
                if (depth < 16 && curMark)
                    InvertRect(&to);
            }
            ForeColor(blackColor);
        }
        RestoreContext();
    }
}

void HandleOptionMouseDown(void) {
    short butNum;
    Point mouse;
    Rect butRect;
    Boolean clicked;

    SaveContext();
    SetPortWindowPort(gWindow[WINOPTIONS]);
    GetMouse(&mouse);
    butNum = mouse.v / 12;
    if (MyIsMenuItemEnabled(gOptionsMenu, butNum + 1)) {
        butRect.left = 0;
        butRect.right = 100;
        butRect.top = butNum * 12;
        butRect.bottom = butRect.top + 12;
        clicked = ButtonPressed(&butRect);
        PTInvalidateWindow(WINOPTIONS);
        if (clicked)
            HandleOptionsChoice(butNum + 1);
    }
    RestoreContext();
}

Boolean ButtonPressed(Rect *rect) {
    CGrafPtr butWorld;
    GDHandle curDevice;
    GrafPtr curWorld;
    Point mouse;
    OSErr error;
    Rect upRect, downRect, offRect;
    Boolean last = false, clicked, firstTime = true;
    RGBColor opColor;
    RgnHandle rgn;

    GetGWorld(&curWorld, &curDevice);
    // prepare to create offscreen for up & down buttons
    offRect = *rect;
    OffsetRect(&offRect, -offRect.left, -offRect.top);
    upRect = offRect;
    downRect = offRect;
    OffsetRect(&downRect, 0, offRect.bottom);
    offRect.bottom *= 2;
    error = NewGWorld(&butWorld, 0, &offRect, nil, nil, 0);
    if (!error) {
        if (!PTLockPixels(GetGWorldPixMap(butWorld))) {
            DisposeGWorld(butWorld);
            error = 1;
        }
    }
    if (!error) {   // create up & down state images
        SetGWorld(butWorld, nil);
        CopyBits(GetPortBitMapForCopyBits(curWorld), GetPortBitMapForCopyBits(butWorld), rect, &upRect, srcCopy, nil);
        opColor.red = opColor.blue = opColor.green = 32768;
        OpColor(&opColor);
        EraseRect(&downRect);
        CopyBits(GetPortBitMapForCopyBits(butWorld), GetPortBitMapForCopyBits(butWorld), &upRect, &downRect, blend, nil);
        rgn = NewRgn();
        ScrollRect(&downRect, 1, 1, rgn);
        ForeColor(blackColor);
        PaintRgn(rgn);
        DisposeRgn(rgn);
    }
    while (StillDown() || firstTime) {
        firstTime = false;
        LetTimersBreathe();
        SetGWorld(curWorld, curDevice);
        GetMouse(&mouse);
        clicked = PtInRect(mouse, rect);
        if (last != clicked) {
            last = clicked;
            if (error) {   // outline
                if (clicked)
                    ForeColor(whiteColor);
                else
                    ForeColor(blackColor);
                FrameRect(rect);
            } else {
                if (clicked) {
                    CopyBits(GetPortBitMapForCopyBits(butWorld), GetPortBitMapForCopyBits(curWorld), &downRect, rect, srcCopy, nil);
                } else {
                    CopyBits(GetPortBitMapForCopyBits(butWorld), GetPortBitMapForCopyBits(curWorld), &upRect, rect, srcCopy, nil);
                }
            }
        }
    }
    ForeColor(blackColor);
    if (!error)
        DisposeGWorld(butWorld);
    return clicked;
}
