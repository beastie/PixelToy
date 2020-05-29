// PixelToy "About" window

#include "PixelToyAbout.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySound.h"
#include "CocoaBridge.h"

extern struct setinformation curSet;
extern struct preferences prefs;

extern WindowPtr gAboutWindow;
extern CGrafPtr aboutWorld;
CGrafPtr aboutStageWorld;
extern MenuHandle gOptionsMenu, gFiltersMenu, gColorsMenu, gSetsMenu;
int aboutByteCount;
extern Ptr aboutBasePtr;
extern CTabHandle regAboutColors, unregAboutColors;
extern Boolean gFirstLaunch;
extern short gSysVersionMajor, gSysVersionMinor;
EventLoopTimerRef aboutAnimationTimer;

unsigned char aboutRed[256], aboutGreen[256], aboutBlue[256];
Str255 gUserName;
long aboutTickStart;
Ptr aboutBasePtr;
PixMapHandle aboutPixMap;

const int aboutBitmapScale = 2;

pascal void AboutWindowTimerEventHandler(EventLoopTimerRef /*theTimer*/, void * /*userData*/);

void CreateAboutWindow(void) {
    SInt32 error;
    CTabHandle colorTable;
    Rect nullRect;
    Boolean storeSound;
    RGBColor color;
    PaletteHandle aboutPalette;

    SaveContext();

    // Set up colors
    colorTable = regAboutColors;

    aboutPalette = NewPalette(254, nil, pmTolerant, 0);
    CTab2Palette(colorTable, aboutPalette, pmTolerant, pmTolerant);

    // Create offscreen world
    SetRect(&nullRect, 0, 0, ABOUTWIDTH * aboutBitmapScale, ABOUTHEIGHT * aboutBitmapScale);
    error = NewGWorld(&aboutWorld, 8, &nullRect, colorTable, nil, 0);
    if (error != noErr)
        StopError(7, error);
    aboutPixMap = GetGWorldPixMap(aboutWorld);
    if (!PTLockPixels(aboutPixMap)) {
        error = UpdateGWorld(&aboutWorld, 8, &nullRect, colorTable, nil, 0);
        if (error == gwFlagErr)
            StopError(7, error);
        aboutPixMap = GetGWorldPixMap(aboutWorld);
        if (!PTLockPixels(aboutPixMap))
            StopError(7, error);
    }
    aboutBasePtr = GetPixBaseAddr(aboutPixMap);
    aboutByteCount = (0x7FFF & (**aboutPixMap).rowBytes);

    NewGWorld(&aboutStageWorld, 32, &nullRect, nil, nil, 0);

    gAboutWindow = GetNewWindow(BASERES, nil, (WindowPtr)-1);
    SetPalette(gAboutWindow, aboutPalette, false);
    ForceOnScreen(gAboutWindow);
    ShowWindow(gAboutWindow);
    SetGWorld(aboutWorld, nil);
    Index2Color(1, &color);
    RGBBackColor(&color);
    EraseRect(&nullRect);

    SelectWindow(gAboutWindow);
    RestoreContext();
    if (gOptionsMenu)
        MyDisableMenuItem(gOptionsMenu, 0);
    if (gFiltersMenu)
        MyDisableMenuItem(gFiltersMenu, 0);
    if (gColorsMenu)
        MyDisableMenuItem(gColorsMenu, 0);
    if (gSetsMenu)
        MyDisableMenuItem(gSetsMenu, 0);
    DrawMenuBar();
    aboutTickStart = TickCount();
    storeSound = curSet.sound;
    curSet.sound = true;
    PlaySound(BASERES, true);
    curSet.sound = storeSound;

    InstallEventLoopTimer(GetMainEventLoop(), 0, kEventDurationSecond / 30.0, NewEventLoopTimerUPP(AboutWindowTimerEventHandler),
                          nil, &aboutAnimationTimer);
}

void DestroyAboutWindow(void) {
    RemoveEventLoopTimer(aboutAnimationTimer);
    DisposeWindow(gAboutWindow);
    DisposeGWorld(aboutWorld);
    DisposeGWorld(aboutStageWorld);
    gAboutWindow = 0;
    if (gOptionsMenu)
        MyEnableMenuItem(gOptionsMenu, 0);
    if (gFiltersMenu)
        MyEnableMenuItem(gFiltersMenu, 0);
    if (gColorsMenu)
        MyEnableMenuItem(gColorsMenu, 0);
    if (gSetsMenu)
        MyEnableMenuItem(gSetsMenu, 0);
    DrawMenuBar();
    if (prefs.fullScreen)
        MyHideMenuBar();
    if (gFirstLaunch) {
        AnimateWindow();
        DoStandardAlert(kAlertNoteAlert, 19);
        gFirstLaunch = false;
    }
}

pascal void AboutWindowTimerEventHandler(EventLoopTimerRef theTimer, void *userData) {
    DrawAboutWindowContents();
}

