/* PixelToyMain.c */

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyParticles.h"
#include "PixelToyFilters.h"
#include "PixelToyAE.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToySound.h"
#include "PixelToyText.h"
#include "PixelToyWaves.h"
#include "PixelToyAbout.h"
#include "PixelToyWindows.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyBitmap.h"
#include "CocoaBridge.h"

extern PaletteHandle winPalette;
extern WindowPtr gWindow[MAXWINDOW];
extern RGBColor currentColor[256];

struct preferences prefs;
struct setinformation curSet;

struct PTLine *ptline;
struct PTWalk *ptwalk;
struct PTBee *ptbee;
struct PTParticle *ptparticle;

ModalFilterUPP DialogFilterProc;
ControlActionUPP SliderFeedbackProc;
WindowPtr gMainWindow = nil, gAboutWindow, gEditWindow;
CGrafPtr offWorld = 0, highQualityWorld = 0, blurWorld = 0;
CGrafPtr doubleWorld = 0, aboutWorld = 0, stage32 = 0;
CTabHandle mainColorTable, systemColorTable;
MenuHandle gAppleMenu, gFileMenu, gOptionsMenu, gActionsMenu, gFiltersMenu;
MenuHandle gColorsMenu, gSetsMenu, gWindowMenu;
MenuRef gHelpMenu;
MenuItemIndex helpMenuIndex;
PixMapHandle fromPixMap, blurPixMap, doublePixMap, highQualityPixMap, stage32PixMap;
Ptr fromBasePtr, toBasePtr, blurBasePtr, doubleBasePtr;
Ptr ptSets;
EventRecord gTheEvent;
short fromByteCount, toByteCount, blurByteCount, doubleByteCount, highQualityByteCount;
short gCopyMode, warpType, oldWidth, oldHeight, movieVRefNum;
short gBarHeight, targetX, targetY;
float curTextXLoc[MAXTEXTS], curTextYLoc[MAXTEXTS];
short gPICTSaveNum;
short gNumEffects, gSysVersionMajor, gSysVersionMinor;
short gNumSets, lastCursor;
long gTicks, realTimeLastTick, expiryTick, gSoundInRefNum, gExportBeginTicks;
long gExportFramesExported;
long maxlines, maxballs, maxbees, maxdrops, maxparticles;
Rect gDragRect, gImageRect, gSizeRect, gCopyFromRect, gCopyToRect, gResizeBoxRect;
Boolean gCapableSoundOut, gDone, gNoPalTrans, gMouseHideOverride = false;
Boolean gCapableSoundIn, gPause, gFirstDoodle, gDoodleInProgress = false;
Boolean gFirstLaunch, gFirstWorld, gBufExists, gCurFullScreenState;
Boolean gQTExporting, gFullBackgrounded, gWasDoodling, gShowNeedsSoundInput;
Boolean gAboutWasPaused, gEditWindowActive, gDrawSprocket, gCurSetsReadOnly;
Movie theMovie = nil;
Track theVideoTrack, theSoundTrack;
Media theVideoMedia, theSoundMedia;
Handle compressedMovieData;
short movieResRefNum, movieResID, gExportFPSSpecified;
ComponentInstance ci;
ImageDescriptionHandle imageDesc;
SoundDescriptionHandle soundDesc;
TimeValue durationPerSample;
FSSpec currentPictureFSS, curSetsSpec;
RgnHandle gGrayRgn = nil;
long aboutWindowTime;
EventLoopTimerRef animationTimer = 0;

#define CONTEXTSTACKSIZE 16
CGrafPtr contextWorld[CONTEXTSTACKSIZE];
GDHandle contextDevice[CONTEXTSTACKSIZE];
int contextStackPointer = 0;

long betaNoticeTick;

extern GDHandle windowDevice;
static EventHandlerRef gAppEventHandlerRef = 0;

OSStatus InstallApplicationEventHandlers(void);
static pascal OSStatus HandleApplicationEvent(EventHandlerCallRef inCallRef, EventRef theEvent, void *userData);
void PixelToyInit(void);
void PixelToyWillTerminate(void);

// Local prototypes
long DesiredSleepTime(void);
void UpdateExportTimeWindowTitle(void);

int PixelToy_RunLoopMain(void) {
    PixelToyInit();
    InstallApplicationEventHandlers();
    RunApplicationEventLoop();
    PixelToyWillTerminate();
    return 0;
}

void PixelToyInit(void) {
    Boolean startFullscreen;
    short y;

    SetUpMacStuff();
    OpenResourceFile();
    curSetsSpec.name[0] = 0;
    if (!CheckSystemRequirements())
        ExitToShell();
    SliderFeedbackProc = NewControlActionUPP((ControlActionProcPtr)MySliderProc);
    CreateDialogFilter();
    ConfigureFilter(1, 1);
    if (!InitializeAEHandlers())
        ExitToShell();
    SetRect(&gDragRect, -32767, -32767, 32767, 32767);
    gPICTSaveNum = 1;
    gMainWindow = 0;
    gAboutWindow = 0;
    gPause = false;
    gFullBackgrounded = false;
    gQTExporting = false;
    gFirstWorld = true;
    gBufExists = false;
    // Determine maximum values and initialize datasets
    maxlines = 2000;
    maxballs = 2000;
    maxbees = 30000;
    maxdrops = 400;
    maxparticles = 30000;
    ptparticle = (struct PTParticle *)NewPtrClear(maxparticles * sizeof(struct PTParticle));
    ptline = (struct PTLine *)NewPtrClear(maxlines * sizeof(struct PTLine));
    ptwalk = (struct PTWalk *)NewPtrClear(maxballs * sizeof(struct PTWalk));
    ptbee = (struct PTBee *)NewPtrClear(maxbees * sizeof(struct PTBee));
    if (!ptparticle || !ptline || !ptwalk || !ptbee)
        StopError(1, -108);
    for (y = 0; y < maxballs; y++) {
        ptwalk[y].x = RandNum(0, 16384);
        ptwalk[y].y = RandNum(0, 16384);
        ptwalk[y].size = RandVariance(50);
    }

    oldWidth = 0;
    GetPrefs();
    startFullscreen = prefs.fullScreen;
    prefs.fullScreen = false;
    GetDefaultPalettes();
    CocoaAppLaunchInits();
    RegisterAppearanceClient();
    InitializePalette();
    CreateWorlds();
    MakeWindow(true);

    long memNeeded = MemoryRequiredCurrent();
    if (!CanAdjustWorlds(memNeeded)) {
        prefs.fullScreen = false;
        prefs.windowPosition[WINMAIN].h = prefs.windowPosition[WINMAIN].v = -1;
        prefs.winxsize = 320;
        prefs.winysize = 240;
        prefs.lowQualityMode = prefs.highQualityMode = false;
        MakeWindow(true);
    }
    if (!SetUpWorlds())
        ExitToShell();

    EmployPalette(false);    // true
    OpenChannel();
    OpenSoundIn(prefs.soundInputDevice);
    MaintainSoundInput();
    if (!gCapableSoundIn)
        prefs.soundInputDevice = 0;
    UpdateCurrentSetNeedsSoundInputMessage();
    PTInitialWindows();
    ShowSettings();
    BuildFontList();
    if (startFullscreen && NotifyReturningFullScreen()) {
        prefs.fullScreen = true;
        DoFullScreenTransition();
    }
    aboutWindowTime = TickCount() + 60;
    expiryTick = aboutWindowTime + 53904;    // 15 mins = 54,000 ticks
    if (gFirstLaunch)
        prefs.autoSetChange = true;
    NotifyVirtualMemorySucks();
    UpdateAnimationTimer();
    betaNoticeTick = TickCount() + 90;
}

void PixelToyWillTerminate(void) {
    StopAnimationTimer();
    if (gQTExporting)
        StopQTExport();
    if (gAboutWindow)
        DestroyAboutWindow();
    DisposeModalFilterUPP(DialogFilterProc);
    if (prefs.fullScreen) {   //MyShowMenuBar();
        prefs.fullScreen = !prefs.fullScreen;
        DoFullScreenTransition();
        prefs.fullScreen = true;
    }
    PTTerminateWindows();
    CloseSoundIn();
    CloseChannel();
    PutPrefs();
    PutPalettes();
    PutPixelToySets();
}

int PixelToy_Main(void) {
    Boolean startFullscreen;
    short y, winNum;

    SetUpMacStuff();
    OpenResourceFile();
    curSetsSpec.name[0] = 0;
    if (!CheckSystemRequirements()) {
        ExitToShell();
    }
    SliderFeedbackProc = NewControlActionUPP((ControlActionProcPtr)MySliderProc);
    CreateDialogFilter();
    ConfigureFilter(1, 1);
    if (!InitializeAEHandlers()) {
        ExitToShell();
    }
    SetRect(&gDragRect, -32767, -32767, 32767, 32767);
    gPICTSaveNum = 1;
    gMainWindow = 0;
    gAboutWindow = 0;
    gPause = false;
    gFullBackgrounded = false;
    gQTExporting = false;
    gFirstWorld = true;
    gBufExists = false;
    // Determine maximum values and initialize datasets
    maxlines = 2000;
    maxballs = 2000;
    maxbees = 30000;
    maxdrops = 400;
    maxparticles = 30000;
    ptparticle = (struct PTParticle *)NewPtrClear(maxparticles * sizeof(struct PTParticle));
    ptline = (struct PTLine *)NewPtrClear(maxlines * sizeof(struct PTLine));
    ptwalk = (struct PTWalk *)NewPtrClear(maxballs * sizeof(struct PTWalk));
    ptbee = (struct PTBee *)NewPtrClear(maxbees * sizeof(struct PTBee));
    if (!ptparticle || !ptline || !ptwalk || !ptbee)
        StopError(1, -108);
    for (y = 0; y < maxballs; y++) {
        ptwalk[y].x = RandNum(0, 16384);
        ptwalk[y].y = RandNum(0, 16384);
        ptwalk[y].size = RandVariance(50);
    }

    oldWidth = 0;
    GetPrefs();
    startFullscreen = prefs.fullScreen;
    prefs.fullScreen = false;
    GetDefaultPalettes();
    MenuBarInit();
    CocoaAppLaunchInits();
    RegisterAppearanceClient();
    DrawMenuBar();    // again for appearance
    InitializePalette();
    CreateWorlds();
    MakeWindow(true);
    long memNeeded = MemoryRequiredCurrent();
    if (!CanAdjustWorlds(memNeeded)) {
        prefs.fullScreen = false;
        prefs.windowPosition[WINMAIN].h = prefs.windowPosition[WINMAIN].v = -1;
        prefs.winxsize = 320;
        prefs.winysize = 240;
        prefs.lowQualityMode = prefs.highQualityMode = false;
        MakeWindow(true);
    }
    if (!SetUpWorlds()) {
        ExitToShell();
    }

    EmployPalette(false);    // true
    OpenChannel();
    OpenSoundIn(prefs.soundInputDevice);
    MaintainSoundInput();
    if (!gCapableSoundIn)
        prefs.soundInputDevice = 0;
    UpdateCurrentSetNeedsSoundInputMessage();
    PTInitialWindows();
    ShowSettings();
    BuildFontList();
    if (startFullscreen && NotifyReturningFullScreen()) {
        prefs.fullScreen = true;
        DoFullScreenTransition();
    }
    aboutWindowTime = TickCount() + 60;
    expiryTick = aboutWindowTime + 53904;    // 15 mins = 54,000 ticks
    if (gFirstLaunch)
        prefs.autoSetChange = true;
    NotifyVirtualMemorySucks();
    UpdateAnimationTimer();
    betaNoticeTick = TickCount() + 90;
    gDone = false;
    while (!gDone) {
        WaitNextEvent(everyEvent, &gTheEvent, 0, nil);
        switch (gTheEvent.what) {
            case mouseDown: HandleMouseDown(); break;
            case mouseUp: gDoodleInProgress = false; break;
            case autoKey:
            case keyDown: HandleKeyDown(LoWord(gTheEvent.message), gTheEvent.modifiers); break;
            case updateEvt:
                BeginUpdate((WindowPtr)gTheEvent.message);
                if ((WindowPtr)gTheEvent.message == gMainWindow) {
                    if (gPause)
                        UpdateToMain(false);
                    EndUpdate((WindowPtr)gTheEvent.message);
                } else if ((WindowPtr)gTheEvent.message == gAboutWindow) {
                    DrawAboutWindowContents();
                    EndUpdate((WindowPtr)gTheEvent.message);
                } else {
                    winNum = -1;
                    y = 0;
                    while (y < MAXWINDOW && winNum == -1) {
                        if (prefs.windowOpen[y]) {
                            if ((WindowPtr)gTheEvent.message == gWindow[y])
                                winNum = y;
                        }
                        y++;
                    }
                    if (winNum != -1) {
                        EndUpdate(gWindow[winNum]);
                        PTDrawWindow(winNum);
                        PTUpdateWindow(winNum);
                    }
                }
                break;
            case kHighLevelEvent: AEProcessAppleEvent(&gTheEvent); break;
            default: break;
        }
    }
    StopAnimationTimer();
    if (gQTExporting)
        StopQTExport();
    if (gAboutWindow)
        DestroyAboutWindow();
    DisposeModalFilterUPP(DialogFilterProc);
    if (prefs.fullScreen) {
        prefs.fullScreen = !prefs.fullScreen;
        DoFullScreenTransition();
        prefs.fullScreen = true;
    }
    PTTerminateWindows();
    CloseSoundIn();
    CloseChannel();
quit:
    PutPrefs();
    PutPalettes();
    PutPixelToySets();
    return 0;
}

OSStatus InstallApplicationEventHandlers(void) {
    const EventTypeSpec lEvents[] = {{kEventClassMouse, kEventMouseWheelMoved},    {kEventClassMouse, kEventMouseMoved},
                                     {kEventClassKeyboard, kEventRawKeyDown},      {kEventClassMenu, kEventMenuEnableItems},
                                     {kEventClassCommand, kEventCommandProcess},   {kEventClassAppleEvent, kEventAppleEvent},
                                     {kEventClassApplication, kEventAppActivated}, {kEventClassApplication, kEventAppTerminated},
                                     {kEventClassVolume, kEventVolumeMounted},     {kEventClassVolume, kEventVolumeUnmounted}};

    EventHandlerUPP appEventHandlerUPP = NewEventHandlerUPP(HandleApplicationEvent);

    return InstallApplicationEventHandler(appEventHandlerUPP, GetEventTypeCount(lEvents), lEvents, NULL, &gAppEventHandlerRef);
}

static OSStatus DoMenu_CommandID(unsigned long commandID) {
    OSStatus myErr = unimpErr;
    return myErr;
}

static OSStatus DoMenu_ItemID(UInt32 itemID) {
    OSStatus myErr = unimpErr;
    return myErr;
}

static OSStatus DoMenu(EventRef theEvent) {
    if (NULL == theEvent)
        return eventNotHandledErr;

    HICommand menuCommand = {0};
    if (noErr != GetEventParameter(theEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof(HICommand), NULL, &menuCommand))
        return eventNotHandledErr;

    // for now we only handle menu commands
    if (menuCommand.menu.menuRef == 0)
        return eventNotHandledErr;

    const short menuID = GetMenuID(menuCommand.menu.menuRef);

    const UInt32 itemID = (UInt32)(menuID << 16) + menuCommand.menu.menuItemIndex;

    if (DoMenu_CommandID(menuCommand.commandID) == noErr)
        return noErr;

    return (itemID) ? DoMenu_ItemID(itemID) : eventNotHandledErr;
}

void HandleQuit(void) {
    PixelToyWillTerminate();
    ExitToShell();
}

