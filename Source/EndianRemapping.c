/*
 *  EndianRemapping.c
 *  PixelToy
 *
 *  Created by Leon McNeill on 10/26/10.
 *  Copyright 2010 Lairware. All rights reserved.
 *
 */

#include "EndianRemapping.h"

Rect RectSwap(Rect *theRect) {
    Rect result;
    result.left = Endian16_Swap(theRect->left);
    result.top = Endian16_Swap(theRect->top);
    result.right = Endian16_Swap(theRect->right);
    result.bottom = Endian16_Swap(theRect->bottom);
    return result;
}

void SwapPrefsBytes(struct preferences *thePrefs) {
    int i;

    thePrefs->winxsize = Endian16_Swap(thePrefs->winxsize);
    thePrefs->winysize = Endian16_Swap(thePrefs->winysize);
    thePrefs->storedxsize = Endian16_Swap(thePrefs->storedxsize);
    thePrefs->storedysize = Endian16_Swap(thePrefs->storedysize);
    for (i = 0; i < MAXWINDOW; i++) {
        thePrefs->windowPosition[i].h =
            Endian16_Swap(thePrefs->windowPosition[i].h);
        thePrefs->windowPosition[i].v =
            Endian16_Swap(thePrefs->windowPosition[i].v);
    }
    thePrefs->visiblePrefsTab = Endian16_Swap(thePrefs->visiblePrefsTab);
    thePrefs->FPSlimit = Endian16_Swap(thePrefs->FPSlimit);
    thePrefs->autoSetInterval = Endian16_Swap(thePrefs->autoSetInterval);
    thePrefs->palTransFrames = Endian16_Swap(thePrefs->palTransFrames);
    for (i = 0; i < MAXACTION; i++) {
        thePrefs->autoPilotValue[i] =
            Endian16_Swap(thePrefs->autoPilotValue[i]);
    }
    thePrefs->QTMode = Endian16_Swap(thePrefs->QTMode);
    thePrefs->QTIncludeAudio = Endian16_Swap(thePrefs->QTIncludeAudio);

    thePrefs->mon.h = Endian16_Swap(thePrefs->mon.h);
    thePrefs->mon.v = Endian16_Swap(thePrefs->mon.v);
    thePrefs->mon.depth = Endian16_Swap(thePrefs->mon.depth);
    thePrefs->mon.bounds.top = Endian16_Swap(thePrefs->mon.bounds.top);
    thePrefs->mon.bounds.left = Endian16_Swap(thePrefs->mon.bounds.left);
    thePrefs->mon.bounds.right = Endian16_Swap(thePrefs->mon.bounds.right);
    thePrefs->mon.bounds.bottom = Endian16_Swap(thePrefs->mon.bounds.bottom);
    thePrefs->mon.refresh = Endian32_Swap(thePrefs->mon.refresh);

    thePrefs->soundInputDevice = Endian16_Swap(thePrefs->soundInputDevice);
    thePrefs->soundInputSource = Endian16_Swap(thePrefs->soundInputSource);
    thePrefs->soundInputSoftwareGain =
        Endian16_Swap(thePrefs->soundInputSoftwareGain);
    thePrefs->soundInputHardwareGain =
        Endian16_Swap(thePrefs->soundInputHardwareGain);
}

void SwapFilterBytes(struct pixeltoyfilter *theFilter) {
    theFilter->target = Endian32_Swap(theFilter->target);
}

