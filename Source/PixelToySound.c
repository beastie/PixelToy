/* PixelToySound.c */

//#include <Movies.h>
#include "PixelToySound.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyIF.h"

#define NUMSOUNDS 2
#define NUMCHANNELS 2

extern struct preferences prefs;
extern struct setinformation curSet;
extern WindowPtr gMainWindow;
extern CGrafPtr offWorld;
extern Boolean gCapableSoundIn, gCapableSoundOut, gQTExporting;
extern short fromByteCount, gSysVersionMajor;
extern Media theSoundMedia;
extern SoundDescriptionHandle soundDesc;
extern long gSoundInRefNum;

Boolean gTwosComplement, gStopRecording, gWaitingToRecord = false;
Boolean gSISupportsHWGain, gSIPlaythruChangeable, gSIAGCChangeable;
short gCurChan, gMaxChan, gNumInputChannels, gSampleBytes;
SndChannelPtr gSampChan[NUMCHANNELS + 1], gToneChan;
Handle gSoundHandle[NUMSOUNDS];
long inDeviceBufSize;
Ptr soundPtr[4];
SPBPtr gSoundParm;

short orgSoundInputDevice = -1, orgSoundInputPlayThru = -1;

// local prototypes
pascal void MyRecordCompletion(SPBPtr inSPBPtr);
void SetSoundInputSettings(void);

void OpenSoundIn(short devNum) {
    OSErr err;
    SICompletionUPP myRecCompUPP;
    Str255 str;
    Handle devIconHandle = nil;

    // uncomment below code for fake sound data testing
    /*
    gSoundInRefNum = 0; gCapableSoundIn = true; gWaitingToRecord = false;
    inDeviceBufSize=2048;
    soundPtr[0] = NewPtrClear(inDeviceBufSize);
    gSoundParm = (SPBPtr)NewPtrClear(sizeof(SPB));
    gSoundParm->error = noErr;
    return;
*/
    if (devNum < 1 || ControlIsPressed()) {
        gCapableSoundIn = false;
        return;
    }    // Control forces no sound

    err = SPBGetIndexedDevice(devNum, str, &devIconHandle);
    if (!err) {
        if (devIconHandle)
            DisposeHandle(devIconHandle);
        err = SPBOpenDevice(str, siWritePermission, &gSoundInRefNum);
    } else
        err = SPBOpenDevice(nil, siWritePermission, &gSoundInRefNum);

    gCapableSoundIn = (err == noErr);
    if (gCapableSoundIn) {
        SetSoundInputSettings();
        soundPtr[0] = NewPtrClear(inDeviceBufSize);
        if (!soundPtr[0])
            StopError(73, memFullErr);
        myRecCompUPP = NewSICompletionUPP(MyRecordCompletion);
        gSoundParm = (SPBPtr)NewPtrClear(sizeof(SPB));
        if (!gSoundParm)
            StopError(77, memFullErr);

        gSoundParm->completionRoutine = myRecCompUPP;
        gSoundParm->userLong = 0;
        gWaitingToRecord = true;
    }
}

pascal void MyRecordCompletion(SPBPtr inSPBPtr) {
    SInt32 orgA5 = SetCurrentA5();
    SetA5(gSoundParm->userLong);
    gWaitingToRecord = true;
    SetA5(orgA5);
}

OSErr MaintainSoundInput(void) {
    OSErr theErr = noErr;

    // Debugging fake sound data
    if (gSoundInRefNum == 0) {
        int i;
        char delta = 0;
        unsigned char v = 127;
        for (i = GetPtrSize(soundPtr[0]) - 1; i >= 0; i--) {
            soundPtr[0][i] = v;
            v += delta;
            delta += RandVariance(2);
            if (delta > 4)
                delta = 4;
            if (delta < -4)
                delta = -4;
            if (v < 40) {
                delta += 2;
            }
            if (v > 210) {
                delta -= 2;
            }
        }
        gNumInputChannels = 1;
        gSampleBytes = 1;
        theErr = noErr;
    }

    // Actually get sound data
    if (gCapableSoundIn && gWaitingToRecord) {
        gWaitingToRecord = false;
        gSoundParm->userLong = SetCurrentA5();
        gSoundParm->inRefNum = gSoundInRefNum;
        gSoundParm->count = inDeviceBufSize / gSampleBytes;
        gSoundParm->milliseconds = 0;    // ignore it
        gSoundParm->bufferLength = inDeviceBufSize;
        gSoundParm->bufferPtr = soundPtr[0];
        gSoundParm->interruptRoutine = nil;
        gSoundParm->error = noErr;
        gSoundParm->unused1 = 0;
        theErr = SPBRecord(gSoundParm, true);
        if (theErr) {
            NoteError(78, theErr);
            CloseSoundIn();
        }
    }

    return theErr;
}