static pascal OSStatus HandleApplicationEvent(EventHandlerCallRef inCallRef, EventRef theEvent, void *userData) {
    OSStatus resultErr = eventNotHandledErr;

    const UInt32 eventClass = GetEventClass(theEvent);
    const UInt32 eventKind = GetEventKind(theEvent);

//    fprintf(stderr, "Event class:%c%c%c%c (%d) kind:%c%c%c%c (%d)\n", (char)((eventClass & 0xFF000000) >> 24),
//            (char)((eventClass & 0x00FF0000) >> 16), (char)((eventClass & 0x0000FF00) >> 8), (char)(eventClass & 0x000000FF),
//            (unsigned int)eventClass, (char)((eventKind & 0xFF000000) >> 24), (char)((eventKind & 0x00FF0000) >> 16),
//            (char)((eventKind & 0x0000FF00) >> 8), (char)(eventKind & 0x000000FF), (unsigned int)eventKind);

    switch (eventClass) {
        case kEventClassMouse: {
            switch (eventKind) {
                case kEventMouseDown:
                    HandleMouseDown();
                    resultErr = noErr;
                    break;
                case kEventMouseUp: gDoodleInProgress = false; break;
            }
        }

        case kEventClassKeyboard: {
            switch (eventKind) {
                case kEventRawKeyDown: {
                    UInt32 keyCode = 0;
                    if (noErr ==
                        GetEventParameter(theEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode)) {
                        HandleKeyDown(keyCode, GetCurrentKeyModifiers());
                        resultErr = noErr;
                    }
                } break;
            }
        }

        case kEventClassApplication: {
            switch (eventKind) {
                case kEventAppTerminated:
                    HandleQuit();
                    resultErr = noErr;
                    break;
            }
        }
        case kEventClassCommand: {
            switch (eventKind) {
                case kEventAppTerminated:
                    HandleQuit();
                    resultErr = noErr;
                    break;
            }
        }

        case kEventClassAppleEvent: {
        }
    }

    return resultErr;
}

int DesiredFramesPerSecond(void) {
    return (prefs.speedLimited) ? prefs.FPSlimit : 60;
}

const int kMultiFrameThreshold = 20;
const int kMultiFrameAmount = 3;    // best FPS we can get is about 30 * this number.

void UpdateAnimationTimer(void) {
    static EventLoopTimerUPP sEventHandlerUPP = NULL;
    if (!sEventHandlerUPP)
        sEventHandlerUPP = NewEventLoopTimerUPP(TimerEventHandler);

    OSStatus status = StopAnimationTimer();
    int desiredFPS = DesiredFramesPerSecond();
    EventTimerInterval interval = kEventDurationSecond / (float)desiredFPS;
    if (desiredFPS > kMultiFrameThreshold)
        interval *= kMultiFrameAmount;
    if (status == noErr && !gPause) {
        InstallEventLoopTimer(GetMainEventLoop(), 0, interval, sEventHandlerUPP, nil, &animationTimer);
    }
}

OSStatus StopAnimationTimer(void) {
    OSStatus result = noErr;
    if (animationTimer) {
        result = RemoveEventLoopTimer(animationTimer);
        animationTimer = 0;
    }
    return result;
}

void LetTimersBreathe() {
    EventRecord event;
    EventAvail(nullEvent, &event);
}

pascal void TimerEventHandler(EventLoopTimerRef theTimer, void *userData) {
    long now = TickCount();
    MaintainCursor();
    if (aboutWindowTime != -1 && now > aboutWindowTime) {
        aboutWindowTime = -1;
        if (!prefs.noSplashScreen)
            CreateAboutWindow();
    }
    if (!gAboutWindow) {
        int desiredFPS = DesiredFramesPerSecond();
        if (desiredFPS > kMultiFrameThreshold) {
            float interFrameDelay = (1.0 / (float)desiredFPS);
            int f;
            for (f = 0; f < kMultiFrameAmount; f++) {
                Boolean lastFrame = (f == (kMultiFrameAmount - 1));
                CFTimeInterval endTime = (lastFrame) ? 0.0 : CFAbsoluteTimeGetCurrent() + interFrameDelay;
                AnimateWindow();
                if (!lastFrame) {
                    QDFlushPortBuffer(GetWindowPort(gMainWindow), nil);
                    float remainingTime = (endTime - CFAbsoluteTimeGetCurrent()) - 0.05;
                    if (remainingTime > 0.01)
                        SleepForSeconds(remainingTime);
                }
            }
        } else {   // below threshold, just animate once.
            AnimateWindow();
        }
    }
}

char *byte_to_binary(int x) {
    static char b[9];
    b[0] = '\0';

    int z;
    for (z = 128; z > 0; z >>= 1) {
        strcat(b, ((x & z) == z) ? "1" : "0");
    }

    return b;
}

void AnimateWindow(void) {
    short c;
    if (!gPause) {
        if (!curSet.postFilter) {
            for (c = 0; c < gNumEffects; c++) {
                if (FilterActive(c))
                    DoFilter(c);
            }
        }
        PaletteTransitionUpdate();
        if (curSet.action[ACTIMAGES])
            DoImages(false);
        if (curSet.action[ACTLINES])
            DoLines();
        if (curSet.action[ACTBALLS])
            DoBalls();
        if (curSet.action[ACTSWARM])
            DoSwarm();
        if (curSet.action[ACTDROPS])
            DoDrops();
        if (curSet.action[ACTSWAVE])
            DoSoundVisuals();
        if (curSet.action[ACTDOODLE])
            DoDoodle();
        if (curSet.action[ACTTEXT])
            DoText();
        if (curSet.action[ACTPARTICLES])
            DoParticles();
        if (curSet.action[ACTIMAGES])
            DoImages(true);
        if (curSet.postFilter) {
            for (c = 0; c < gNumEffects; c++) {
                if (FilterActive(c))
                    DoFilter(c);
            }
        }
        FadeEdges();
        DoMirroring();
#if 0    //DEBUG
    // Overlay key map
    if (LockAllWorlds()) {
        SaveContext();
        
        UInt32 keyMods = GetCurrentKeyModifiers();
        char *modifiersAsBytes = &keyMods;
        Str255 modStr = "\p";
        int c; for (c=0; c<4; c++)
            { strcat(modStr+1, byte_to_binary(modifiersAsBytes[c])); }
        modStr[0]=32;
        
        UInt8 keymap[16];
        GetKeys(keymap);
        Str255 binString = "\p";
        for (c=0; c<16; c++)
            { strcat(binString+1, byte_to_binary(keymap[c])); }
        binString[0] = 128;
        
        static short keyMapFont = -1;
        if (keyMapFont==-1)
            GetFNum("\pMonaco", &keyMapFont);
            
        SetGWorld(offWorld, nil);
        ForeColor(blackColor);
        Rect clearRect;
        SetRect(&clearRect, 0, 0, 128*6+2, 34);
        PaintRect(&clearRect);
        ForeColor(whiteColor);
        TextSize(9);
        TextFont(keyMapFont);
        MoveTo(2, 9);
        DrawString(modStr);
        Str32 decStr; NumToString(keyMods, decStr);
        MoveTo(200, 9);
        DrawString(decStr);
        MoveTo(2, 17);
        DrawString(binString);
        for (c=0; c<128; c++) { binString[c+1]='0'+(c/10); }
        MoveTo(2, 25);
        DrawString(binString);
        for (c=0; c<128; c++) { binString[c+1]='0'+(c%10); }
        MoveTo(2, 33);
        DrawString(binString);
        ForeColor(blackColor);
        UnlockAllWorlds();
        RestoreContext();
    }
#endif
        UpdateToMain(false);
        if (gQTExporting)
            FrameQTExport();
        TimedSetChange();
        DoAutoPilot(false);
        // comment out "gCapableSoundIn &&" here to set fake sound data
        if (gCapableSoundIn && SetUsesSoundInput(&curSet))
            MaintainSoundInput();
    }
}

void MyOpenFile(FSSpec fss) {
    if (FileHasSuffixOrType(&fss, 'CPAL')) {
        PutPalettes();
        GetPalettes(&fss);
        return;
    }

    if (FileHasSuffixOrType(&fss, 'STNG')) {
        GetPixelToySets(fss);
        if (prefs.useFirstSet)
            UsePixelToySet(0);
        if (gCurSetsReadOnly)
            DoStandardAlert(kAlertNoteAlert, 8);
        return;
    }

    if (FileHasSuffixOrType(&fss, 'CFLT')) {
        LoadFilter(fss);
        return;
    }

    // otherwise, assume it's an image
    currentPictureFSS = fss;
    if (DrawCurrentImage(true) && gFileMenu)
        MyEnableMenuItem(gFileMenu, RELOADID);
}

void GetAndDrawImage(void) {
    OSErr err;
    FSSpec fss;

    InitCursor();
    err = CocoaChooseFile(CFSTR("ChooseOpenImageFile"), NULL, &fss);
    if (noErr == err) {
        if (gFileMenu)
            MyEnableMenuItem(gFileMenu, RELOADID);
        BlockMoveData(&fss, &currentPictureFSS, sizeof(struct FSSpec));
        DrawCurrentImage(true);
    }
}

Boolean DrawCurrentImage(Boolean resize) {
    OSErr err;
    short c, newSize, sWidth, sHeight;
    Rect pictRect;
    float mult, vmult;
    ComponentInstance gi;
    Rect mainRect;
    BitMap qdbm;
    RGBColor color;

    GetQDGlobalsScreenBits(&qdbm);
    mainRect = qdbm.bounds;

    err = GetGraphicsImporterForFile(&currentPictureFSS, &gi);
    if (err) {
        SysBeep(1);
        CloseComponent(gi);
        ResetCursor();
        return false;
    }
    err = GraphicsImportGetBoundsRect(gi, &pictRect);
    if (err) {
        SysBeep(1);
        CloseComponent(gi);
        ResetCursor();
        return false;
    }

    PushPal();
    for (c = 0; c <= 255; c++) {    // solid greyscale palette
        curSet.palentry[c].red = curSet.palentry[c].green = curSet.palentry[c].blue = c * 256;
    }
    EmployPalette(true);
    if (prefs.noImageResize || (!resize) || gQTExporting) {
        pictRect = gImageRect;    // override image's rect with current window shape
    } else {
        if (prefs.fullScreen) {    // resize PICT proportionally to fill screen
            ClearFromAndBase();
            mult = (float)prefs.winxsize / (float)(pictRect.right - pictRect.left);
            vmult = (float)prefs.winysize / (float)(pictRect.bottom - pictRect.top);
            if (vmult < mult)
                mult = vmult;    // use lesser of two resize values
            newSize = (pictRect.right - pictRect.left) * mult;
            pictRect.left = (prefs.winxsize / 2) - (newSize / 2);
            pictRect.right = pictRect.left + newSize;
            newSize = (pictRect.bottom - pictRect.top) * mult;
            pictRect.top = (prefs.winysize / 2) - (newSize / 2);
            pictRect.bottom = pictRect.top + newSize;
        } else {   // resize window to PICT's size
            prefs.storedxsize = prefs.winxsize;
            prefs.storedysize = prefs.winysize;
            prefs.windowPosition[WINMAINSTORED].h = prefs.windowPosition[WINMAIN].h;
            prefs.windowPosition[WINMAINSTORED].v = prefs.windowPosition[WINMAIN].v;
            sWidth = (mainRect.right - mainRect.left) - 8;
            sHeight = (mainRect.bottom - mainRect.top) - 50;
            if (prefs.lowQualityMode) {
                sWidth /= 2;
                sHeight /= 2;
            }
            mult = (float)sWidth / (float)(pictRect.right - pictRect.left);
            vmult = (float)sHeight / (float)(pictRect.bottom - pictRect.top);
            if (vmult < mult)
                mult = vmult;
            prefs.winxsize = pictRect.right - pictRect.left;
            prefs.winysize = pictRect.bottom - pictRect.top;
            if (mult < 1) {   // image must be shrunk down
                prefs.winxsize *= mult;
                prefs.winysize *= mult;
                pictRect.right = prefs.winxsize;
                pictRect.bottom = prefs.winysize;
            }
            if (!EqualRect(&pictRect, &gImageRect)) {
                StopAnimationTimer();
                MakeWindow(true);
                if (!SetUpWorlds()) {
                    prefs.winxsize = prefs.storedxsize;
                    prefs.winysize = prefs.storedysize;
                    prefs.windowPosition[WINMAIN].h = prefs.windowPosition[WINMAINSTORED].h;
                    prefs.windowPosition[WINMAIN].v = prefs.windowPosition[WINMAINSTORED].v;
                    MakeWindow(true);
                    if (!SetUpWorlds())
                        StopError(13, -108);
                } else if (prefs.dragSizeAllWindows)
                    MagneticWindows(prefs.storedxsize, prefs.storedysize);
                UpdateAnimationTimer();
            }
        }
    }
    SaveContext();
    if (LockAllWorlds()) {
        SetGWorld(offWorld, 0);
        ForeColor(blackColor);
        BackColor(whiteColor);
        err = GraphicsImportSetBoundsRect(gi, &pictRect);
        if (err) {
            SysBeep(1);
            goto dcierror;
        }
        color.red = color.green = color.blue = 0;
        err = GraphicsImportSetGraphicsMode(gi, srcCopy, &color);
        err = GraphicsImportSetGWorld(gi, nil, nil);
        if (err) {
            SysBeep(1);
            goto dcierror;
        }
        err = GraphicsImportDraw(gi);
        if (err) {
            SysBeep(1);
            goto dcierror;
        }
    }
dcierror:
    err = CloseComponent(gi);
    RestoreContext();
    UnlockAllWorlds();
    PopPal();
    ResetCursor();
    if (gPause)
        UpdateToMain(false);
    return true;
}

