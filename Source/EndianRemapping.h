/*
 *  EndianRemapping.h
 *  PixelToy
 *
 *  Created by Leon McNeill on 10/26/10.
 *  Copyright 2010 Lairware. All rights reserved.
 *
 */

#include "Defs&Structs.h"

void RemapPrefsNtoB(struct preferences *thePrefs);
void RemapPrefsBtoN(struct preferences *thePrefs);
void RemapPrefsLtoN(struct preferences *thePrefs);

void RemapSetNtoB(struct setinformation *theSet);
void RemapSetBtoN(struct setinformation *theSet);
void RemapSetLtoN(struct setinformation *theSet);

void RemapFilterNtoB(struct pixeltoyfilter *theFilter);
void RemapFilterBtoN(struct pixeltoyfilter *theFilter);

void RemapPaletteToN(struct PTPalette *thePalette);