void SwapSetBytes(struct setinformation *theSet) {
    int i;
    for (i = 0; i < 256; i++) {
        theSet->palentry[i].red = Endian16_Swap(theSet->palentry[i].red);
        theSet->palentry[i].green = Endian16_Swap(theSet->palentry[i].green);
        theSet->palentry[i].blue = Endian16_Swap(theSet->palentry[i].blue);
    }
    SwapFilterBytes(&theSet->customfilter);
    for (i = 0; i < MAXACTION; i++) {
        theSet->actCount[i] = Endian16_Swap(theSet->actCount[i]);
        theSet->actSiz[i] = Endian16_Swap(theSet->actSiz[i]);
        theSet->actSizeVar[i] = Endian16_Swap(theSet->actSizeVar[i]);
        theSet->actMisc1[i] = Endian16_Swap(theSet->actMisc1[i]);
        theSet->actMisc2[i] = Endian16_Swap(theSet->actMisc2[i]);
    }
    for (i = 0; i < MAXTEXTS; i++) {
        theSet->textGen[i].fontID = Endian16_Swap(theSet->textGen[i].fontID);
        theSet->textGen[i].size = Endian16_Swap(theSet->textGen[i].size);
        theSet->textGen[i].xpos = Endian16_Swap(theSet->textGen[i].xpos);
        theSet->textGen[i].ypos = Endian16_Swap(theSet->textGen[i].ypos);
        theSet->textGen[i].brightness =
            Endian16_Swap(theSet->textGen[i].brightness);
        theSet->textGen[i].behave = Endian16_Swap(theSet->textGen[i].behave);
        theSet->textGen[i].reserved1 =
            Endian16_Swap(theSet->textGen[i].reserved1);
        theSet->textGen[i].reserved2 =
            Endian16_Swap(theSet->textGen[i].reserved2);
    }
    for (i = 0; i < MAXGENERATORS; i++) {
        theSet->pg_pbehavior[i] = Endian16_Swap(theSet->pg_pbehavior[i]);
        theSet->pg_genaction[i] = Endian16_Swap(theSet->pg_genaction[i]);
        theSet->pg_solid[i] = Endian16_Swap(theSet->pg_solid[i]);
        theSet->pg_rate[i] = Endian16_Swap(theSet->pg_rate[i]);
        theSet->pg_gravity[i] = Endian16_Swap(theSet->pg_gravity[i]);
        theSet->pg_xloc[i] = Endian16_Swap(theSet->pg_xloc[i]);
        theSet->pg_xlocv[i] = Endian16_Swap(theSet->pg_xlocv[i]);
        theSet->pg_yloc[i] = Endian16_Swap(theSet->pg_yloc[i]);
        theSet->pg_ylocv[i] = Endian16_Swap(theSet->pg_ylocv[i]);
        theSet->pg_dx[i] = Endian16_Swap(theSet->pg_dx[i]);
        theSet->pg_dxv[i] = Endian16_Swap(theSet->pg_dxv[i]);
        theSet->pg_dy[i] = Endian16_Swap(theSet->pg_dy[i]);
        theSet->pg_dyv[i] = Endian16_Swap(theSet->pg_dyv[i]);
        theSet->pg_size[i] = Endian16_Swap(theSet->pg_size[i]);
        theSet->pg_brightness[i] = Endian16_Swap(theSet->pg_brightness[i]);
        theSet->pg_reserved2[i] = Endian16_Swap(theSet->pg_reserved2[i]);
    }
    for (i = 0; i < MAXIMAGES; i++) {
        theSet->image[i].active = Endian16_Swap(theSet->image[i].active);
        theSet->image[i].orgwidth = Endian16_Swap(theSet->image[i].orgwidth);
        theSet->image[i].orgheight = Endian16_Swap(theSet->image[i].orgheight);
        theSet->image[i].action = Endian16_Swap(theSet->image[i].action);
        int j;
        for (j = 0; j < 8; j++) {
            theSet->image[i].actionVar[j] =
                Endian16_Swap(theSet->image[i].actionVar[j]);
        }
        theSet->image[i].place = RectSwap(&theSet->image[i].place);

        theSet->image[i].depth = Endian16_Swap(theSet->image[i].depth);
        theSet->image[i].britemode = Endian16_Swap(theSet->image[i].britemode);
        theSet->image[i].brightness =
            Endian16_Swap(theSet->image[i].brightness);

        theSet->image[i].maskfss.vRefNum =
            Endian16_Swap(theSet->image[i].maskfss.vRefNum);
        theSet->image[i].maskfss.parID =
            Endian32_Swap(theSet->image[i].maskfss.parID);

        theSet->image[i].reserved1 = Endian16_Swap(theSet->image[i].reserved1);
        theSet->image[i].reserved2 = Endian16_Swap(theSet->image[i].reserved2);
    }
    theSet->horizontalMirror = Endian16_Swap(theSet->horizontalMirror);
    theSet->verticalMirror = Endian16_Swap(theSet->verticalMirror);
    theSet->constrainMirror = Endian16_Swap(theSet->constrainMirror);
    theSet->paletteSoundMode = Endian16_Swap(theSet->paletteSoundMode);
    for (i = 0; i < MAXWAVES; i++) {
        theSet->wave[i].thickness = Endian16_Swap(theSet->wave[i].thickness);
        theSet->wave[i].spacing = Endian16_Swap(theSet->wave[i].spacing);
        theSet->wave[i].brightness = Endian16_Swap(theSet->wave[i].brightness);
        theSet->wave[i].action = Endian16_Swap(theSet->wave[i].action);
        int j;
        for (j = 0; j < 8; j++) {
            theSet->wave[i].actionVar[j] =
                Endian16_Swap(theSet->wave[i].actionVar[j]);
        }
        theSet->wave[i].place = RectSwap(&theSet->wave[i].place);
        theSet->wave[i].reserved1 = Endian16_Swap(theSet->wave[i].reserved1);
        theSet->wave[i].reserved2 = Endian16_Swap(theSet->wave[i].reserved2);
    }
}

void RemapPaletteToN(struct PTPalette *thePalette) {
    Boolean doSwap = false;
#if TARGET_RT_LITTLE_ENDIAN
    doSwap = (thePalette->palname[63] == 0);
    thePalette->palname[63] = 1;
#elif TARGET_RT_BIG_ENDIAN
    doSwap = (thePalette->palname[63] == 1);
    thePalette->palname[63] = 0;
#endif
    if (!doSwap)
        return;

    int i;
    for (i = 0; i < 256; i++) {
        thePalette->palentry[i].red =
            Endian16_Swap(thePalette->palentry[i].red);
        thePalette->palentry[i].green =
            Endian16_Swap(thePalette->palentry[i].green);
        thePalette->palentry[i].blue =
            Endian16_Swap(thePalette->palentry[i].blue);
    }
}

#pragma mark -
// ------------------------------------------------------------

void RemapPrefsNtoB(struct preferences *thePrefs) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapPrefsBytes(thePrefs);
#endif
}

void RemapPrefsBtoN(struct preferences *thePrefs) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapPrefsBytes(thePrefs);
#endif
}

void RemapPrefsLtoN(struct preferences *thePrefs) {
#if TARGET_RT_BIG_ENDIAN
    SwapPrefsBytes(thePrefs);
#endif
}

void RemapSetNtoB(struct setinformation *theSet) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapSetBytes(theSet);
#endif
}

void RemapSetBtoN(struct setinformation *theSet) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapSetBytes(theSet);
#endif
}

void RemapSetLtoN(struct setinformation *theSet) {
#if TARGET_RT_BIG_ENDIAN
    SwapSetBytes(theSet);
#endif
}

void RemapFilterNtoB(struct pixeltoyfilter *theFilter) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapFilterBytes(theFilter);
#endif
}

void RemapFilterBtoN(struct pixeltoyfilter *theFilter) {
#if TARGET_RT_LITTLE_ENDIAN
    SwapFilterBytes(theFilter);
#endif
}