Ptr GetSoundInput16BitSamples(UInt32 desiredNumberOfSamples) {
    MaintainSoundInput();
    if (!gCapableSoundIn || (gSampleBytes < 1 || gSampleBytes > 2)) {
        fprintf(stderr, "GetSoundInput16BitSamples gCapableSoundIn=%d gSampleBytes=%d\n", gCapableSoundIn, gSampleBytes);
        return nil;
    }

    unsigned numSamplesAvailable = (gSoundParm->bufferLength / gSampleBytes);
    Ptr outPtr = NewPtr(desiredNumberOfSamples * sizeof(SInt16));
    SInt16 *OutAs16 = (SInt16 *)outPtr;
    int scaler = (desiredNumberOfSamples / numSamplesAvailable) + 1;
    int i;
    for (i = 0; i < desiredNumberOfSamples; i++) {
        unsigned offset = i / scaler;
        if (offset < numSamplesAvailable) {
            SInt16 value = 0;
            if (gSampleBytes == 1) {
                SInt8 *InPtrAs8 = (SInt8 *)soundPtr[0];
                value = InPtrAs8[offset] << 8;
            } else if (gSampleBytes == 2) {
                SInt16 *InPtrAs16 = (SInt16 *)soundPtr[0];
                value = InPtrAs16[offset];
            } else if (gSampleBytes == 4) {
                SInt32 *InPtrAs32 = (SInt32 *)soundPtr[0];
                value = InPtrAs32[offset] >> 16;
            }
            OutAs16[i] = value;
        }
    }

    return outPtr;
}

void SetSoundInputSettings(void) {
    OSErr err;
    short snum;
    OSType ostype;
    if (!gCapableSoundIn)
        return;
    // Remember what was originally set so we can restore it later
    if (orgSoundInputDevice == -1) {
        err = SPBGetDeviceInfo(gSoundInRefNum, siInputSource, &orgSoundInputDevice);
        if (err)
            orgSoundInputDevice = -1;
        err = SPBGetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &orgSoundInputPlayThru);
        if (err)
            orgSoundInputPlayThru = -1;
    }
    // Establish sound input device number
    if (prefs.soundInputSource == -1) {   // default, so try for CD
        ostype = kCDSource;
        err = SPBSetDeviceInfo(gSoundInRefNum, siOSTypeInputSource, &ostype);
        err = SPBGetDeviceInfo(gSoundInRefNum, siInputSource, &snum);
        if (!err)
            prefs.soundInputSource = snum;
    }
    if (prefs.soundInputSource > 0) {
        snum = prefs.soundInputSource;
        err = SPBSetDeviceInfo(gSoundInRefNum, siInputSource, &snum);
        if (err) {   // couldn't use device we're used to, find out current setting and keep it
            err = SPBGetDeviceInfo(gSoundInRefNum, siInputSource, &snum);
            prefs.soundInputSource = snum;
        }
    }

    // Establish Playthrough
    err = SPBGetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &snum);
    gSIPlaythruChangeable = (err == noErr);
    if (gSIPlaythruChangeable) {
        short test = !snum;
        err = SPBSetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &test);
        err = SPBGetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &test);
        gSIPlaythruChangeable = (err == noErr && test != snum);
        if (gSIPlaythruChangeable) {   // now we know it's really changeable
            snum = prefs.soundInputPlayThru;
            err = SPBSetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &snum);
        }
    }

    // Determine sample size (try for 16)
    snum = 16;
    err = SPBSetDeviceInfo(gSoundInRefNum, siSampleSize, &snum);
    err = SPBGetDeviceInfo(gSoundInRefNum, siSampleSize, &snum);
    gSampleBytes = (snum / 8);
    // Establish # of input channels (try for just 1)
    snum = 1;
    err = SPBSetDeviceInfo(gSoundInRefNum, siNumberChannels, &snum);
    err = SPBGetDeviceInfo(gSoundInRefNum, siNumberChannels, &gNumInputChannels);
    // Determine Twos Complement value
    err = SPBGetDeviceInfo(gSoundInRefNum, siTwosComplementOnOff, &snum);
    gTwosComplement = (snum > 0);
    if (siSampleSize != 8)
        gTwosComplement = true;

    // Establish Automatic Gain Control (and whether or not it is changeable)
    err = SPBGetDeviceInfo(gSoundInRefNum, siAGCOnOff, &snum);
    gSIAGCChangeable = (err == noErr);
    if (gSIAGCChangeable) {
        short test = !snum;
        err = SPBSetDeviceInfo(gSoundInRefNum, siAGCOnOff, &test);
        err = SPBGetDeviceInfo(gSoundInRefNum, siAGCOnOff, &test);
        gSIAGCChangeable = (err == noErr && test != snum);
        if (gSIAGCChangeable) {   // now we know it's really changeable
            snum = prefs.soundInputAutoGain;
            err = SPBSetDeviceInfo(gSoundInRefNum, siAGCOnOff, &snum);
        }
    }

    // Establish Hardware audio gain
    // assume it is supported, SetSoundInputGain will turn off gSISupportsHWGain if not.
    gSISupportsHWGain = true;
    SetSoundInputGain(gSoundInRefNum, prefs.soundInputHardwareGain);

    // Finished configuring input device, now determine device buffer size
    long desiredBufferSize = 4096 * gSampleBytes;
    err = SPBSetDeviceInfo(gSoundInRefNum, siDeviceBufferInfo, &desiredBufferSize);
    err = SPBGetDeviceInfo(gSoundInRefNum, siDeviceBufferInfo, &inDeviceBufSize);
    if (err)
        inDeviceBufSize = 8192;
    if (inDeviceBufSize == 0)
        inDeviceBufSize = 256;
}