void HandleMouseDown(void) {
    WindowPtr whichWindow;
    short i, thePart, num, oldx, oldy;
    long menuChoice, sizeGot;
    Rect minMaxRect;
    Boolean isAboutWindow, isMainWindow, isEditWindow, specialResize;
    Point mouse;
    Rect mainRect;
    BitMap qdbm;
    long memCurrent, memNeeded;

    GetQDGlobalsScreenBits(&qdbm);
    mainRect = qdbm.bounds;
    mouse = gTheEvent.where;
    if (prefs.fullScreen && mouse.v < gBarHeight)
        MyShowMenuBar();
    thePart = FindWindow(gTheEvent.where, &whichWindow);
    num = -1;
    for (i = 0; i < MAXWINDOW; i++) {
        if (prefs.windowOpen[i]) {
            if (whichWindow == gWindow[i])
                num = i;
        }
    }
    isMainWindow = (whichWindow == gMainWindow);
    isAboutWindow = (whichWindow == gAboutWindow);
    isEditWindow = (whichWindow == gEditWindow);
    // if the About window is open and the user tries to work with something else,
    // kill the About window before doing anything else.
    if (gAboutWindow && !isAboutWindow)
        DestroyAboutWindow();
    if (lastCursor == 2 && !gQTExporting)
        thePart = inGrow;
    switch (thePart) {
        case inGrow:
            if (isMainWindow && !prefs.fullScreen) {
                short newWidth, newHeight;
                memCurrent = MemoryRequiredCurrent();
                SetRect(&minMaxRect, 64, 64, 32767, 32767);
                sizeGot = GrowWindow(whichWindow, gTheEvent.where, &minMaxRect);
                newWidth = LoWord(sizeGot);
                newHeight = HiWord(sizeGot);
                memNeeded = MemoryRequiredWithValues(newWidth, newHeight, prefs.lowQualityMode, prefs.highQualityMode);
                if (CanAdjustWorlds(memNeeded - memCurrent)) {
                    Rect rect;
                    StopAnimationTimer();
                    GetPortBounds(GetWindowPort(gMainWindow), &rect);
                    oldx = rect.right;
                    oldy = rect.bottom;
                    if (prefs.lowQualityMode) {
                        newHeight /= 2;
                        newWidth /= 2;
                    }
                    if (prefs.highQualityMode) {
                        newHeight *= 2;
                        newWidth *= 2;
                    }
                    oldWidth = prefs.winxsize;
                    oldHeight = prefs.winysize;
                    prefs.winxsize = newWidth;
                    prefs.winysize = newHeight;
                    MakeWindow(true);
                    specialResize = prefs.dragSizeAllWindows;
                    if (OptionIsPressed())
                        specialResize = !specialResize;
                    if (!SetUpWorlds()) {   // Failed, shouldn't happen!
                        prefs.winxsize = oldWidth;
                        prefs.winysize = oldHeight;
                        MakeWindow(true);
                        if (!SetUpWorlds())
                            StopError(13, -108);
                    } else /* it's all good */ if (specialResize) {
                        MagneticWindows(oldx, oldy);
                    }
                    prefs.mainWindowZoomed = false;
                    UpdateAnimationTimer();
                }
            }
            break;
        case inGoAway:
            if (TrackGoAway(whichWindow, gTheEvent.where)) {
                if (isMainWindow) {
                    gDone = true;
                }
                if (isAboutWindow)
                    DestroyAboutWindow();
                if (num != -1)
                    PTDestroyWindow(num);
            }
            break;
        case inMenuBar:
            gMouseHideOverride = true;
            if (!IsMacOSX())
                StopAnimationTimer();
            menuChoice = MenuSelect(gTheEvent.where);
            if (!IsMacOSX())
                UpdateAnimationTimer();
            HandleMenuChoice(menuChoice);
            gMouseHideOverride = false;
            if (prefs.fullScreen) { /*MyShowMenuBar();*/
                MyHideMenuBar();
            }
            break;
        case inSysWindow: break;
        case inDrag:    // La cage aux folles
            HandleWindowDrag(whichWindow, gTheEvent.where);
            break;
        case inZoomIn:
        case inZoomOut:    // no ZoomWindow, do it myself!
            if (gQTExporting)
                break;
            if (isMainWindow && !prefs.fullScreen) {
                if (TrackBox(whichWindow, gTheEvent.where, thePart)) {
                    Boolean error;
                    memCurrent = MemoryRequiredCurrent();
                    StopAnimationTimer();
                    if (prefs.mainWindowZoomed) {   // unzoom
                        oldWidth = prefs.winxsize;
                        oldHeight = prefs.winysize;
                        prefs.winxsize = prefs.storedxsize;
                        prefs.winysize = prefs.storedysize;
                        prefs.windowPosition[WINMAIN] = prefs.windowPosition[WINMAINSTORED];
                    } else {   // zoom out
                        oldWidth = prefs.winxsize;
                        oldHeight = prefs.winysize;
                        prefs.storedxsize = prefs.winxsize;
                        prefs.storedysize = prefs.winysize;
                        prefs.windowPosition[WINMAINSTORED] = prefs.windowPosition[WINMAIN];
                        GetWindowDeviceRect(whichWindow, &minMaxRect);
                        num = minMaxRect.right - minMaxRect.left - 17;
                        prefs.windowPosition[WINMAIN].h = minMaxRect.left + 8;
                        if (prefs.lowQualityMode)
                            num /= 2;
                        if (prefs.highQualityMode)
                            num *= 2;
                        prefs.winxsize = num;

                        if (EqualRect(&minMaxRect, &mainRect)) {   // main screen, avoid menubar
                            num = minMaxRect.bottom - minMaxRect.top - 53;
                            prefs.windowPosition[WINMAIN].v = minMaxRect.top + 44;
                        } else {
                            num = minMaxRect.bottom - minMaxRect.top - 33;
                            prefs.windowPosition[WINMAIN].v = minMaxRect.top + 24;
                        }
                        if (prefs.lowQualityMode)
                            num /= 2;
                        if (prefs.highQualityMode)
                            num *= 2;
                        prefs.winysize = num;
                    }
                    prefs.mainWindowZoomed = !prefs.mainWindowZoomed;
                    MakeWindow(true);
                    memNeeded = MemoryRequiredCurrent();
                    error = (!CanAdjustWorlds(memNeeded - memCurrent));
                    if (!error)
                        error = !SetUpWorlds();

                    if (error) {
                        prefs.mainWindowZoomed = !prefs.mainWindowZoomed;
                        prefs.winxsize = prefs.storedxsize;
                        prefs.winysize = prefs.storedysize;
                        prefs.windowPosition[WINMAIN] = prefs.windowPosition[WINMAINSTORED];
                        MakeWindow(true);
                        if (!SetUpWorlds())
                            StopError(13, -108);
                    }
                    UpdateAnimationTimer();
                }
            }
            break;
        case inContent:    // Aww, not satisfied?
            if (num != -1) {
                HandleWindowMouseDown(num);
            } else {
                SelectWindow(whichWindow);
                if (isAboutWindow)
                    DestroyAboutWindow();
                if (isMainWindow)
                    gDoodleInProgress = true;
            }
            break;
    }
    if (prefs.fullScreen)
        MyHideMenuBar();
}

void MaintainCursor(void) {   // 0=arrow 1=hidden 2=resize
    CGrafPtr curPort;
    short thisCursor;
    static CursHandle resizeCursorHandle = 0, hiddenCursorHandle;
    static short mbarHeight = -1;
    Point where;
    Cursor arrowCursor;
    Rect rect;

    if (mbarHeight < 0) {
        mbarHeight = GetMBarHeight();
        if (mbarHeight < 4)
            mbarHeight = 4;
    }
    GetPort(&curPort);
    SetPortWindowPort(gMainWindow);
    GetMouse(&where);
    thisCursor = 0;
    if (prefs.hideMouse && !gMouseHideOverride) {
        WindowPtr front = FrontWindow();
        Boolean frontIsDialog = true;
        short i = 0;
        if (front == gMainWindow)
            frontIsDialog = false;
        while (frontIsDialog && i < MAXWINDOW) {
            if (prefs.windowOpen[i]) {
                if (front == gWindow[i])
                    frontIsDialog = false;
            }
            ++i;
        }
        thisCursor = 0;
        if (!frontIsDialog) {   // we're allowed to hide the cursor
            GetWindowPortBounds(gMainWindow, &rect);
            if (PtInRect(where, &rect)) {   // and we're within the main window
                Boolean isOverToolWindow = false;
                i = 0;
                while (i < MAXWINDOW && !isOverToolWindow) {
                    if (prefs.windowOpen[i]) {
                        Point thisPoint;
                        SetPortWindowPort(gWindow[i]);
                        GetMouse(&thisPoint);
                        GetWindowPortBounds(gWindow[i], &rect);
                        rect.top -= 16;    // manually add the window's title bar
                        isOverToolWindow = (PtInRect(thisPoint, &rect));
                    }
                    i++;
                }
                if (!isOverToolWindow)
                    thisCursor = 1;
            }
        }
        if (prefs.fullScreen) {
            if (where.v < mbarHeight)
                thisCursor = 0;
        }
    }
    if (!prefs.fullScreen) {
        if (curPort == GetWindowPort(gMainWindow) && PtInRect(where, &gResizeBoxRect)) {
            thisCursor = 2;
        }
    }
    if (thisCursor != lastCursor) {
        lastCursor = thisCursor;
        switch (thisCursor) {
            case 0:    // arrow
                GetQDGlobalsArrow(&arrowCursor);
                SetCursor(&arrowCursor);
                break;
            case 1:    // hidden
                if (hiddenCursorHandle == 0)
                    hiddenCursorHandle = GetCursor(BASERES + 1);
                SetCursor(*hiddenCursorHandle);
                break;
            case 2:    // resize
                if (resizeCursorHandle == 0)
                    resizeCursorHandle = GetCursor(BASERES);
                SetCursor(*resizeCursorHandle);
                break;
        }
    }
    SetPort(curPort);
}

void ResetCursor(void) {
    lastCursor = 0;
    InitCursor();
}

void SetResizeBoxRect(void) {
    GetWindowPortBounds(gMainWindow, &gResizeBoxRect);
    gResizeBoxRect.left = gResizeBoxRect.right - 13;
    gResizeBoxRect.top = gResizeBoxRect.bottom - 13;
}

void HandleKeyDown(char theChar, EventModifiers modifiers) {
    ObscureCursor();

    if (modifiers & cmdKey) {
        switch (theChar) {
            case 28:    // <-
                prefs.soundInputSoftwareGain -= 10;
                if (prefs.soundInputSoftwareGain < 0)
                    prefs.soundInputSoftwareGain = 0;
                break;
            case 29:    // ->
                prefs.soundInputSoftwareGain += 10;
                if (prefs.soundInputSoftwareGain > 100)
                    prefs.soundInputSoftwareGain = 100;
                break;
            case '.':
                if (gQTExporting)
                    StopQTExport();
                break;
            default:
                if (modifiers & shiftKey) {
                    switch (theChar) {
                        case 'o': HandleFileChoice(OPENSETSID); break;
                        case 'h': HandleOptionsChoice(HIGHQUALITYID); break;
                        case 's': HandleOptionsChoice(WAVEOPTSID); break;
                        default: HandleMenuChoice(MenuKey(theChar)); break;
                    }
                } else
                    HandleMenuChoice(MenuKey(theChar));
                break;
        }
        ObscureCursor();
    } else {    // command key not held down
        short index;
        if (theChar >= 'a' && theChar <= 'z')
            theChar -= 32;
        if (modifiers & controlKey)
            theChar += '@';
        // Letters & 1-3 change effect
        if (theChar >= '1' && theChar <= '3')
            ToggleMirror(theChar - '1');
        index = (theChar - 'A');
        if (theChar >= 'A' && theChar <= 'Z') {
            //#warning control and non-control have been swapped
            if (!(modifiers & controlKey)) {   // filter change
                if (index < gNumEffects) {
                    if (modifiers & shiftKey)
                        HandleFiltersChoice(index + FIRSTFILTER);
                    else {
                        ClearAllFilters();
                        HandleFiltersChoice(index + FIRSTFILTER);
                    }
                }
            } else
                UsePixelToySet(index);
        }
        // 45678 change color sound effect
        if (theChar >= '4' && theChar <= '8') {
            curSet.paletteSoundMode = (theChar - '4');
            ShowSettings();
        }
    }
}

void HandleMenuChoice(long menuChoice) {
    short theMenu, theItem;

    if (menuChoice != 0) {
        theMenu = HiWord(menuChoice);
        theItem = LoWord(menuChoice);
        switch (theMenu) {
            case APPLEMENU: HandleAppleChoice(theItem); break;
            case FILEMENU: HandleFileChoice(theItem); break;
            case OPTIONSMENU: HandleOptionsChoice(theItem); break;
            case ACTIONSMENU: HandleActionsChoice(theItem); break;
            case FILTERSMENU: HandleFiltersChoice(theItem); break;
            case COLORSMENU: HandleColorsChoice(theItem); break;
            case SETSMENU: HandleSetsChoice(theItem); break;
            case WINDOWMENU: HandleWindowChoice(theItem); break;
            default:    // we shall assume it's the help menu.
                if (theItem == helpMenuIndex) {
                    FSSpec fss;
                    if (GetManualHTMLFSSpec(&fss) == noErr)
                        DoFinderOpenFSSpec(&fss);
                }
                break;
        }
        HiliteMenu(0);
    }
}

void HandleAppleChoice(short theItem) {
    Boolean soundWas;

    switch (theItem) {
        case ABOUTID:
            if (gAboutWindow == nil) {
                soundWas = curSet.sound;
                PlaySound(ABOUTNOISE, true);
                curSet.sound = soundWas;
                CreateAboutWindow();
            } else {
                SelectWindow(gAboutWindow);
            }
            break;
        default: break;
    }
}

void HandleFileChoice(short theItem) {
    short winNum, i;
    WindowPtr frontWindow;

    switch (theItem) {
        case OPENSETSID: GetAndUsePixelToySets(); break;
        case OPENID: GetAndDrawImage(); break;
        case RELOADID:
            DrawCurrentImage(false);
            if (gPause)
                UpdateToMain(false);    // draw even tho paused
            break;
        case SAVEID: UpdateToPNG(); break;
        case SAVESETSID: SaveSetsAs(); break;
        case EXPORTQTID: StartQTExport(); break;
        case CLEARID: ClearFromAndBase(); break;
        case PAUSEID:
            gPause = !gPause;
            UpdateAnimationTimer();
            ShowSettings();
            break;
        case CLOSEWINDOWID:
            frontWindow = FrontWindow();
            winNum = -1;
            for (i = 0; i < MAXWINDOW; i++) {
                if (prefs.windowOpen[i]) {
                    if (frontWindow == gWindow[i])
                        winNum = i;
                }
            }
            if (winNum != -1)
                PTDestroyWindow(winNum);
            if (frontWindow == gMainWindow)
                gDone = true;
            if (frontWindow == gAboutWindow)
                DestroyAboutWindow();
            break;
        case QUITID: gDone = true; break;
    }
}

void HandleOptionsChoice(short theItem) {
    long memNeeded, memCurrent;
    Boolean remember;
    char div;
    OSErr err;

    switch (theItem) {
        case LOWQUALITYID:
            memCurrent = MemoryRequiredCurrent();
            StopAnimationTimer();
            div = 2 + (2 * prefs.highQualityMode);
            oldWidth = prefs.winxsize;
            oldHeight = prefs.winysize;
            prefs.lowQualityMode = !prefs.lowQualityMode;
            if (prefs.lowQualityMode) {
                prefs.winxsize /= div;
                prefs.winysize /= div;
                prefs.storedxsize /= div;
                prefs.storedysize /= div;
            } else {
                prefs.winxsize *= div;
                prefs.winysize *= div;
                prefs.storedxsize *= div;
                prefs.storedysize *= div;
            }
            remember = prefs.highQualityMode;
            prefs.highQualityMode = false;
            memNeeded = MemoryRequiredCurrent();
            err = !CanAdjustWorlds(memNeeded - memCurrent);
            if (!err) {
                MakeWindow(true);
                err = !SetUpWorlds();
            }
            if (err) {
                prefs.lowQualityMode = !prefs.lowQualityMode;
                prefs.highQualityMode = remember;
                prefs.winxsize = oldWidth;
                prefs.winysize = oldHeight;
                MakeWindow(true);
                if (!SetUpWorlds())
                    StopError(13, -108);
            }
            UpdateAnimationTimer();
            break;
        case HIGHQUALITYID:
            memCurrent = MemoryRequiredCurrent();
            StopAnimationTimer();
            div = 2 + (2 * prefs.lowQualityMode);
            oldWidth = prefs.winxsize;
            oldHeight = prefs.winysize;
            prefs.highQualityMode = !prefs.highQualityMode;
            if (!prefs.highQualityMode) {
                prefs.winxsize /= div;
                prefs.winysize /= div;
                prefs.storedxsize /= div;
                prefs.storedysize /= div;
            } else {
                prefs.winxsize *= div;
                prefs.winysize *= div;
                prefs.storedxsize *= div;
                prefs.storedysize *= div;
            }
            remember = prefs.lowQualityMode;
            prefs.lowQualityMode = false;
            memNeeded = MemoryRequiredCurrent();
            err = !CanAdjustWorlds(memNeeded - memCurrent);
            if (!err) {
                MakeWindow(true);
                err = !SetUpWorlds();
            }

            if (err) {
                prefs.lowQualityMode = remember;
                prefs.highQualityMode = !prefs.highQualityMode;
                prefs.winxsize = oldWidth;
                prefs.winysize = oldHeight;
                MakeWindow(true);
                if (!SetUpWorlds())
                    StopError(13, -108);
            }
            UpdateAnimationTimer();
            break;
        case FULLSCREENID:
            prefs.fullScreen = !prefs.fullScreen;
            DoFullScreenTransition();
            break;
        case DITHERID:
            prefs.dither = !prefs.dither;
            SetCopyMode();
            break;
        case AUTOPILOTID: prefs.autoPilotActive = !prefs.autoPilotActive; break;
        case CONFIGPILOTID: ConfigureAutopilotDialog(); break;
        case PREFSID: PreferencesDialog(); break;
        case AFTEREFFECTID:
            CurSetTouched();
            curSet.postFilter = !curSet.postFilter;
            break;
        case EMBOSSID:
            CurSetTouched();
            curSet.emboss = !curSet.emboss;
            break;
        case WAVEOPTSID: WavesEditor(); break;
        case TEXTOPTSID: PixelToyTextEditor(); break;
        case PARTICOPTID: ParticleEditor(); break;
        case VARID: MiscOptionsDialog(); break;
        case BITMAPOPTID: ImageEditor(); break;
    }
    ShowSettings();
}

