// PixelToy Apple Event Handlers

#include "PixelToyAE.h"

#include "Defs&Structs.h"
#include "PixelToyIF.h"
#include "PixelToyMain.h"
#include "PixelToySets.h"

extern Boolean gDone, setInMemory;
extern struct preferences prefs;
extern PixMapHandle stage32PixMap;

Boolean InitializeAEHandlers(void) {
    OSErr error;
    AEEventHandlerUPP aehpp;

    aehpp = NewAEEventHandlerUPP(AEOpenApp);
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenApplication, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEOpenDocs);
    error = AEInstallEventHandler(kCoreEventClass, kAEOpenDocuments, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEPrintDocs);
    error = AEInstallEventHandler(kCoreEventClass, kAEPrintDocuments, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEQuit);
    error = AEInstallEventHandler(kCoreEventClass, kAEQuitApplication, aehpp, 0, false);
    if (error)
        return false;

    aehpp = NewAEEventHandlerUPP(AEShowPrefs);
    error = AEInstallEventHandler(kCoreEventClass, kAEShowPreferences, aehpp, 0, false);
    if (error)
        return false;

    return true;
}

static pascal OSErr AEOpenApp(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    LoadDefaultSets();
    return noErr;
}

static pascal OSErr AEOpenDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    OSErr error;
    AEDescList docList;
    AEKeyword keywd;
    DescType returnedType;
    long numItems, i;
    FSSpec fss;
    Size actualSize;

    error = AEGetParamDesc(event, keyDirectObject, typeAEList, &docList);
    if (!error) {
        error = AECountItems(&docList, &numItems);
        if (!error) {
            for (i = 1; i <= numItems; i++) {
                error = AEGetNthPtr(&docList, i, typeFSS, &keywd, &returnedType, &fss, sizeof(FSSpec), &actualSize);
                if (!error)
                    MyOpenFile(fss);
            }
            // if no sets file has yet been opened, open default now!
            if (!setInMemory)
                LoadDefaultSets();
        }
        AEDisposeDesc(&docList);
    }
    return error;
}

static pascal OSErr AEPrintDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    DoStandardAlert(kAlertStopAlert, 1);
    return noErr;
}

static pascal OSErr AEQuit(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    gDone = true;
    return noErr;
}

static pascal OSErr AEShowPrefs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon) {
    PreferencesDialog();
    return noErr;
}