void CloseSoundIn(void) {
    OSErr err;

    if (gCapableSoundIn) {
        gStopRecording = true;
        gCapableSoundIn = false;
        err = SPBStopRecording(gSoundInRefNum);
        if (err)
            NoteError(76, err);
        if (orgSoundInputDevice != -1)
            err = SPBSetDeviceInfo(gSoundInRefNum, siInputSource, &orgSoundInputDevice);
        if (orgSoundInputPlayThru != -1)
            err = SPBSetDeviceInfo(gSoundInRefNum, siPlayThruOnOff, &orgSoundInputPlayThru);
        err = SPBCloseDevice(gSoundInRefNum);
        if (err)
            NoteError(52, err);
        DisposePtr(soundPtr[0]);
        DisposePtr((Ptr)gSoundParm);
    }
}

OSErr SetSoundInputGain(int soundInRefNum, short intGain) {
    if (gCapableSoundIn && gSISupportsHWGain) {    // intGain -50 = 0.5, +50 = 1.5
        Fixed fnum = (Fixed)(((float)intGain + 100.0) * 655.36);
        OSErr err = SPBSetDeviceInfo(soundInRefNum, siInputGain, &fnum);
        if (err)
            gSISupportsHWGain = false;
        if (gSISupportsHWGain) {
            Fixed test;
            err = SPBGetDeviceInfo(soundInRefNum, siInputGain, &test);
            if (err || test != fnum)
                gSISupportsHWGain = false;
        }
        return err;
    }
    return siInputDeviceErr;
}

void WaitUntilRecordingComplete(void) {
    if (gSoundParm->error > 0) {
        long beginWait = TickCount();
        while ((gSoundParm->error > 0) && (TickCount() - beginWait) < 30) {
        }
    }
}

short RecentVolume(void) {    // 0-128, -1 = no sound in
    long max, c;
    short peak = 0, value, bufNum;
    static short result;

    if (!gCapableSoundIn)
        return -1;
    WaitUntilRecordingComplete();

    bufNum = 0;
    max = inDeviceBufSize / gSampleBytes;
    if (max > 200)
        max = 200;
    for (c = 0; c < max; c++) {
        value = (unsigned char)soundPtr[bufNum][c * gSampleBytes + (gSampleBytes - 1)];
        if (gTwosComplement) {
            if (value < 128) {
                value += 128;
            } else {
                value -= 128;
            }
        }
        value = 128 - value;
        if (value < 0)
            value = -value;
        if (value > peak)
            peak = value;
    }
    result = peak;
    if (prefs.soundInputSoftwareGain) {
        result = (float)result * ((float)prefs.soundInputSoftwareGain / 25.0);
        if (result > 128)
            result = 128;
    }
    return result;
}

void PlaySound(short what, Boolean async) {
    Boolean clearChan;
    long err;
    SCStatus status;
    short startChannel;

    if (!gCapableSoundOut)
        return;
    if (curSet.sound) {
        clearChan = false;
        startChannel = gCurChan;
        while (clearChan == false) {
            err = SndChannelStatus(gSampChan[gCurChan], sizeof(status), &status);
            if (status.scChannelBusy) {
                gCurChan++;
                if (gCurChan > gMaxChan)
                    gCurChan = 1;
                if (gCurChan == startChannel)
                    clearChan = true;
            } else {
                clearChan = true;
            }
        }
        err = SndPlay(gSampChan[gCurChan], (SndListHandle)gSoundHandle[what - BASERES], async);
        if (err)
            StopError(56, err);
    }
}