void HandleActionsChoice(short theItem) {
    short i;

    switch (theItem) {
        case BOUNCINGID:
            CurSetTouched();
            curSet.action[ACTLINES] = !curSet.action[ACTLINES];
            break;
        case RANDOMWALKID:
            CurSetTouched();
            curSet.action[ACTBALLS] = !curSet.action[ACTBALLS];
            break;
        case SWARMID:
            CurSetTouched();
            curSet.action[ACTSWARM] = !curSet.action[ACTSWARM];
            break;
        case RAINID:
            CurSetTouched();
            curSet.action[ACTDROPS] = !curSet.action[ACTDROPS];
            break;
        case SOUNDWAVEID:
            CurSetTouched();
            curSet.action[ACTSWAVE] = !curSet.action[ACTSWAVE];
            break;
        case DOODLEID:
            CurSetTouched();
            curSet.action[ACTDOODLE] = !curSet.action[ACTDOODLE];
            break;
        case TEXTID:
            CurSetTouched();
            curSet.action[ACTTEXT] = !curSet.action[ACTTEXT];
            break;
        case PARTICLEID:
            CurSetTouched();
            curSet.action[ACTPARTICLES] = !curSet.action[ACTPARTICLES];
            if (curSet.action[ACTPARTICLES])
                ResetParticles();
            break;
        case IMAGEID:
            CurSetTouched();
            curSet.action[ACTIMAGES] = !curSet.action[ACTIMAGES];
            if (!curSet.action[ACTIMAGES]) {   // just turned it off
                for (i = 0; i < MAXIMAGES; i++) {
                    ResetBMWorld(i, SB_BOTH);
                }
            }
            break;
    }
    ShowSettings();
}

void HandleFiltersChoice(short theItem) {
#ifdef LOGGING
    Str255 str;
    AddLog("\pMenu Filters: ", false);
    GetMenuItemText(gFiltersMenu, theItem, str);
    AddLog(str, true);
#endif
    CurSetTouched();
    switch (theItem) {
        case CLEARFILTERS: ClearAllFilters(); break;
        case EDITCUSTOM: EditCustomFilter(); break;
        case HORIZMIRROR: ToggleMirror(0); break;
        case VERTMIRROR: ToggleMirror(1); break;
        case CONSTRAINMIRROR: ToggleMirror(2); break;
        default:
            ToggleFilter(theItem - 4);    // 3=first effect menu entry
            break;
    }
    ShowSettings();
}

void HandleColorsChoice(short theItem) {
    switch (theItem) {
        case ADDPALETTEID: DoAddPaletteDialog(); break;
        case DELPALETTEID: DeletePaletteDialog(); break;
        case RENAMEPALETTEID: DoRenamePaletteDialog(-1); break;
        case RANDPALETTEID:
            if (ShiftIsPressed())
                prefs.roughPalette = !prefs.roughPalette;
            RandomPalette(curSet.emboss);
            EmployPalette(false);
            CurSetTouched();
            break;
        case PREVPALETTEID:
            UseColorPreset(-1, false);
            CurSetTouched();
            break;
        case NEXTPALETTEID:
            UseColorPreset(1, false);
            CurSetTouched();
            break;
        case NOAUDIOCOLOR:
        case AUDIOBLACK:
        case AUDIOWHITE:
        case AUDIOINVERT:
        case AUDIOROTATE:
            curSet.paletteSoundMode = (theItem - NOAUDIOCOLOR);
            ShowSettings();
            break;
        default:    // a palette resource
            UseColorPreset(theItem, false);
            CurSetTouched();
            break;
    }
}

void HandleSetsChoice(short theItem) {
    Boolean newSetChange;
    Str255 str;

    switch (theItem) {
        case ADDSETID: InsertNewSetDialog(); break;
        case DELSETID: DeleteOldSetDialog(); break;
        case RENAMESETID: RenameOldSet(-1); break;
        case EDITCOMMENTSID: EditSetComments(); break;
        case ZEROSETTINGSID: ZeroSettings(); break;
        case TIMESETID:
            newSetChange = !prefs.autoSetChange;
            if (newSetChange)
                newSetChange = GetAutoInterval();
            prefs.autoSetChange = newSetChange;
            if (prefs.autoSetChange) {
                GetIndString(str, BASERES, 16);
            } else {
                GetIndString(str, BASERES, 17);
            }
            if (gSetsMenu)
                SetMenuItemText(gSetsMenu, TIMESETID, str);
            ShowSettings();
            break;
        case PREVSETID: UsePixelToySet(-1); break;
        case NEXTSETID: UsePixelToySet(-2); break;
        default:
            UsePixelToySet(theItem - (NEXTSETID + 2));    // 0-whatever
            break;
    }
}

OSErr GetManualHTMLFSSpec(FSSpec *manual) {
    FSSpec fss;
    OSErr err;
    Str255 str = "\p";
    GetIndString(str, BASERES, 59);    // Manual:manual.html
    if (str[0] < 1)
        return bdNamErr;
    err = FSMakeFSSpec(0, nil, str, &fss);
    if (err == noErr && manual)
        BlockMoveData(&fss, manual, sizeof(FSSpec));
    return err;
}

OSStatus DoFinderOpenFSSpec(FSSpec *fss) {
    AEAddressDesc fndrAddress;
    AEDescList targetListDesc;
    AppleEvent theAEvent, theReply;
    OSType fndrCreator = 'MACS';
    OSStatus err;
    AliasHandle theAlias;

    AECreateDesc(typeNull, NULL, 0, &fndrAddress);
    AECreateDesc(typeNull, NULL, 0, &theAEvent);
    AECreateDesc(typeNull, NULL, 0, &targetListDesc);
    AECreateDesc(typeNull, NULL, 0, &theReply);
    err = NewAlias(nil, fss, &theAlias);
    if (err == noErr) {
        err = AECreateDesc(typeApplSignature, (Ptr)&fndrCreator, sizeof(fndrCreator), &fndrAddress);
        if (err == noErr) {
            err = AECreateAppleEvent(kCoreEventClass, kAEOpenDocuments, &fndrAddress, kAutoGenerateReturnID, kAnyTransactionID,
                                     &theAEvent);
            if (err == noErr) {
                err = AECreateList(NULL, 0, false, &targetListDesc);
                if (err == noErr) {
                    HLock((Handle)theAlias);
                    err = AEPutPtr(&targetListDesc, 1, typeAlias, *theAlias, GetHandleSize((Handle)theAlias));
                    HUnlock((Handle)theAlias);
                    err = AEPutParamDesc(&theAEvent, keyDirectObject, &targetListDesc);
                    if (err == noErr) {
                        err = AESend(&theAEvent, &theReply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, NULL, NULL);
                    }
                }
            }
        }
        DisposeHandle((Handle)theAlias);
    }
    if (err != noErr)
        NoteError(84, err);
    AEDisposeDesc(&targetListDesc);
    AEDisposeDesc(&theAEvent);
    AEDisposeDesc(&fndrAddress);
    AEDisposeDesc(&theReply);
    return err;
}

void SetCopyMode(void) {
    gCopyMode = srcCopy;
    if (prefs.dither)
        gCopyMode = ditherCopy;
}

void MenuBarInit(void) {
    Handle myMenuBar;

    myMenuBar = GetNewMBar(128);
    SetMenuBar(myMenuBar);
    gAppleMenu = GetMenuHandle(APPLEMENU);
    gFileMenu = GetMenuHandle(FILEMENU);
    gOptionsMenu = GetMenuHandle(OPTIONSMENU);
    gActionsMenu = GetMenuHandle(ACTIONSMENU);
    gFiltersMenu = GetMenuHandle(FILTERSMENU);
    gWindowMenu = GetMenuHandle(WINDOWMENU);

    PopulateColorsMenu();
    if (IsMacOSX()) {
        // Enable app menu Preferences command
        EnableMenuCommand(NULL, kHICommandPreferences);
        // Remove Quit item & preceding divider
        DeleteMenuItem(gFileMenu, QUITID);
        DeleteMenuItem(gFileMenu, QUITID - 1);
    }
    // Change "hotkey" for High Quality to command-SHIFT-H
    SetMenuItemModifiers(gOptionsMenu, HIGHQUALITYID, kMenuShiftModifier);
    SetMenuItemModifiers(gFileMenu, OPENSETSID, kMenuShiftModifier);
    if (gFiltersMenu)
        gNumEffects = CountMenuItems(gFiltersMenu);
    DrawMenuBar();
}

void DoFullScreenTransition(void) {
    short i;
    Rect scr;
    OSErr err = noErr;
    WindowPtr tempWindow;
    long memCurrent, memNeeded;

    if (prefs.fullScreen == gCurFullScreenState)
        return;
    gCurFullScreenState = prefs.fullScreen;
    StopAnimationTimer();
    if (!prefs.fullScreen) {   // from full-screen to windowed
        oldWidth = prefs.winxsize;
        oldHeight = prefs.winysize;
        prefs.winxsize = prefs.storedxsize;
        prefs.winysize = prefs.storedysize;
        prefs.windowPosition[WINMAIN] = prefs.windowPosition[WINMAINSTORED];
        EnsureSystemPalette();
        MyShowMenuBar();
        MakeWindow(false);
        if (prefs.changeResolution)
            SwitchOutResolution();
        if (!SetUpWorlds()) {   // jeeze, this shouldn't happen.
            prefs.windowPosition[WINMAIN].h = prefs.windowPosition[WINMAIN].v = -1;
            prefs.winxsize = 320;
            prefs.winysize = 240;
            MakeWindow(true);
            if (!SetUpWorlds())
                StopError(13, -108);
        }
        PopPal();
        PopWindows();
        UpdateAnimationTimer();
        return;
    }

    // from windowed to full-screen
    // Do we have enough memory to do this?
    memCurrent = MemoryRequiredCurrent();
    if (prefs.changeResolution) {
        memNeeded = MemoryRequiredWithValues(prefs.mon.h, prefs.mon.v, prefs.lowQualityMode, prefs.highQualityMode);
    } else {
        GetWindowDeviceRect(gMainWindow, &scr);
        memNeeded = MemoryRequiredWithValues(scr.right, scr.bottom, prefs.lowQualityMode, prefs.highQualityMode);
    }
    if (CanAdjustWorlds(memNeeded - memCurrent)) {
        // First time, ask if user will want resolution changing.
        if (!prefs.notFirstFullScreen) {
            StopAutoChanges();
            EnsureSystemPalette();
            i = CautionAlert(BASERES + 5, nil);    // switch now? no DialogFilterProc
            if (i == 1) {                           // yes please
                prefs.changeResolution = true;
            }
            PopPal();
            ResumeAutoChanges();
        }
        // First time, let them know how to get out of full screen.
        if (!prefs.notFirstFullScreen)
            DoStandardAlert(kAlertNoteAlert, 21);

        // Store current window info for restoration later
        prefs.notFirstFullScreen = true;
        PushWindows();
        oldWidth = prefs.winxsize;
        oldHeight = prefs.winysize;
        prefs.storedxsize = prefs.winxsize;
        prefs.storedysize = prefs.winysize;
        prefs.windowPosition[WINMAINSTORED] = prefs.windowPosition[WINMAIN];

        // Change resolution with RezLib or just hide the menu bar.
        if (prefs.changeResolution) {   // RezLib change to defined monitor res
            GetWindowDevice(gMainWindow);    // windowDevice
            err = SwitchInResolution();
            scr = (*windowDevice)->gdRect;
        } else {
            MyHideMenuBar();
            GetWindowDeviceRect(gMainWindow, &scr);
        }
        prefs.windowPosition[WINMAIN].h = scr.left;
        prefs.windowPosition[WINMAIN].v = scr.top;
        prefs.winxsize = scr.right - scr.left;
        prefs.winysize = scr.bottom - scr.top;

        if (err) {   // Rezlib shit itself, so abort switch to full-screen
            gCurFullScreenState = prefs.fullScreen = false;
            prefs.winxsize = prefs.storedxsize;
            prefs.winysize = prefs.storedysize;
            prefs.windowPosition[WINMAIN] = prefs.windowPosition[WINMAINSTORED];
            EnsureSystemPalette();
            MyShowMenuBar();
            MakeWindow(false);
            if (!SetUpWorlds())
                StopError(13, -108);
            PopPal();
            PopWindows();
        } else {   // Either not using RezLib or it worked fine, so ...
            if (prefs.highQualityMode) {
                prefs.winxsize *= 2;
                prefs.winysize *= 2;
            }
            if (prefs.lowQualityMode) {
                prefs.winxsize /= 2;
                prefs.winysize /= 2;
            }
            MakeWindow(true);

            // Create a window and destroy it to force removal of corners
            tempWindow = NewCWindow(nil, &scr, nil, false, 0, (WindowPtr)-1, false, 0);
            ForceOnScreen(tempWindow);
            ShowWindow(tempWindow);
            DisposeWindow(tempWindow);

            if (!SetUpWorlds()) {   // Couldn't resize worlds for some reason
                if (prefs.changeResolution)
                    SwitchOutResolution();
                gCurFullScreenState = prefs.fullScreen = false;
                prefs.winxsize = prefs.storedxsize;
                prefs.winysize = prefs.storedysize;
                prefs.windowPosition[WINMAIN] = prefs.windowPosition[WINMAINSTORED];
                MyShowMenuBar();
                MakeWindow(true);
                if (!SetUpWorlds())
                    StopError(13, -108);
                PopPal();
                PopWindows();
            }
            ObscureCursor();
        }
    } else {   // Don't think we have enough memory for this.
        gCurFullScreenState = prefs.fullScreen = false;
    }
    UpdateAnimationTimer();
}

void MakeWindow(Boolean forceOnScreen) {
    Rect winRect, mainRect;
    short windowType, pixelWidth, pixelHeight, screenWidth, screenHeight;
    BitMap qdbm;

    pixelWidth = prefs.winxsize;
    pixelHeight = prefs.winysize;
    if (prefs.highQualityMode) {
        pixelWidth /= 2;
        pixelHeight /= 2;
    }
    if (prefs.lowQualityMode) {
        pixelWidth *= 2;
        pixelHeight *= 2;
    }
    if (prefs.windowPosition[WINMAIN].h == -1) {   // default to centered on main screen
        GetQDGlobalsScreenBits(&qdbm);
        mainRect = qdbm.bounds;
        screenWidth = mainRect.right;
        screenHeight = mainRect.bottom;
        if (!prefs.fullScreen) {
            screenHeight -= GetMBarHeight();
            screenWidth -= 4;
        }
        prefs.windowPosition[WINMAIN].h = (screenWidth / 2) - (prefs.winxsize / 2);
        prefs.windowPosition[WINMAIN].v = (screenHeight / 2) - (prefs.winysize / 2);
    }
    winRect.left = prefs.windowPosition[WINMAIN].h;
    winRect.top = prefs.windowPosition[WINMAIN].v;
    if (prefs.highQualityMode) {
        winRect.right = winRect.left + (prefs.winxsize / 2);
        winRect.bottom = winRect.top + (prefs.winysize / 2);
    }
    if (prefs.lowQualityMode) {
        winRect.right = winRect.left + (prefs.winxsize * 2);
        winRect.bottom = winRect.top + (prefs.winysize * 2);
    }
    if (!prefs.lowQualityMode && !prefs.highQualityMode) {
        winRect.right = winRect.left + prefs.winxsize;
        winRect.bottom = winRect.top + prefs.winysize;
    }

    if (prefs.fullScreen)
        windowType = kWindowDocumentProc;    // kWindowModalDialogProc
    else
        windowType = kWindowFullZoomDocumentProc;

    // Create window if doesn't exist, otherwise move, size, and type.
    if (!gMainWindow) {
        gMainWindow = NewCWindow(nil, &winRect, "\p", true, windowType, nil, true, 0);
    } else {
        MoveWindow(gMainWindow, prefs.windowPosition[WINMAIN].h, prefs.windowPosition[WINMAIN].v, false);
        SizeWindow(gMainWindow, winRect.right - winRect.left, winRect.bottom - winRect.top, true);
        if (GetWindowKind(gMainWindow) != windowType) {
            SetWindowKind(gMainWindow, windowType);
        }
    }

    if (forceOnScreen && !gCurFullScreenState)
        ForceOnScreen(gMainWindow);
    ShowWindow(gMainWindow);
    MySetWindowTitle();

    SetPortWindowPort(gMainWindow);
    SetRect(&gImageRect, 0, 0, prefs.winxsize, prefs.winysize);
    SetResizeBoxRect();
    if (prefs.fullScreen)
        MyHideMenuBar();
    SendBehind(gMainWindow, 0);
}