void DrawAboutWindowContents(void) {
    static short titleFnum = -1, otherFnum = -1, genevaFnum = -1;
    long index;
    RGBColor color;
    Str63 str;
    Rect rect, rect2;

    if (genevaFnum < 1)
        GetFNum("\pGeneva", &genevaFnum);
    if (genevaFnum < 1)
        genevaFnum = 0;

    if (titleFnum < 1)
        GetFNum("\pTechno", &titleFnum);
    if (titleFnum < 1)
        GetFNum("\pGill Sans", &titleFnum);
    if (titleFnum < 1)
        GetFNum("\pGillSans", &titleFnum);
    if (titleFnum < 1)
        titleFnum = genevaFnum;

    if (otherFnum < 1) {
        GetFNum("\pGill Sans", &otherFnum);
        if (otherFnum < 1)
            GetFNum("\pGillSans", &otherFnum);
        if (otherFnum < 1)
            GetFNum("\pTechno", &otherFnum);
        if (otherFnum < 1)
            otherFnum = genevaFnum;
    }

    SaveContext();
    // do effect
    dispatch_apply(150 * aboutBitmapScale, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = (current + 34) * aboutByteCount;
        int endX = (ABOUTWIDTH - 8) * aboutBitmapScale;
        long yOffset1 = (aboutByteCount * 3);
        long yOffset2 = (aboutByteCount * 6);
        int x;
        for (x = 8 * aboutBitmapScale; x < endX; x++) {
            aboutBasePtr[x + offset1] =
                ((unsigned char)aboutBasePtr[x + offset1] * 128 + (unsigned char)aboutBasePtr[x + offset1 - 6] * 8 +
                 (unsigned char)aboutBasePtr[x + offset1 - 3] * 8 + (unsigned char)aboutBasePtr[x + offset1 - 1] * 16 +
                 (unsigned char)aboutBasePtr[x + offset1 + 6] * 8 + (unsigned char)aboutBasePtr[x + offset1 + 3] * 8 +
                 (unsigned char)aboutBasePtr[x + offset1 + 1] * 16 + (unsigned char)aboutBasePtr[x + offset1 - yOffset1] * 8 +
                 (unsigned char)aboutBasePtr[x + offset1 - yOffset2] * 8 +
                 (unsigned char)aboutBasePtr[x + offset1 - aboutByteCount] * 16 +
                 (unsigned char)aboutBasePtr[x + offset1 + yOffset1] * 8 + (unsigned char)aboutBasePtr[x + offset1 + yOffset2] * 8 +
                 (unsigned char)aboutBasePtr[x + offset1 + aboutByteCount] * 16) >>
                8;
        }
        offset1 += aboutByteCount;
    });

    SetGWorld(aboutWorld, nil);

    Index2Color(1, &color);
    RGBForeColor(&color);

    static StringPtr sVersPtr = NULL;
    if (sVersPtr == NULL) {
        CFStringRef versString = (CFStringRef)CopyAppVersionString();
        if (versString) {
            StringPtr tempStr = calloc(1, 256);
            if (CFStringGetPascalString(versString, tempStr, 255, kCFStringEncodingMacRoman))
                sVersPtr = tempStr;
            CFRelease(versString);
        }
        if (sVersPtr == NULL)
            sVersPtr = "\p";
    }
    if (sVersPtr != NULL) {
        int versionFontSize = 12;
        SetRect(&rect, 0, (ABOUTHEIGHT - (versionFontSize * 2)) * aboutBitmapScale, 90 * aboutBitmapScale,
                ABOUTHEIGHT * aboutBitmapScale);
        PaintRect(&rect);
        TextFont(titleFnum);
        TextSize(versionFontSize * aboutBitmapScale);
        MyDrawText(sVersPtr, 9 * aboutBitmapScale, (ABOUTHEIGHT - 9) * aboutBitmapScale, 255);
    }

    TextFont(titleFnum);
    Str32 title = "\ppixeltoy";
    memcpy(str, title, title[0] + 1);
    index = 130 * aboutBitmapScale;
    TextSize(index);
    while (StringWidth(str) > ((ABOUTWIDTH * aboutBitmapScale) - 20)) {
        index--;
        TextSize(index);
    }
    index = (TickCount() - aboutTickStart) % 512;
    if (index > 255)
        index = 512 - index;
    if (index > 250)
        index = 250;
    if (index < 1)
        index = 1;
    int frameIndex = index;
    MyDrawText(str, (ABOUTWIDTH * aboutBitmapScale) / 2 - (StringWidth(str) / 2), 120 * aboutBitmapScale, index);
    TextFont(otherFnum);
    index = (TickCount() - aboutTickStart);
    if (index > 255)
        index = 255;
    TextSize(18 * aboutBitmapScale);

    GetIndString(str, BASERES + 3, 3);    // written by leon mcneill
    MyDrawText(str, (ABOUTWIDTH * aboutBitmapScale) / 2 - (StringWidth(str) / 2), 210 * aboutBitmapScale, index);
    GetAppCopyright(&str);
    MyDrawText(str, (ABOUTWIDTH * aboutBitmapScale) / 2 - (StringWidth(str) / 2), 240 * aboutBitmapScale, index);
    Str32 webaddress = "\plairware.com";
    MyDrawText(webaddress, (ABOUTWIDTH * aboutBitmapScale) / 2 - (StringWidth(webaddress) / 2), 270 * aboutBitmapScale, index);

    GetPortBounds(aboutWorld, &rect);
    Index2Color(frameIndex, &color);
    RGBForeColor(&color);
    PenSize(2, 2);
    FrameRect(&rect);

    RestoreContext();

    GetWindowPortBounds(gAboutWindow, &rect2);

    SetGWorld(aboutStageWorld, nil);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(GetPortBitMapForCopyBits(aboutWorld), GetPortBitMapForCopyBits(aboutStageWorld), &rect, &rect, srcCopy, nil);
    SetPortWindowPort(gAboutWindow);
    ForeColor(blackColor);
    BackColor(whiteColor);
    CopyBits(GetPortBitMapForCopyBits(aboutStageWorld), GetPortBitMapForCopyBits(GetWindowPort(gAboutWindow)), &rect, &rect2,
             srcCopy, nil);

    RestoreContext();
}