void OpenChannel(void) {
    short err, chanType, soundNum, chanNum;
    Boolean channelStop;

    chanType = sampledSynth;
    gCurChan = 1;

    err = SndNewChannel(&gSampChan[0], chanType, 0, nil);
    if (err) {
        if (ControlIsPressed())
            StopError(54, err);
        gCapableSoundOut = false;
        return;
    }
    gCapableSoundOut = true;
    for (soundNum = 0; soundNum < NUMSOUNDS; soundNum++) {
        gSoundHandle[soundNum] = GetResource('snd ', BASERES + soundNum);
        LoadResource(gSoundHandle[soundNum]);
        HLock(gSoundHandle[soundNum]);
        if (gSoundHandle[soundNum] == nil)
            StopError(55, ResError());
    }
    channelStop = false;
    chanNum = 1;
    while (chanNum < NUMCHANNELS && !channelStop) {
        err = SndNewChannel(&gSampChan[chanNum], chanType, 0, nil);
        if (err) {
            channelStop = true;
        } else {
            chanNum++;
        }
    }
    gMaxChan = chanNum;
}

void CloseChannel(void) {
    short err;

    return;

    err = SndDisposeChannel(gSampChan[1], false);
    err = SndDisposeChannel(gSampChan[2], false);
    err = SndDisposeChannel(gSampChan[3], false);
    err = SndDisposeChannel(gSampChan[4], false);
}

void PrepareSoundInputDeviceMenu(ControlHandle theControl) {
    short checkItemNum, i, menuItemNum, numItems;
    Handle devIconHandle;
    Str255 deviceName;
    OSErr err;
    MenuRef theMenu;

    if (!theControl)
        return;
    theMenu = GetControlPopupMenuHandle(theControl);
    if (!theMenu)
        return;
    numItems = CountMenuItems(theMenu);
    for (i = 3; i <= numItems; i++)
        DeleteMenuItem(theMenu, 3);
    menuItemNum = 2;
    i = 1;
    err = noErr;
    while (err == noErr && i < 64) {
        err = SPBGetIndexedDevice(i, deviceName, &devIconHandle);
        if (err == noErr) {
            menuItemNum++;
            AppendMenuItemText(theMenu, deviceName);
            if (devIconHandle)
                DisposeHandle(devIconHandle);
            i++;
        }
    }
    SetControlMaximum(theControl, i + 1);
    checkItemNum = 1;    // assume "none"
    if (prefs.soundInputDevice > 0 && prefs.soundInputDevice < i)
        checkItemNum = prefs.soundInputDevice + 2;
    SetControlValue(theControl, checkItemNum);
    if (i == 1) {
        while (CountMenuItems(theMenu) > 1)
            DeleteMenuItem(theMenu, 2);
    }
}

void PrepareSoundInputSourceMenu(ControlHandle theControl) {
    short numItems, i, c;
    long offset;
    Str255 sourceName;
    OSErr err;
    Handle namesHandle;
    MenuRef theMenu;

    if (!theControl)
        return;
    theMenu = GetControlPopupMenuHandle(theControl);
    if (!theMenu)
        return;
    numItems = CountMenuItems(theMenu);
    for (i = 1; i <= numItems; i++)
        DeleteMenuItem(theMenu, 1);
    // No device selected
    if (prefs.soundInputDevice < 1) {
        GetIndString(sourceName, BASERES, 54);
        AppendMenuItemText(theMenu, sourceName);
        SetControlValue(theControl, 1);
        DisableMenuItem(theMenu, 1);
        return;
    }
    // If not capable of sound input, use message
    if (!gCapableSoundIn) {
        GetIndString(sourceName, BASERES, 53);
        AppendMenuItemText(theMenu, sourceName);
        SetControlValue(theControl, 1);
        DisableMenuItem(theMenu, 1);
        return;
    }
    namesHandle = NewHandleClear(16384);
    if (!GoodHandle(namesHandle))
        return;
    err = SPBGetDeviceInfo(gSoundInRefNum, siInputSourceNames, &namesHandle);
    numItems = (unsigned char)(*namesHandle)[0] | (unsigned char)(*namesHandle)[1] * 256;
    if (numItems < 1) {
        GetIndString(sourceName, BASERES, 42);
        AppendMenuItemText(theMenu, sourceName);
        DisableMenuItem(theMenu, 1);
    } else {
        offset = 2;
        for (i = 0; i < numItems; i++) {
            c = (*namesHandle)[offset] + 1;
            if (c > 0 && c < 64) {
                BlockMoveData((*namesHandle) + offset, sourceName, c);
                offset += sourceName[0] + 1;
                AppendMenuItemText(theMenu, sourceName);
            } else {
                i = numItems;
            }    // force break
        }
    }
    if (namesHandle)
        DisposeHandle(namesHandle);
    SetControlMaximum(theControl, 32767);

    if (gCapableSoundIn) {
        ActivateControl(theControl);
        err = SPBGetDeviceInfo(gSoundInRefNum, siInputSource, &i);
        SetControlValue(theControl, i);
    } else {
        DeactivateControl(theControl);
    }
}