void CreateWorlds(void) {
    OSErr err;
    Rect nullRect;

    SetRect(&nullRect, 0, 0, 32, 32);

    err = NewGWorld(&offWorld, 8, &nullRect, mainColorTable, nil, 0);
    if (err != noErr)
        StopError(3, err);
    SaveContext();
    SetGWorld(offWorld, nil);
    BackColor(blackColor);
    EraseRect(&nullRect);
    RestoreContext();
    err = NewGWorld(&highQualityWorld, 32, &nullRect, nil, nil, 0);
    if (err != noErr)
        StopError(4, err);
    err = NewGWorld(&blurWorld, 8, &nullRect, mainColorTable, nil, 0);
    if (err != noErr)
        StopError(5, err);
    err = NewGWorld(&doubleWorld, 8, &nullRect, mainColorTable, nil, 0);
    if (err != noErr)
        StopError(6, err);
    err = NewGWorld(&stage32, 32, &nullRect, nil, nil, 0);
    if (err != noErr)
        StopError(58, err);
}

long MemoryRequiredCurrent(void) {
    Rect curWinRect;

    GetWindowPortBounds(gMainWindow, &curWinRect);
    return MemoryRequiredWithValues(curWinRect.right, curWinRect.bottom, prefs.lowQualityMode, prefs.highQualityMode);
}

long MemoryRequiredWithValues(short winWidth, short winHeight, Boolean lowQualityMode, Boolean highQuality) {
    long memNeeded = 0;
    short offWidth, offHeight;

    if (winWidth % 32)
        winWidth += (32 - (winWidth % 32));    // simulate rowBytes padding
    offWidth = winWidth;
    offHeight = winHeight;
    if (lowQualityMode) {
        offWidth /= 2;
        offHeight /= 2;
    }
    if (highQuality) {
        offWidth *= 2;
        offHeight *= 2;
    }

    // stage32             32 bits        width*height, lowQualityMode/highQuality ignored
    // highQualityWorld    32 bits        if highQuality, width*2 * height*2, else 32x32
    // doubleWorld          8 bits        if lowQualityMode, width*height, else 32x32
    // blurWorld            8 bits        offWidth*offHeight
    // offWorld             8 bits        offWidth*offHeight

    memNeeded += (winWidth * winHeight) * 4;

    if (highQuality)
        memNeeded += ((winWidth * 2) * (winHeight * 2)) * 4;
    else
        memNeeded += (32 * 32) * 4;

    if (lowQualityMode)
        memNeeded += (winWidth * winHeight);
    else
        memNeeded += (32 * 32);

    memNeeded += (offWidth * offHeight) * 2;

    return memNeeded;
}

Boolean CanAdjustWorlds(long numberOfBytesChange) {
    if (IsMacOSX())
        return true;
    if (numberOfBytesChange < 0)
        return true;
    if (numberOfBytesChange > MaxBlock()) {
        DoStandardAlert(kAlertStopAlert, 18);
        return false;
    }
    return true;
}

// Updates all GWorlds to new sizes if necessary
Boolean SetUpWorlds(void) {   // returns true if successful
    Boolean err = false, hqShrinking, doubleShrinking;
    short y;
    float sizeX, sizeY;
    Rect newRect, oldRect;

    // Determine if doubleWorld is growing or shrinking
    GetPortBounds(doubleWorld, &oldRect);
    if (prefs.lowQualityMode)
        SetRect(&gSizeRect, 0, 0, prefs.winxsize * 2, prefs.winysize * 2);
    else
        SetRect(&gSizeRect, 0, 0, 32, 32);
    doubleShrinking = ((oldRect.bottom * oldRect.right) > (gSizeRect.bottom * gSizeRect.right));

    // If it's shrinking, do it now
    if (doubleShrinking) {
        err = UpdateGWorld(&doubleWorld, 8, &gSizeRect, mainColorTable, nil, clipPix);
        if (err != noErr) {
            DisposeGWorld(doubleWorld);
            err = NewGWorld(&doubleWorld, 8, &gSizeRect, mainColorTable, nil, 0);
            if (err)
                StopError(81, err);
        }
    }

    // Determine if highQualityWorld is growing or shrinking
    GetPortBounds(highQualityWorld, &oldRect);
    if (prefs.highQualityMode)
        SetRect(&newRect, 0, 0, prefs.winxsize, prefs.winysize);
    else
        SetRect(&newRect, 0, 0, 32, 32);
    hqShrinking = ((oldRect.bottom * oldRect.right) > (newRect.bottom * newRect.right));

    // If it's shrinking, do it now
    if (hqShrinking) {
        if (!MacEqualRect(&oldRect, &newRect)) {
            err = UpdateGWorld(&highQualityWorld, 32, &newRect, nil, nil, clipPix);
            if (err != noErr) {
                DisposeGWorld(highQualityWorld);
                err = NewGWorld(&highQualityWorld, 32, &newRect, nil, nil, 0);
                if (err)
                    StopError(80, err);
            }
        }
    }

    GetPortBounds(stage32, &oldRect);
    GetWindowPortBounds(gMainWindow, &newRect);
    if (!MacEqualRect(&oldRect, &newRect)) {
        err = UpdateGWorld(&stage32, 32, &newRect, nil, nil, clipPix);
        if (err != noErr) {
            DisposeGWorld(stage32);
            err = NewGWorld(&stage32, 32, &newRect, nil, nil, 0);
            if (err)
                StopError(79, err);
        }
    }

    GetPortBounds(blurWorld, &oldRect);
    if (!MacEqualRect(&oldRect, &gImageRect)) {
        err = UpdateGWorld(&blurWorld, 8, &gImageRect, mainColorTable, nil, clipPix);
        if (err != noErr) {
            DisposeGWorld(blurWorld);
            err = NewGWorld(&blurWorld, 8, &gImageRect, mainColorTable, nil, 0);
            if (err)
                StopError(82, err);
        }
    }

    // Back up contents of old offworld & update
    if (!err) {
        if (offWorld) {
            fromPixMap = GetGWorldPixMap(offWorld);
            err = !LockPixels(fromPixMap);
            fromBasePtr = GetPixBaseAddr(fromPixMap);
            fromByteCount = (0x7FFF & (**fromPixMap).rowBytes);
            if (!err)
                ResChangeBitsPush();
            UnlockPixels(fromPixMap);
        }

        GetPortBounds(offWorld, &oldRect);
        if (!MacEqualRect(&oldRect, &gImageRect)) {
            err = UpdateGWorld(&offWorld, 8, &gImageRect, mainColorTable, nil, clipPix);
            if (err) {
                DisposeGWorld(offWorld);
                err = NewGWorld(&offWorld, 8, &gImageRect, mainColorTable, nil, 0);
                if (err) {
                    ResChangeBitsRelease();
                    err = NewGWorld(&offWorld, 8, &gImageRect, mainColorTable, nil, 0);
                }
            }
            if (err != noErr)
                StopError(83, QDError());
            err = noErr;
        }
    }

    // More setup
    if (!err) {
        if (LockAllWorlds()) {
            SaveContext();
            SetGWorld(offWorld, 0);
            SetForeAndBackColors();
            PaintRect(&gImageRect);
            SetGWorld(blurWorld, 0);
            ForeColor(blackColor);
            PaintRect(&gImageRect);
            SetGWorld(doubleWorld, nil);
            ForeColor(blackColor);
            BackColor(whiteColor);
            RestoreContext();
            UnlockAllWorlds();
        }
        TextSize(9);
        TextMode(srcCopy);    // TextFont(geneva);
    }

    if (!err) {   // and final setup
        ResChangeBitsPop();
        if (oldWidth) {
            if (oldWidth != prefs.winxsize || oldHeight != prefs.winysize) {
                sizeX = (float)prefs.winxsize / (float)oldWidth;
                sizeY = (float)prefs.winysize / (float)oldHeight;
                for (y = 0; y < MAXTEXTS; y++) {
                    curTextXLoc[y] *= sizeX;
                    curTextYLoc[y] *= sizeY;
                }
            }
        }
    }

    // doubleWorld isn't shrinking so we saved it for last.
    if (!err && !doubleShrinking) {
        GetPortBounds(doubleWorld, &oldRect);
        if (!MacEqualRect(&oldRect, &gSizeRect)) {
            err = UpdateGWorld(&doubleWorld, 8, &gSizeRect, mainColorTable, nil, clipPix);
            if (err != noErr) {
                DisposeGWorld(doubleWorld);
                err = NewGWorld(&doubleWorld, 8, &gSizeRect, mainColorTable, nil, 0);
                if (err)
                    StopError(81, err);
            }
        }
    }

    // highQualityWorld isn't shrinking so we saved it for last.
    if (!err && !hqShrinking) {
        GetPortBounds(highQualityWorld, &oldRect);
        if (prefs.highQualityMode)
            SetRect(&newRect, 0, 0, prefs.winxsize, prefs.winysize);
        else
            SetRect(&newRect, 0, 0, 32, 32);
        if (!MacEqualRect(&oldRect, &newRect)) {
            err = UpdateGWorld(&highQualityWorld, 32, &newRect, nil, nil, clipPix);
            if (err != noErr) {
                DisposeGWorld(highQualityWorld);
                err = NewGWorld(&highQualityWorld, 32, &newRect, nil, nil, 0);
                if (err)
                    StopError(80, err);
            }
        }
    }

    if (err) {
        EnsureSystemPalette();
        DoStandardAlert(kAlertStopAlert, 18);
        PopPal();
    }
    return (!err);
}

Boolean LockAllWorlds(void) {
    OSErr err;

    fromPixMap = GetGWorldPixMap(offWorld);
    err = !PTLockPixels(fromPixMap);
    fromBasePtr = GetPixBaseAddr(fromPixMap);
    fromByteCount = (0x7FFF & (**fromPixMap).rowBytes);

    if (!err) {
        blurPixMap = GetGWorldPixMap(blurWorld);
        err = !PTLockPixels(blurPixMap);
        blurBasePtr = GetPixBaseAddr(blurPixMap);
        blurByteCount = (0x7FFF & (**blurPixMap).rowBytes);
    }

    if (!err) {
        highQualityPixMap = GetGWorldPixMap(highQualityWorld);
        err = !PTLockPixels(highQualityPixMap);
    }

    if (!err) {
        stage32PixMap = GetGWorldPixMap(stage32);
        err = !PTLockPixels(stage32PixMap);
        if (prefs.lowQualityMode) {
            doublePixMap = GetGWorldPixMap(doubleWorld);
            err = !PTLockPixels(doublePixMap);
            doubleBasePtr = GetPixBaseAddr(doublePixMap);
            doubleByteCount = (0x7FFF & (**doublePixMap).rowBytes);

            gCopyFromRect = gSizeRect;
            InsetRect(&gCopyFromRect, FRAMESIZE * 2, FRAMESIZE * 2);
            GetWindowPortBounds(gMainWindow, &gCopyToRect);
            InsetRect(&gCopyToRect, FRAMESIZE * 2, FRAMESIZE * 2);
        } else {
            gCopyFromRect = gImageRect;
            InsetRect(&gCopyFromRect, FRAMESIZE, FRAMESIZE);
            GetWindowPortBounds(gMainWindow, &gCopyToRect);
            InsetRect(&gCopyToRect, FRAMESIZE, FRAMESIZE);
        }
    }
    return (err == noErr);
}

void UnlockAllWorlds(void) {
    UnlockPixels(fromPixMap);
    UnlockPixels(blurPixMap);
    UnlockPixels(highQualityPixMap);
    UnlockPixels(stage32PixMap);
    if (prefs.lowQualityMode) {
        UnlockPixels(doublePixMap);
    }
}

void DoLines(void) {
    long temp;
    float xs, ys, varianceSizer;
    short ox, oy, x, y, sizer, thisSize, halfSize, num;
    float useWidth, useHeight;
    RGBColor drawColor;

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
    Index2Color((long)255, &drawColor);
    RGBForeColor(&drawColor);
    sizer = curSet.actSiz[ACTLINES] / 10 + 1;
    varianceSizer = curSet.actSizeVar[ACTLINES] / (float)500;
    xs = (float)32768 / useWidth;
    ys = (float)32768 / useHeight;
    if (curSet.actReactSnd[ACTLINES]) {
        thisSize = PixelSize(RecentVolume() / 10);
        halfSize = thisSize / 2;
    }
    // find an empty spot and add one
    num = 0;
    while (num < curSet.actCount[ACTLINES] && ptline[num].x) {
        num++;
    }
    if (num < curSet.actCount[ACTLINES]) {
        ptline[num].x = RandNum(xs, 32767 - xs);
        ptline[num].y = 0;
        ptline[num].dx = RandVariance(1500);
        ptline[num].dy = RandNum(0, 1000);
        ptline[num].weight = RandVariance(100);
        ptline[num].gravity = ((ptline[num].weight + 100) / 10) + 10;
    }
    // update all lines
    for (num = 0; num < curSet.actCount[ACTLINES]; num++) {
        if (ptline[num].x) {
            ox = ptline[num].x / xs;
            oy = ptline[num].y / ys;

            temp = ptline[num].x + ptline[num].dx;
            if (temp < xs) {
                temp = xs;
                ptline[num].dx = -ptline[num].dx * .9;
            }
            if (temp > 32767 - xs) {
                temp = 32767 - xs;
                ptline[num].dx = -ptline[num].dx * .9;
            }
            ptline[num].x = temp;

            temp = ptline[num].y + ptline[num].dy;
            if (temp > 32767 - ys) {
                temp = 32767 - ys;
                ptline[num].dy = -ptline[num].dy * .9;
                ptline[num].dx *= .95;
            }
            ptline[num].y = temp;

            ptline[num].dy += ptline[num].gravity;
            x = ptline[num].x / xs;
            y = ptline[num].y / ys;
            if (!curSet.actReactSnd[ACTLINES]) {
                thisSize = sizer;
                thisSize += (ptline[num].weight * varianceSizer);
                thisSize = PixelSize(thisSize);
                if (thisSize < 1)
                    thisSize = 1;
                halfSize = thisSize / 2;
            }
            PenSize(thisSize, thisSize);
            MoveTo(ox - halfSize, oy - halfSize);
            LineTo(x - halfSize, y - halfSize);
            if (LongAbsolute(ptline[num].dy) < ys && ptline[num].y > 32767 - (ys * 2)) {
                ptline[num].x = 0;
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
}

void DoBalls(void) {
    long nx, ny;
    short num, size, halfsize, maxspeed;
    float xs, ys, sizer;
    Rect walkRect;
    float useWidth, useHeight;

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
    sizer = curSet.actSizeVar[ACTBALLS] / (float)200;
    xs = (float)16384 / useWidth;
    ys = (float)16384 / useHeight;
    // add a new ball if a spot is vacant
    num = 0;
    while (num < curSet.actCount[ACTBALLS] && ptwalk[num].x != 0) {
        num++;
    }
    if (num < curSet.actCount[ACTBALLS]) {
        ptwalk[num].size = RandVariance(100);
        if (curSet.actMisc1[ACTBALLS]) {   // appear randomly
            ptwalk[num].x = RandNum(0, 16384);
            ptwalk[num].y = RandNum(0, 16384);
            ptwalk[num].dx = RandVariance(100);
            ptwalk[num].dy = RandVariance(100);
        } else {   // come from offscreen
            switch (RandNum(0, 4)) {
                case 0:    // arrive on left
                    ptwalk[num].x = 1;
                    ptwalk[num].y = RandNum(0, 16384);
                    ptwalk[num].dx = RandNum(25, 100);
                    ptwalk[num].dy = RandVariance(25);
                    break;
                case 1:    // arrive on right
                    ptwalk[num].x = 16384;
                    ptwalk[num].y = RandNum(0, 16384);
                    ptwalk[num].dx = RandNum(-100, -25);
                    ptwalk[num].dy = RandVariance(25);
                    break;
                case 2:    // arrive on top
                    ptwalk[num].y = 1;
                    ptwalk[num].x = RandNum(0, 16384);
                    ptwalk[num].dx = RandVariance(25);
                    ptwalk[num].dy = RandNum(25, 100);
                    break;
                case 3:    // arrive on bottom
                    ptwalk[num].y = 16384;
                    ptwalk[num].x = RandNum(0, 16384);
                    ptwalk[num].dx = RandVariance(25);
                    ptwalk[num].dy = RandNum(-100, -25);
                    break;
            }
        }
    }

    for (num = 0; num < curSet.actCount[ACTBALLS]; num++) {
        if (ptwalk[num].x) {
            if (curSet.actReactSnd[ACTBALLS]) {
                size = RecentVolume();
            } else {
                size = (curSet.actSiz[ACTBALLS] + ptwalk[num].size * sizer) * 2;
            }
            if (!prefs.lowQualityMode && !prefs.highQualityMode)
                size /= 2;
            if (prefs.lowQualityMode)
                size /= 4;
            if (!curSet.actReactSnd[ACTBALLS]) {
                if (size < 1)
                    size = 1;
            }
            halfsize = size / 2;
            maxspeed = (size * xs) / 16;
            if (maxspeed < xs)
                maxspeed = xs;

            nx = ptwalk[num].x + ptwalk[num].dx;
            ny = ptwalk[num].y + ptwalk[num].dy;
            ptwalk[num].dx += RandVariance(25);
            ptwalk[num].dy += RandVariance(25);
            if (ptwalk[num].dx > maxspeed)
                ptwalk[num].dx -= xs;
            if (ptwalk[num].dx < -maxspeed)
                ptwalk[num].dx += xs;
            if (ptwalk[num].dy > maxspeed)
                ptwalk[num].dy -= xs;
            if (ptwalk[num].dy < -maxspeed)
                ptwalk[num].dy += xs;
            if (curSet.actMisc1[ACTBALLS]) {   // constrain on-screen
                if (nx < (halfsize * xs)) {
                    nx = halfsize * xs;
                    ptwalk[num].dx = -ptwalk[num].dx;
                }
                if (nx > 16384 - (halfsize * xs)) {
                    nx = 16384 - (halfsize * xs);
                    ptwalk[num].dx = -ptwalk[num].dx;
                }
                if (ny < (halfsize * ys)) {
                    ny = halfsize * ys;
                    ptwalk[num].dy = -ptwalk[num].dy;
                }
                if (ny > 16384 - (halfsize * ys)) {
                    ny = 16384 - (halfsize * ys);
                    ptwalk[num].dy = -ptwalk[num].dy;
                }
            } else {
                if (nx < -(size * xs) || nx > 16384 + (size * xs) || ny < -(size * ys) || ny > 16384 + (size * ys))
                    ptwalk[num].x = 0;
            }
            if (ptwalk[num].x) {
                ptwalk[num].x = (short)nx;
                ptwalk[num].y = (short)ny;
                walkRect.left = (ptwalk[num].x / xs) - halfsize;
                walkRect.top = (ptwalk[num].y / ys) - halfsize;
                walkRect.right = walkRect.left + size;
                walkRect.bottom = walkRect.top + size;
                PaintOval(&walkRect);
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
}

void DoDoodle(void) {
    Point mousePoint;
    short penSz, hps;
    static short doodleX, doodleY;

    if (!curSet.action[ACTDOODLE])
        return;
    if (gDoodleInProgress) {
        if (LockAllWorlds()) {
            SetPortWindowPort(gMainWindow);
            GetMouse(&mousePoint);
            if (curSet.actReactSnd[ACTDOODLE]) {
                penSz = RecentVolume();
            } else {
                penSz = curSet.actSiz[ACTDOODLE];
            }
            if (prefs.lowQualityMode)
                penSz /= 4;
            if (!prefs.lowQualityMode && !prefs.highQualityMode)
                penSz /= 2;
            if (penSz < 1 && !curSet.actReactSnd[ACTDOODLE])
                penSz = 1;
            hps = penSz / 2;
            if (prefs.lowQualityMode) {
                mousePoint.h /= 2;
                mousePoint.v /= 2;
            }
            if (prefs.highQualityMode) {
                mousePoint.h *= 2;
                mousePoint.v *= 2;
            }
            SetGWorld(offWorld, nil);
            PenSize(penSz, penSz);
            if (!gFirstDoodle) {
                MoveTo(doodleX - hps, doodleY - hps);
                LineTo(mousePoint.h - hps, mousePoint.v - hps);
            } else {
                gFirstDoodle = false;
            }
            doodleX = mousePoint.h;
            doodleY = mousePoint.v;
            UnlockAllWorlds();
        }
        RestoreContext();
    } else
        gFirstDoodle = true;
}

void DoSwarm(void) {
    short bee;
    float multX, multY, xs, ys;
    long offset;
    short targetSpeedX, targetSpeedY, ox, oy, sizer, beeSize;
    short queenReact, droneReact, queenMaxSp = 256, droneMaxSp = 128;
    Boolean firstTime, hideQueen, followMouse;
    Point mouse;
    float useWidth, useHeight;

    if (!LockAllWorlds())
        return;
    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    SaveContext();
    xs = (float)16384 / useWidth;
    ys = (float)16384 / useHeight;
    queenReact = 12;
    droneReact = curSet.actSizeVar[ACTSWARM] / 12 + 1;
    followMouse = curSet.actMisc1[ACTSWARM] & 0x01;
    hideQueen = curSet.actMisc1[ACTSWARM] & 0x02;
    if (followMouse) {
        SetPortWindowPort(gMainWindow);
        GetMouse(&mouse);
        targetX = mouse.h * xs;
        targetY = mouse.v * ys;
        if (prefs.lowQualityMode) {
            targetX /= 2;
            targetY /= 2;
        }
        if (prefs.highQualityMode) {
            targetX *= 2;
            targetY *= 2;
        }
        if (targetX < 1)
            targetX = 1;
        if (targetY < 1)
            targetY = 1;
        if (targetX > 16384)
            targetX = 16384;
        if (targetY > 16384)
            targetY = 16384;
    } else {
        if (!hideQueen) {
            ox = targetX / xs;
            oy = targetY / ys;
            offset = (fromByteCount * oy + ox);
            fromBasePtr[offset] = 128;
        }
    }
    sizer = PixelSize(curSet.actSiz[ACTSWARM]) / 10;
    if (sizer < 1)
        sizer = 1;
    if (curSet.actReactSnd[ACTSWARM])
        sizer = RecentVolume() / 4;
    SetGWorld(offWorld, nil);
    for (bee = 0; bee < curSet.actCount[ACTSWARM]; bee++) {
        firstTime = false;
        ox = ptbee[bee].x;
        oy = ptbee[bee].y;
        if (ptbee[bee].x == 0) {
            firstTime = true;
            ptbee[bee].x = RandNum(1, 16384);
            ptbee[bee].y = RandNum(1, 16384);
            ptbee[bee].dx = 0;
            ptbee[bee].dy = 0;
            if (bee == 0) {
                targetX = RandNum(1, 16384);
                targetY = RandNum(1, 16384);
            }
        }
        if (bee == 0) {   // the leader
            multX = Absolute(targetX - ptbee[bee].x);
            multY = Absolute(targetY - ptbee[bee].y);
            if (multX > multY) {
                if (targetX > ptbee[bee].x) {
                    targetSpeedX = queenMaxSp;
                } else {
                    targetSpeedX = -queenMaxSp;
                }
                if (targetY > ptbee[bee].y) {
                    targetSpeedY = queenMaxSp / (multX / multY);
                } else {
                    targetSpeedY = -(queenMaxSp / (multX / multY));
                }
            } else {
                if (targetY > ptbee[bee].y) {
                    targetSpeedY = queenMaxSp;
                } else {
                    targetSpeedY = -queenMaxSp;
                }
                if (targetX > ptbee[bee].x) {
                    targetSpeedX = queenMaxSp / (multY / multX);
                } else {
                    targetSpeedX = -(queenMaxSp / (multY / multX));
                }
            }
            if (ptbee[bee].dx > targetSpeedX)
                ptbee[bee].dx -= queenReact;
            if (ptbee[bee].dx < targetSpeedX)
                ptbee[bee].dx += queenReact;
            if (ptbee[bee].dy > targetSpeedY)
                ptbee[bee].dy -= queenReact;
            if (ptbee[bee].dy < targetSpeedY)
                ptbee[bee].dy += queenReact;

            ptbee[bee].x += ptbee[bee].dx;
            ptbee[bee].y += ptbee[bee].dy;
            if (ptbee[bee].x < 1) {
                ptbee[bee].x = 1;
                ptbee[bee].dx = -ptbee[bee].dx;
            }
            if (ptbee[bee].y < 1) {
                ptbee[bee].y = 1;
                ptbee[bee].dy = -ptbee[bee].dy;
            }
            if (ptbee[bee].x > 16384) {
                ptbee[bee].x = 16384;
                ptbee[bee].dx = -ptbee[bee].dx;
            }
            if (ptbee[bee].y > 16384) {
                ptbee[bee].y = 16384;
                ptbee[bee].dy = -ptbee[bee].dy;
            }

            if (ptbee[bee].x > (targetX - 256) && ptbee[bee].x < (targetX + 256) && ptbee[bee].y > (targetY - 256) &&
                ptbee[bee].y < (targetY + 256)) {
                targetX = RandNum(1, 16384);
                targetY = RandNum(1, 16384);
            }
            if (!firstTime) {
                beeSize = sizer / 2;
                if (sizer > 0 && beeSize < 1)
                    beeSize = 1;
                if (!hideQueen) {
                    PenSize(sizer, sizer);
                    MoveTo((ox / xs) - beeSize, (oy / ys) - beeSize);
                    LineTo((ptbee[bee].x / xs) - beeSize, (ptbee[bee].y / ys) - beeSize);
                }
                PenSize(beeSize, beeSize);
                beeSize /= 2;
            }
        } else {   // a chaser
            multX = Absolute(ptbee[0].x - ptbee[bee].x);
            multY = Absolute(ptbee[0].y - ptbee[bee].y);
            if (multX > multY) {
                if (ptbee[0].x > ptbee[bee].x) {
                    targetSpeedX = droneMaxSp;
                } else {
                    targetSpeedX = -droneMaxSp;
                }
                if (ptbee[0].y > ptbee[bee].y) {
                    targetSpeedY = droneMaxSp / (multX / multY);
                } else {
                    targetSpeedY = -(droneMaxSp / (multX / multY));
                }
            } else {
                if (ptbee[0].y > ptbee[bee].y) {
                    targetSpeedY = droneMaxSp;
                } else {
                    targetSpeedY = -droneMaxSp;
                }
                if (ptbee[0].x > ptbee[bee].x) {
                    targetSpeedX = droneMaxSp / (multY / multX);
                } else {
                    targetSpeedX = -(droneMaxSp / (multY / multX));
                }
            }
            ptbee[bee].dy += RandVariance(20 - curSet.actSizeVar[ACTSWARM] / 5);
            ptbee[bee].dx += RandVariance(20 - curSet.actSizeVar[ACTSWARM] / 5);
            if (ptbee[bee].dx > targetSpeedX)
                ptbee[bee].dx -= droneReact;
            if (ptbee[bee].dx < targetSpeedX)
                ptbee[bee].dx += droneReact;
            if (ptbee[bee].dy > targetSpeedY)
                ptbee[bee].dy -= droneReact;
            if (ptbee[bee].dy < targetSpeedY)
                ptbee[bee].dy += droneReact;

            ptbee[bee].x += ptbee[bee].dx;
            ptbee[bee].y += ptbee[bee].dy;
            if (ptbee[bee].x < 1) {
                ptbee[bee].x = 1;
                ptbee[bee].dx = -ptbee[bee].dx;
            }
            if (ptbee[bee].y < 1) {
                ptbee[bee].y = 1;
                ptbee[bee].dy = -ptbee[bee].dy;
            }
            if (ptbee[bee].x > 16384) {
                ptbee[bee].x = 16384;
                ptbee[bee].dx = -ptbee[bee].dx;
            }
            if (ptbee[bee].y > 16384) {
                ptbee[bee].y = 16384;
                ptbee[bee].dy = -ptbee[bee].dy;
            }

            if (!firstTime) {
                MoveTo((ox / xs) - beeSize, (oy / ys) - beeSize);
                LineTo((ptbee[bee].x / xs) - beeSize, (ptbee[bee].y / ys) - beeSize);
            }
            if (!hideQueen) {
                if (ptbee[bee].x > (ptbee[0].x - 128) && ptbee[bee].x < (ptbee[0].x + 128) && ptbee[bee].y > (ptbee[0].y - 128) &&
                    ptbee[bee].y < (ptbee[0].y + 128)) {
                    ptbee[bee].x = 0;
                }
            }
        }
    }
    UnlockAllWorlds();
    RestoreContext();
}

void DoDrops(void) {
    static long lastTick = 0;
    short num, d, size;
    Rect splatRect;
    float divisor;
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

    if (lastTick == 0)
        lastTick = TickCount();
    divisor = (float)60 / (float)(TickCount() - lastTick);
    num = (float)curSet.actCount[ACTDROPS] / divisor;
    if (num > 100)
        num = 100;
    if (num) {
        lastTick = TickCount();
        for (d = 0; d < num; d++) {
            if (curSet.actReactSnd[ACTDROPS]) {
                size = RecentVolume();
                if (!prefs.lowQualityMode && !prefs.highQualityMode)
                    size /= 2;
                if (prefs.lowQualityMode)
                    size /= 4;
            } else {
                size = PixelSize(curSet.actSiz[ACTDROPS] + RandVariance(curSet.actSizeVar[ACTDROPS] / 2));
                if (size < 2)
                    size = 2;
            }
            if (size > 0) {
                if (curSet.actMisc1[ACTDROPS]) {   // constrain onscreen
                    splatRect.left = RandNum(2, useWidth - size - 1);
                    splatRect.top = RandNum(2, useHeight - size - 1);
                } else {
                    splatRect.left = RandNum(-size, useWidth + size);
                    splatRect.top = RandNum(-size, useHeight + size);
                }
                splatRect.right = splatRect.left + size;
                splatRect.bottom = splatRect.top + size;
                PaintOval(&splatRect);
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
}

/*
for example, user has curSet.actCount[ACTDROPS] set to 50, meaning 50 per second.
10 ticks have elapsed since the last DoDrops.  divide numDrops by (60/elapsed) means draw 8.
*/
short RandNum(short low, short high) {
    long range;

    range = (high - low);
    return ((Random() & 0x7FFF) % range) + low;
}

short RandVariance(short plusminus) {
    if (plusminus < 1)
        return 0;
    return RandNum(0, (plusminus * 2) + 1) - plusminus;
}

short PixelSize(short inSize) {
    if (prefs.lowQualityMode && inSize > 1)
        inSize /= 2;
    if (prefs.highQualityMode)
        inSize *= 2;
    return inSize;
}

void SetForeAndBackColors(void) {
    RGBColor maxColor;

    SaveContext();
    if (LockAllWorlds()) {
        SetGWorld(offWorld, nil);
        Index2Color((long)0, &maxColor);
        RGBBackColor(&maxColor);
        Index2Color((long)255, &maxColor);
        RGBForeColor(&maxColor);
        UnlockAllWorlds();
    }
    RestoreContext();
}

void DoubleBlit(void) {
    if (LockAllWorlds()) {
#if USE_GCD
        dispatch_apply(prefs.winysize, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
            Ptr fromCopy = fromBasePtr + (fromByteCount * current);
            Ptr toCopy = doubleBasePtr + (doubleByteCount * (current * 2));
            Ptr toSecondLineCopy = toCopy + doubleByteCount;
            int xpos2 = 0;
            int xpos;
            for (xpos = 0; xpos < prefs.winxsize; xpos++) {
                unsigned char val = fromCopy[xpos];
                toCopy[xpos2] = val;
                toSecondLineCopy[xpos2++] = val;
                toCopy[xpos2] = val;
                toSecondLineCopy[xpos2++] = val;
            }
        });

#else
        Ptr toSecondLineCopy;
        unsigned char val;
        int ypos = prefs.winysize;
        Ptr fromCopy = fromBasePtr;
        Ptr toCopy = doubleBasePtr;

        while (ypos > 0) {
            int xpos2 = 0;
            toSecondLineCopy = toCopy + doubleByteCount;
            int xpos;
            for (xpos = 0; xpos < prefs.winxsize; xpos++) {
                val = fromCopy[xpos];
                toCopy[xpos2] = val;
                toSecondLineCopy[xpos2++] = val;
                toCopy[xpos2] = val;
                toSecondLineCopy[xpos2++] = val;
            }
            fromCopy += fromByteCount;
            toCopy += (doubleByteCount * 2);
            ypos--;
        }
#endif
    }
    UnlockAllWorlds();
}

void DoubleEmbossBlit(void) {
    Ptr toCopy, fromCopy, toSecondLineCopy;
    short ypos, xpos, xpos2;
    unsigned char val;

    fromCopy = blurBasePtr;
    toCopy = doubleBasePtr;

    ypos = prefs.winysize;

    while (ypos > 0) {
        xpos2 = 0;
        toSecondLineCopy = toCopy + doubleByteCount;
        for (xpos = 0; xpos < prefs.winxsize; xpos++) {
            val = fromCopy[xpos];
            toCopy[xpos2] = val;
            toCopy[xpos2 + 1] = val;
            toSecondLineCopy[xpos2] = val;
            toSecondLineCopy[xpos2 + 1] = val;
            xpos2 += 2;
        }
        fromCopy += blurByteCount;
        toCopy += doubleByteCount * 2;
        ypos--;
    }
}

void HighQualityBlit(void) {
    Rect rect1, rect2, rect3;

    GetPortBounds(offWorld, &rect1);
    GetPortBounds(highQualityWorld, &rect2);
    GetPortBounds(stage32, &rect3);

    SaveContext();
    if (LockAllWorlds()) {
        SetGWorld(highQualityWorld, nil);
        ForeColor(blackColor);
        BackColor(whiteColor);
        CopyBits(GetPortBitMapForCopyBits(offWorld), GetPortBitMapForCopyBits(highQualityWorld), &rect1, &rect2, srcCopy, nil);
        SetGWorld(stage32, nil);
        CopyBits(GetPortBitMapForCopyBits(highQualityWorld), GetPortBitMapForCopyBits(stage32), &rect2, &rect3, srcCopy, nil);
        UnlockAllWorlds();
    }
    RestoreContext();
}

// squish large 8-bit offWorld into normal 32-bit stage32
// currentColor[256] ... fromBasePtr, fromByteCount
void HighQualityBlitOptimized(void) {
    SaveContext();
    if (LockAllWorlds()) {
        unsigned char red[256], green[256], blue[256];
        short i, x, y;
        Rect fromRect, stageRect;
        unsigned char r1, r2, r3, r4;
        unsigned char g1, g2, g3, g4;
        unsigned char b1, b2, b3, b4;
        Ptr stageBasePtr = GetPixBaseAddr(stage32PixMap);
        long stageByteCount = (0x7FFF & (**stage32PixMap).rowBytes);
        GetPortBounds(offWorld, &fromRect);
        GetPortBounds(stage32, &stageRect);
        // build unsigned char color table
        for (i = 0; i < 256; i++) {
            red[i] = (unsigned char)(currentColor[i].red >> 8);
            green[i] = (unsigned char)(currentColor[i].green >> 8);
            blue[i] = (unsigned char)(currentColor[i].blue >> 8);
        }
        for (y = 0; y < (stageRect.bottom - stageRect.top); y++) {
            Ptr fromPtr = fromBasePtr + (fromByteCount * (y * 2));
            Ptr stagePtr = stageBasePtr + (stageByteCount * y);
            for (x = 0; x < (stageRect.right - stageRect.left); x++) {
                // gather source color values
                r1 = red[(unsigned char)fromPtr[0]];
                r2 = red[(unsigned char)fromPtr[1]];
                r3 = red[(unsigned char)fromPtr[fromByteCount]];
                r4 = red[(unsigned char)fromPtr[fromByteCount + 1]];

                g1 = green[(unsigned char)fromPtr[0]];
                g2 = green[(unsigned char)fromPtr[1]];
                g3 = green[(unsigned char)fromPtr[fromByteCount]];
                g4 = green[(unsigned char)fromPtr[fromByteCount + 1]];

                b1 = blue[(unsigned char)fromPtr[0]];
                b2 = blue[(unsigned char)fromPtr[1]];
                b3 = blue[(unsigned char)fromPtr[fromByteCount]];
                b4 = blue[(unsigned char)fromPtr[fromByteCount + 1]];

                fromPtr += 2;
                // build composite mixed 32-bit value
                if (r1 == r2 && r2 == r3 && r3 == r4)
                    stagePtr[1] = r1;
                else
                    stagePtr[1] = (unsigned char)((r1 + r2 + r3 + r4) >> 2);
                if (g1 == g2 && g2 == g3 && g3 == g4)
                    stagePtr[2] = g1;
                else
                    stagePtr[2] = (unsigned char)((g1 + g2 + g3 + g4) >> 2);
                if (b1 == b2 && b2 == b3 && b3 == b4)
                    stagePtr[3] = b1;
                else
                    stagePtr[3] = (unsigned char)((b1 + b2 + b3 + b4) >> 2);
                stagePtr += 4;
            }
        }
        UnlockAllWorlds();
    }
    RestoreContext();
}

void HighQualityEmbossBlit(void) {
    Rect rect1, rect2, rect3;

    GetPortBounds(blurWorld, &rect1);
    GetPortBounds(highQualityWorld, &rect2);
    GetPortBounds(stage32, &rect3);

    SaveContext();
    if (LockAllWorlds()) {
        SetGWorld(highQualityWorld, nil);
        ForeColor(blackColor);
        BackColor(whiteColor);
        CopyBits(GetPortBitMapForCopyBits(blurWorld), GetPortBitMapForCopyBits(highQualityWorld), &rect1, &rect2, srcCopy, nil);
        SetGWorld(stage32, nil);
        CopyBits(GetPortBitMapForCopyBits(highQualityWorld), GetPortBitMapForCopyBits(stage32), &rect2, &rect3, srcCopy, nil);
        UnlockAllWorlds();
    }
    RestoreContext();
}

void PostEmboss(void) {
    short value;
    long offset1, offset2;
    short y, x;

    if (LockAllWorlds()) {
        offset1 = offset2 = 0;
        for (y = 0; y < prefs.winysize; y++) {
            for (x = 1; x < prefs.winxsize; x++) {
                value = ((unsigned char)fromBasePtr[offset1 + x] - (unsigned char)fromBasePtr[offset1 + x - 1]) * 2;
                if (value < -128) {
                    value = -128;
                } else {
                    if (value > 127)
                        value = 127;
                }
                blurBasePtr[offset2 + x] = value + 128;
            }
            blurBasePtr[offset2] = (unsigned char)blurBasePtr[offset2 + 1];
            offset1 += fromByteCount;
            offset2 += blurByteCount;
        }
    }
    UnlockAllWorlds();
}

void UpdateToMain(Boolean toStage) {
    CGrafPtr thisWorld, toWorld;
    static long frameID = 0;

    frameID++;
    AddFPSToWindowTitle(frameID);
    if (!LockAllWorlds())
        return;
    SaveContext();
    if (toStage) {
        thisWorld = stage32;
    } else {
        thisWorld = GetWindowPort(gMainWindow); /*thisWorld = mainWorld;*/
    }
    if (prefs.lowQualityMode) {
        if (curSet.emboss) {
            PostEmboss();
            DoubleEmbossBlit();
        } else {
            DoubleBlit();
        }
        SetGWorld(thisWorld, nil);
        CopyBits(GetPortBitMapForCopyBits(doubleWorld), GetPortBitMapForCopyBits(thisWorld), &gCopyFromRect, &gCopyToRect,
                 gCopyMode, nil);
    }

    if (prefs.highQualityMode) {
        if (curSet.emboss) {
            PostEmboss();
            HighQualityEmbossBlit();
        } else {
            HighQualityBlitOptimized();
        }
        if (!toStage) {
            SetGWorld(thisWorld, nil);
            CopyBits(GetPortBitMapForCopyBits(stage32), GetPortBitMapForCopyBits(thisWorld), &gCopyToRect, &gCopyToRect, gCopyMode,
                     nil);
        }
    }

    if (!prefs.lowQualityMode && !prefs.highQualityMode) {
        if (curSet.emboss) {
            PostEmboss();
            SetGWorld(thisWorld, nil);
            CopyBits(GetPortBitMapForCopyBits(blurWorld), GetPortBitMapForCopyBits(thisWorld), &gCopyFromRect, &gCopyToRect,
                     gCopyMode, nil);
        } else {
            if (toStage) {
                SetGWorld(stage32, nil);
                CopyBits(GetPortBitMapForCopyBits(offWorld), GetPortBitMapForCopyBits(stage32), &gCopyFromRect, &gCopyToRect,
                         gCopyMode, nil);
            } else {
                SetPortWindowPort(gMainWindow);
                toWorld = GetWindowPort(gMainWindow);
                SetGWorld(toWorld, nil);
                CopyBits(GetPortBitMapForCopyBits(offWorld), GetPortBitMapForCopyBits(toWorld), &gCopyFromRect, &gCopyToRect,
                         gCopyMode, nil);
            }
        }
    }
    RestoreContext();
    UnlockAllWorlds();
    if (gShowNeedsSoundInput)
        ShowNeedsSoundInputMessage();
}

void ShowNeedsSoundInputMessage(void) {
    static short genevaFnum = -1;
    Rect theRect;
    Str255 message;

    SaveContext();
    SetPortWindowPort(gMainWindow);
    if (genevaFnum < 1) {
        GetFNum("\pGeneva", &genevaFnum);
        if (genevaFnum < 1)
            genevaFnum = 0;
    }
    GetWindowPortBounds(gMainWindow, &theRect);
    theRect.top = theRect.bottom - 13;
    BackColor(blueColor);
    ForeColor(whiteColor);
    TextMode(srcOr);
    TextSize(9);
    TextFont(genevaFnum);
    EraseRect(&theRect);
    GetIndString(message, BASERES, 55);
    MoveTo(theRect.right / 2 - StringWidth(message) / 2, theRect.bottom - 3);
    DrawString(message);
    ForeColor(blackColor);
    BackColor(whiteColor);
    RestoreContext();
}

void UpdateToPNG(void) {
    CGrafPtr thisFileWorld;
    CursHandle watch;
    FSSpec fss;

    StopAnimationTimer();
    ResetCursor();
    OSErr err =
        CocoaSaveFile(CFSTR("SavePictureTitle"), CFSTR("SavePictureMessage"), CFSTR("png"), (OSType)'ogle', (OSType)'PNG ', &fss);
    if (noErr == err) {
        watch = GetCursor(watchCursor);
        SetCursor(*watch);
        // Set up thisFileWorld
        if (prefs.highQualityMode) {
            if (curSet.emboss) {
                PostEmboss();
                HighQualityEmbossBlit();
            } else {
                HighQualityBlitOptimized();
            }
            thisFileWorld = stage32;
        }
        if (prefs.lowQualityMode) {
            if (curSet.emboss) {
                PostEmboss();
                DoubleEmbossBlit();
            } else {
                DoubleBlit();
            }
            thisFileWorld = doubleWorld;
        }
        if (!prefs.lowQualityMode && !prefs.highQualityMode) {
            if (curSet.emboss) {
                PostEmboss();
                thisFileWorld = blurWorld;
            } else {
                thisFileWorld = offWorld;
            }
        }

        // Save the image from thisFileWorld
        {
            CGrafPtr outputWorld;
            Rect outputRect;
            GetPortBounds(thisFileWorld, &outputRect);
            err = NewGWorld(&outputWorld, 32, &outputRect, nil, nil, 0);
            if (err == noErr) {
                ComponentInstance ge = nil;
                SaveContext();
                LockAllWorlds();
                PTLockPixels(GetGWorldPixMap(outputWorld));
                SetGWorld(outputWorld, nil);
                CopyBits(GetPortBitMapForCopyBits(thisFileWorld), GetPortBitMapForCopyBits(outputWorld), &outputRect, &outputRect,
                         srcCopy, nil);
                UnlockAllWorlds();
                RestoreContext();
                err = HDelete(fss.vRefNum, fss.parID, fss.name);
                if (!IsMacOSX()) {
                    err = HCreate(fss.vRefNum, fss.parID, fss.name, (OSType)'ogle', (OSType)'PNG ');
                    //if (err) StopError(14, err);
                }

                err = OpenADefaultComponent(GraphicsExporterComponentType, kQTFileTypePNG, &ge);
                if (err || ge == nil)
                    NoteError(15, err);

                if (!err) {
                    err = GraphicsExportSetInputGWorld(ge, outputWorld);
                    if (err)
                        NoteError(16, err);
                }

                if (!err) {
                    err = GraphicsExportSetOutputFile(ge, &fss);
                    if (err)
                        NoteError(17, err);
                }

                if (!err) {
                    err = GraphicsExportDoExport(ge, nil);
                    if (err)
                        NoteError(18, err);
                }

                if (ge)
                    CloseComponent(ge);

                if (err)
                    HDelete(fss.vRefNum, fss.parID, fss.name);

                DisposeGWorld(outputWorld);
            }
        }
        // Create custom icon
        ResetCursor();
        gPICTSaveNum++;
    }
    UpdateAnimationTimer();
}

void ClearFromAndBase(void) {
    long offset1, offset2;
    short x, y;

    if (LockAllWorlds()) {
        offset1 = offset2 = 0;
        for (y = 0; y < prefs.winysize; y++) {
            for (x = 0; x < prefs.winxsize; x++) {
                fromBasePtr[offset1 + x] = 0;
                blurBasePtr[offset2 + x] = 0;
            }
            offset1 += fromByteCount;
            offset2 += blurByteCount;
        }
    }
    UnlockAllWorlds();
}

void StartQTExport(void) {
    Boolean result = false;
    FSSpec fss;
    OSErr err;
    long maxCompressedSize;
    ComponentResult cr;
    SCTemporalSettings ts;
    Str255 str;

    if (gQTExporting) {
        StopQTExport();
        return;
    }
    StopAnimationTimer();
    if (!QuickTimeOptionsDialog()) {
        UpdateAnimationTimer();
        return;
    }
    ResetCursor();
    // saved filename was curSet.windowTitle + " Movie"
    err = CocoaSaveFile(CFSTR("SaveMovieTitle"), CFSTR("SaveMovieMessage"), CFSTR("mov"), (OSType)'TVOD', (OSType)'MooV', &fss);
    if (noErr == err)
        result = true;

    if (result) {
        realTimeLastTick = 0;
        movieResRefNum = 0;
        movieResID = 0;
        movieVRefNum = fss.vRefNum;
        err = CreateMovieFile(&fss, 'TVOD', smCurrentScript, createMovieFileDeleteCurFile, &movieResRefNum, &theMovie);
        if (err)
            StopError(20, err);
        theVideoTrack = NewMovieTrack(theMovie, FixRatio(gCopyToRect.right, 1), FixRatio(gCopyToRect.bottom, 1), kNoVolume);
        err = GetMoviesError();
        if (err)
            StopError(21, err);
        theVideoMedia = NewTrackMedia(theVideoTrack, VideoMediaType, 600, nil, 0);    // 600 = video time scale!?
        err = GetMoviesError();
        if (err)
            StopError(22, err);
        err = BeginMediaEdits(theVideoMedia);
        if (err)
            StopError(23, err);
        if (prefs.QTIncludeAudio) {
            theSoundTrack = NewMovieTrack(theMovie, 0, 0, kFullVolume);
            err = GetMoviesError();
            if (err)
                StopError(21, err);
            theSoundMedia = NewTrackMedia(theSoundTrack, SoundMediaType, 600, nil, 0);
            err = GetMoviesError();
            if (err)
                StopError(22, err);
            err = BeginMediaEdits(theSoundMedia);
            if (err)
                StopError(23, err);
            soundDesc = (SoundDescriptionHandle)NewHandle(sizeof(SoundDescription));
            if (!soundDesc)
                StopError(23, memFullErr);
            (*soundDesc)->descSize = sizeof(SoundDescription);
            (*soundDesc)->dataFormat = (OSType)'raw ';
            (*soundDesc)->numChannels = 1;
            (*soundDesc)->sampleSize = 8;
            (*soundDesc)->sampleRate = 0x56EE8BA3;    // 22254.5454 = 0x56EE8BA3
            // plus a *bunch* of reserved crap!
            (*soundDesc)->resvd1 = 0;
            (*soundDesc)->resvd2 = 0;
            (*soundDesc)->dataRefIndex = 1;
            (*soundDesc)->version = 0;
            (*soundDesc)->revlevel = 0;
            (*soundDesc)->vendor = 0;
            (*soundDesc)->compressionID = 0;
            (*soundDesc)->packetSize = 0;
        }
        UpdateToMain(true);
        ci = OpenDefaultComponent(StandardCompressionType, StandardCompressionSubType);
        cr = SCDefaultPixMapSettings(ci, stage32PixMap, true);
        cr = SCSetTestImagePixMap(ci, stage32PixMap, nil, scPreferCropping);
        cr = SCRequestSequenceSettings(ci);
        if (cr != scUserCancelled && cr >= 0) {
            cr = SCGetInfo(ci, scTemporalSettingsType, &ts);
            gExportFPSSpecified = ts.frameRate >> 16;
            durationPerSample = 600 / gExportFPSSpecified;
            err = GetMaxCompressionSize(stage32PixMap, &gCopyToRect, 0, codecNormalQuality, 'rle ', (CompressorComponent)anyCodec,
                                        &maxCompressedSize);
            if (err)
                StopError(24, err);
            compressedMovieData = NewHandle(maxCompressedSize);
            err = MemError();
            if (err)
                StopError(25, err);
            MoveHHi(compressedMovieData);
            HLock(compressedMovieData);
            imageDesc = (ImageDescriptionHandle)NewHandle(4);
            err = MemError();
            if (err)
                StopError(26, err);
            if (!err) {
                gQTExporting = true;
                GetIndString(str, BASERES, 10);
                if (gFileMenu)
                    SetMenuItemText(gFileMenu, EXPORTQTID, str);
                DrawMenuBar();
                cr = SCCompressSequenceBegin(ci, stage32PixMap, nil, &imageDesc);
                gExportBeginTicks = TickCount();
                gExportFramesExported = 0;
                UpdateExportTimeWindowTitle();
            }
        } else {   // cancelled, close up
            err = EndMediaEdits(theVideoMedia);
            if (prefs.QTIncludeAudio)
                err = EndMediaEdits(theSoundMedia);
            err = CloseMovieFile(movieResRefNum);
            err = CloseComponent(ci);
            err = HDelete(fss.vRefNum, fss.parID, fss.name);
        }
    }
    UpdateAnimationTimer();
}

void FrameQTExport(void) {
    HParamBlockRec hpb;
    OSErr err;
    TimeValue thisDuration;
    long dataSize;
    short syncFlag;

    if (gQTExporting) {
        gExportFramesExported++;
        UpdateExportTimeWindowTitle();
        UpdateToMain(true);
        err = SCCompressSequenceFrame(ci, stage32PixMap, &gCopyToRect, &compressedMovieData, &dataSize, &syncFlag);
        if (prefs.QTMode == 1) {   // Fixed frame rate
            err = AddMediaSample(theVideoMedia, compressedMovieData, 0, dataSize, durationPerSample,
                                 (SampleDescriptionHandle)imageDesc, 1, syncFlag, nil);
            if (err)
                StopError(28, err);
        } else {   // real time
            if (realTimeLastTick == 0) {
                thisDuration = 1;
            } else {
                thisDuration = (TickCount() - realTimeLastTick) * 10;
            }
            realTimeLastTick = TickCount();
            err = AddMediaSample(theVideoMedia, compressedMovieData, 0, dataSize, thisDuration, (SampleDescriptionHandle)imageDesc,
                                 1, syncFlag, nil);
            if (err)
                StopError(28, err);
        }
        hpb.volumeParam.ioNamePtr = nil;
        hpb.volumeParam.ioVRefNum = movieVRefNum;
        hpb.volumeParam.ioVolIndex = 0;
        err = PBHGetVInfo(&hpb, false);
        if (err == noErr) {
            if (hpb.volumeParam.ioVFrBlk < 32) {
                DoStandardAlert(kAlertCautionAlert, 13);
                StopQTExport();
            }
        }
    }
}

void StopQTExport(void) {
    TimeValue theDuration;
    Str255 str;
    OSErr err;
    Size siz;

    if (gQTExporting) {
        err = SCCompressSequenceEnd(ci);
        gQTExporting = false;
        err = EndMediaEdits(theVideoMedia);
        if (err)
            StopError(29, err);
        theDuration = GetMediaDuration(theVideoMedia);
        err = InsertMediaIntoTrack(theVideoTrack, 0, 0, theDuration, fixed1);
        if (err)
            StopError(30, err);
        err = AddMovieResource(theMovie, movieResRefNum, &movieResID, nil);
        if (err)
            StopError(31, err);
        if (prefs.QTIncludeAudio) {
            err = EndMediaEdits(theSoundMedia);
            if (err)
                StopError(29, err);
            theDuration = GetMediaDuration(theSoundMedia);
            err = InsertMediaIntoTrack(theSoundTrack, 0, 0, theDuration, fixed1);
            if (err)
                StopError(30, err);
            err = AddMovieResource(theMovie, movieResRefNum, &movieResID, nil);
            if (err)
                StopError(31, err);
        }
        err = CloseMovieFile(movieResRefNum);
        if (err)
            StopError(32, err);
        err = CloseComponent(ci);
        if (imageDesc)
            DisposeHandle((Handle)imageDesc);
        imageDesc = nil;
        if (soundDesc)
            DisposeHandle((Handle)soundDesc);
        soundDesc = nil;
        HUnlock(compressedMovieData);
        if (compressedMovieData)
            DisposeHandle(compressedMovieData);
        GetIndString(str, BASERES, 9);
        if (gFileMenu)
            SetMenuItemText(gFileMenu, EXPORTQTID, str);
        DrawMenuBar();
        MySetWindowTitle();
        MaxMem(&siz);
    }
}

void ShowSettings(void) {
    short c, numEffects;

    if (gFileMenu)
        CheckMenuItem(gFileMenu, PAUSEID, gPause);
    if (gSetsMenu)
        CheckMenuItem(gSetsMenu, TIMESETID, prefs.autoSetChange);
    if (gOptionsMenu) {
        CheckMenuItem(gOptionsMenu, LOWQUALITYID, prefs.lowQualityMode);
        CheckMenuItem(gOptionsMenu, HIGHQUALITYID, prefs.highQualityMode);
        CheckMenuItem(gOptionsMenu, FULLSCREENID, prefs.fullScreen);
        CheckMenuItem(gOptionsMenu, DITHERID, prefs.dither);
        CheckMenuItem(gOptionsMenu, AUTOPILOTID, prefs.autoPilotActive);
        CheckMenuItem(gOptionsMenu, AFTEREFFECTID, curSet.postFilter);
        CheckMenuItem(gOptionsMenu, EMBOSSID, curSet.emboss);
    }
    if (gActionsMenu) {
        CheckMenuItem(gActionsMenu, BOUNCINGID, curSet.action[ACTLINES]);
        CheckMenuItem(gActionsMenu, RANDOMWALKID, curSet.action[ACTBALLS]);
        CheckMenuItem(gActionsMenu, SWARMID, curSet.action[ACTSWARM]);
        CheckMenuItem(gActionsMenu, RAINID, curSet.action[ACTDROPS]);
        CheckMenuItem(gActionsMenu, SOUNDWAVEID, curSet.action[ACTSWAVE]);
        CheckMenuItem(gActionsMenu, DOODLEID, curSet.action[ACTDOODLE]);
        CheckMenuItem(gActionsMenu, TEXTID, curSet.action[ACTTEXT]);
        CheckMenuItem(gActionsMenu, PARTICLEID, curSet.action[ACTPARTICLES]);
        CheckMenuItem(gActionsMenu, IMAGEID, curSet.action[ACTIMAGES]);
    }
    if (gColorsMenu) {
        CheckMenuItem(gColorsMenu, NOAUDIOCOLOR, (curSet.paletteSoundMode == PSMODE_NONE));
        CheckMenuItem(gColorsMenu, AUDIOBLACK, (curSet.paletteSoundMode == PSMODE_BLACK));
        CheckMenuItem(gColorsMenu, AUDIOWHITE, (curSet.paletteSoundMode == PSMODE_WHITE));
        CheckMenuItem(gColorsMenu, AUDIOINVERT, (curSet.paletteSoundMode == PSMODE_INVERT));
        CheckMenuItem(gColorsMenu, AUDIOROTATE, (curSet.paletteSoundMode == PSMODE_ROTATE));
    }
    if (gFiltersMenu) {
        numEffects = CountMenuItems(gFiltersMenu) - 2;    // don't count "None" or divider
        for (c = 0; c < numEffects; c++) {
            CheckMenuItem(gFiltersMenu, c + FIRSTFILTER, FilterActive(c));
        }
        CheckMenuItem(gFiltersMenu, HORIZMIRROR, curSet.horizontalMirror);
        CheckMenuItem(gFiltersMenu, VERTMIRROR, curSet.verticalMirror);
        CheckMenuItem(gFiltersMenu, CONSTRAINMIRROR, curSet.constrainMirror);
    }
    if (gWindowMenu)
        CheckMenuItem(gWindowMenu, MOVESIZEGROUP, prefs.dragSizeAllWindows);
    if (prefs.windowOpen[WINCOLORS])
        PTInvalidateWindow(WINCOLORS);
    if (prefs.windowOpen[WINACTIONS])
        PTInvalidateWindow(WINACTIONS);
    if (prefs.windowOpen[WINOPTIONS])
        PTInvalidateWindow(WINOPTIONS);
    UpdateCurrentSetNeedsSoundInputMessage();
}

void AddFPSToWindowTitle(long frameID) {   // Use main window title to show FPS
    static Str32 FPSStr = "\p";
    Str32 suffix = "\p fps";
    Str255 str;
    static long lastFrameNum = 0, lastUpdate = 0, nextUpdate = 0;
    short FPS;
    float secAdj;

    if (!prefs.showFPS || gQTExporting)
        return;
    if (TickCount() >= nextUpdate) {   // update FPSStr
        // calculate latest FPS
        secAdj = 600.0 / (float)(TickCount() - lastUpdate);
        FPS = (frameID - lastFrameNum) * secAdj;
        NumToString(FPS, FPSStr);
        FPSStr[FPSStr[0] + 1] = FPSStr[FPSStr[0]];
        FPSStr[FPSStr[0]] = '.';
        FPSStr[0]++;
        BlockMoveData(suffix + 1, FPSStr + FPSStr[0] + 1, suffix[0]);
        FPSStr[0] += suffix[0];
        lastUpdate = TickCount();
        nextUpdate = lastUpdate + 120;
        lastFrameNum = frameID;
        BlockMoveData(curSet.windowTitle, str, curSet.windowTitle[0] + 1);
        str[++str[0]] = ' ';
        str[++str[0]] = '@';
        str[++str[0]] = ' ';
        BlockMoveData(FPSStr + 1, str + str[0] + 1, FPSStr[0]);
        str[0] += FPSStr[0];
        SetWTitle(gMainWindow, str);
    }
}

void UpdateExportTimeWindowTitle(void) {
    static long nextUpdate = 0;
    long now, secsExported;

    if (!gQTExporting)
        return;
    now = TickCount();
    if (now >= nextUpdate) {
        Str63 numStr;
        Str255 title = "\p[";

        if (prefs.QTMode == 1)
            secsExported = (gExportFramesExported / gExportFPSSpecified);
        else
            secsExported = (now - gExportBeginTicks) / 60;

        NumToString(secsExported / 60, numStr);
        BlockMoveData(numStr + 1, title + title[0] + 1, numStr[0] + 1);
        title[0] += numStr[0];
        title[++title[0]] = ':';

        NumToString(secsExported % 60, numStr);
        if ((secsExported % 60) < 10)
            title[++title[0]] = '0';
        BlockMoveData(numStr + 1, title + title[0] + 1, numStr[0] + 1);
        title[0] += numStr[0];
        title[++title[0]] = ']';
        title[++title[0]] = ' ';
        GetIndString(numStr, BASERES, 6);
        BlockMoveData(numStr + 1, title + title[0] + 1, numStr[0] + 1);
        title[0] += numStr[0];
        SetWTitle(gMainWindow, title);

        nextUpdate = now + 60;
    }
}

short Absolute(short num) {
    if (num >= 0)
        return num;
    return -num;
}

void StopError(short desc, OSErr type) {
    Str255 typeStr, descStr = "\pdo something";

    StopAnimationTimer();
    SetGWorld(GetWindowPort(gMainWindow), nil);
    SetPortWindowPort(gMainWindow);
    ForeColor(blackColor);
    BackColor(whiteColor);
    ErrorToString(type, typeStr);
    if (desc)
        GetIndString(descStr, BASERES + 2, desc);
    ParamText(typeStr, descStr, nil, nil);
    if (type == memFullErr) {   // Error is memory low condition
        DoStandardAlert(kAlertStopAlert, 2);
    } else {   // or something else
        StopAlert(BASERES + 1, nil);
    }
    ExitToShell();
}

void NoteError(short desc, OSErr type) {
    Str255 typeStr, descStr = "\pdo something";

    StopAnimationTimer();
    SaveContext();
    SetPortWindowPort(gMainWindow);
    ForeColor(blackColor);
    BackColor(whiteColor);
    ErrorToString(type, typeStr);
    if (desc)
        GetIndString(descStr, BASERES + 2, desc);
    ParamText(typeStr, descStr, nil, nil);
    if (type == memFullErr) {   // Error is memory low condition
        DoStandardAlert(kAlertNoteAlert, 2);
    } else {   // or something else
        NoteAlert(BASERES + 1, nil);
    }
    RestoreContext();
    UpdateAnimationTimer();
}

void ErrorToString(OSErr err, Str255 str) {
    short i = 0;
    Str32 numStr, errStr = "\pError #";

    if (err == nsvErr)
        i = 1;
    if (err == ioErr)
        i = 2;
    if (err == bdNamErr)
        i = 3;
    if (err == fnfErr)
        i = 4;
    if (err == volOffLinErr)
        i = 5;
    if (err == memFullErr)
        i = 6;
    if (err == dupFNErr)
        i = 7;
    if (err == codecBadDataErr)
        i = 8;
    if (err == couldNotResolveDataRef)
        i = 9;
    if (err == fBsyErr)
        i = 10;
    if (err == paramErr)
        i = 11;
    if (err == dskFulErr)
        i = 12;
    if (err == noHardwareErr)
        i = 13;
    if (err == notEnoughHardwareErr)
        i = 14;
    if (err == cantFindHandler)
        i = 15;
    if (err == siDeviceBusyErr)
        i = 16;
    if (err == siBadSoundInDevice)
        i = 17;
    if (err == siBadRefNum)
        i = 18;
    if (err == afpAccessDenied)
        i = 19;
    if (err == dirNFErr)
        i = 20;
    if (i > 0) {
        GetIndString(str, BASERES + 7, i);
    } else {
        BlockMoveData(errStr, str, errStr[0] + 1);
        NumToString(err, numStr);
        BlockMoveData(numStr + 1, str + str[0] + 1, numStr[0]);
        str[0] += numStr[0];
    }
}

Boolean PTLockPixels(PixMapHandle pm) {
    if (LockPixels(pm))
        return true;
    return false;
}

// Stack-based context restoration
void SaveContext(void) {
    GetGWorld(&contextWorld[contextStackPointer], &contextDevice[contextStackPointer]);
    if (contextStackPointer < (CONTEXTSTACKSIZE - 1))
        contextStackPointer++;
}

void RestoreContext(void) {
    if (contextStackPointer > 0)
        contextStackPointer--;
    SetGWorld(contextWorld[contextStackPointer], contextDevice[contextStackPointer]);
}
