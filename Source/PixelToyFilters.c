/* PixelToyFilters.c */

#include "PixelToyFilters.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToyWindows.h"
#include "EndianRemapping.h"
#include "CocoaBridge.h"

extern MenuHandle gFiltersMenu;
extern PaletteHandle winPalette;
extern ModalFilterUPP DialogFilterProc;
extern struct preferences prefs;
extern struct setinformation curSet;
extern short blurByteCount, fromByteCount;
extern Ptr blurBasePtr, fromBasePtr;
extern Boolean gDone;

struct pixeltoyfilter editfilter;
short numFilterPoints, filterWeight[64];
long filterOffset[64];

void FilterBlur(short filterWidth, short filterHeight);
void FilterBlurMore(short filterWidth, short filterHeight);
void FilterBlurFast(short filterWidth, short filterHeight);
void FilterBlurSink(short filterWidth, short filterHeight);
void FilterWindWest(short filterWidth, short filterHeight);
void FilterWindEast(short filterWidth, short filterHeight);
void FilterSmearHoriz(short filterWidth, short filterHeight);
void FilterSmearVert(short filterWidth, short filterHeight);
void FilterSmearDiag(short filterWidth, short filterHeight);
void FilterZoomInFast(short filterWidth, short filterHeight);
void FilterZoomInSlow(short filterWidth, short filterHeight);
void FilterZoomOutSlow(short filterWidth, short filterHeight);
void FilterSpreadHoriz(short filterWidth, short filterHeight);
void FilterSpreadVert(short filterWidth, short filterHeight);
void FilterWaterfall(short filterWidth, short filterHeight);
void FilterFester(short filterWidth, short filterHeight);
void FilterFade(short filterWidth, short filterHeight);
void FilterKaleidoscope(short filterWidth, short filterHeight);
void FilterMelt(short filterWidth, short filterHeight);
void FilterWatercolor(short filterWidth, short filterHeight);
void FilterShimmer(short filterWidth, short filterHeight);
void FilterSplitHoriz(short filterWidth, short filterHeight);
void FilterSplitVert(short filterWidth, short filterHeight);
void FilterErase(short filterWidth, short filterHeight);

void ToggleFilter(short filter) {   // 0-31
    curSet.filter[filter] = !curSet.filter[filter];
    PTInvalidateWindow(WINFILTERS);
}

Boolean FilterActive(short filter) {
    return curSet.filter[filter];
}

void ClearAllFilters(void) {
    short c;
    for (c = 0; c < 256; c++) {
        curSet.filter[c] = 0;
    }
    PTInvalidateWindow(WINFILTERS);
}

void ToggleMirror(short what) {   // 0=horiz 1=vert 2=constrain
    CurSetTouched();
    if (what == 0)
        curSet.horizontalMirror = !curSet.horizontalMirror;
    if (what == 1)
        curSet.verticalMirror = !curSet.verticalMirror;
    if (what == 2)
        curSet.constrainMirror = !curSet.constrainMirror;
    PTInvalidateWindow(WINFILTERS);
    ShowSettings();
}

void FadeEdges(void) {
    short x, y;
    long offset;

    if (LockAllWorlds()) {
        offset = 0;
        for (x = 0; x < prefs.winxsize; x++) {
            if ((unsigned char)fromBasePtr[offset + x])
                fromBasePtr[offset + x]--;
        }
        offset += fromByteCount;
        for (y = 1; y < prefs.winysize - 1; y++) {
            if ((unsigned char)fromBasePtr[offset])
                fromBasePtr[offset]--;
            if ((unsigned char)fromBasePtr[offset + prefs.winxsize - 1])
                fromBasePtr[offset + prefs.winxsize - 1]--;
            offset += fromByteCount;
        }
        for (x = 0; x < prefs.winxsize; x++) {
            if ((unsigned char)fromBasePtr[offset + x])
                fromBasePtr[offset + x]--;
        }
    }
    UnlockAllWorlds();
}

void DoFilter(short filterNum) {
    short filterWidth, filterHeight;

    if (LockAllWorlds()) {
        filterWidth = prefs.winxsize;
        filterHeight = prefs.winysize;
        if (curSet.verticalMirror && curSet.constrainMirror)
            filterWidth = (prefs.winxsize / 2) + 2;
        if (curSet.horizontalMirror && curSet.constrainMirror)
            filterHeight = (prefs.winysize / 2) + 2;
        switch (filterNum) {
            case 0: FilterBlur(filterWidth, filterHeight); break;
            case 1: FilterBlurMore(filterWidth, filterHeight); break;
            case 2: FilterBlurFast(filterWidth, filterHeight); break;
            case 3: FilterBlurSink(filterWidth, filterHeight); break;
            case 4: FilterWindWest(filterWidth, filterHeight); break;
            case 5: FilterWindEast(filterWidth, filterHeight); break;
            case 6: FilterSmearHoriz(filterWidth, filterHeight); break;
            case 7: FilterSmearVert(filterWidth, filterHeight); break;
            case 8: FilterSmearDiag(filterWidth, filterHeight); break;
            case 9: FilterZoomInFast(filterWidth, filterHeight); break;
            case 10: FilterZoomInSlow(filterWidth, filterHeight); break;
            case 11: FilterZoomOutSlow(filterWidth, filterHeight); break;
            case 12: FilterSpreadHoriz(filterWidth, filterHeight); break;
            case 13: FilterSpreadVert(filterWidth, filterHeight); break;
            case 14: FilterWaterfall(filterWidth, filterHeight); break;
            case 15: FilterFester(filterWidth, filterHeight); break;
            case 16: FilterFade(filterWidth, filterHeight); break;
            case 17: FilterKaleidoscope(filterWidth, filterHeight); break;
            case 18: FilterMelt(filterWidth, filterHeight); break;
            case 19: FilterWatercolor(filterWidth, filterHeight); break;
            case 20: FilterShimmer(filterWidth, filterHeight); break;
            case 21: FilterSplitHoriz(filterWidth, filterHeight); break;
            case 22: FilterSplitVert(filterWidth, filterHeight); break;
            case 23: FilterErase(filterWidth, filterHeight); break;
            case 24: DoCustomFilter(); break;
        }
    }
    UnlockAllWorlds();
}

void FilterBlur(short filterWidth, short filterHeight) {
#if USE_GCD
    unsigned x;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = (unsigned char)fromBasePtr[fromByteCount + x];
    }
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = fromByteCount * (current + 1);
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        unsigned short x = 0;
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 4 + (unsigned char)fromBasePtr[x + offset1 - 1] +
                 (unsigned char)fromBasePtr[x + offset1 + 1] + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount]) >>
                3;
        }
    });
    long offset1 = (filterHeight - 1) * fromByteCount;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }

#else
    unsigned short x, y;
    long offset1;

    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = (unsigned char)fromBasePtr[fromByteCount + x];
    }
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 4 + (unsigned char)fromBasePtr[x + offset1 - 1] +
                 (unsigned char)fromBasePtr[x + offset1 + 1] + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount]) >>
                3;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
#endif
}

void FilterBlurMore(short filterWidth, short filterHeight) {
#if USE_GCD
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (y = 0; y < 3; y++) {
        for (x = 0; x < filterWidth; x++) {
            fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 + fromByteCount];
        }
        offset1 += fromByteCount;
    }
    long doubleFrom = (fromByteCount * 2);

    dispatch_apply(filterHeight - 6, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long thisOffset = (current + 3) * fromByteCount;
        fromBasePtr[thisOffset] = (unsigned char)fromBasePtr[thisOffset + 1];
        fromBasePtr[thisOffset + 1] = (unsigned char)fromBasePtr[thisOffset + 2];
        fromBasePtr[thisOffset + 2] = (unsigned char)fromBasePtr[thisOffset + 3];
        fromBasePtr[thisOffset + filterWidth - 1] = (unsigned char)fromBasePtr[thisOffset + filterWidth - 2];
        fromBasePtr[thisOffset + filterWidth - 2] = (unsigned char)fromBasePtr[thisOffset + filterWidth - 3];
        fromBasePtr[thisOffset + filterWidth - 3] = (unsigned char)fromBasePtr[thisOffset + filterWidth - 4];
        int x;
        for (x = 3; x < filterWidth - 3; x++) {
            fromBasePtr[x + thisOffset] =
                ((unsigned char)fromBasePtr[x + thisOffset] * 16 +
                 (unsigned char)fromBasePtr[x + thisOffset - fromByteCount - 1] * 2 +
                 (unsigned char)fromBasePtr[x + thisOffset + fromByteCount - 1] * 2 +
                 (unsigned char)fromBasePtr[x + thisOffset - fromByteCount + 1] * 2 +
                 (unsigned char)fromBasePtr[x + thisOffset + fromByteCount + 1] * 2 +
                 (unsigned char)fromBasePtr[x + thisOffset - 3] + (unsigned char)fromBasePtr[x + thisOffset + 3] +
                 (unsigned char)fromBasePtr[x + thisOffset + doubleFrom - 2] +
                 (unsigned char)fromBasePtr[x + thisOffset + doubleFrom + 2] +
                 (unsigned char)fromBasePtr[x + thisOffset - doubleFrom + 2] +
                 (unsigned char)fromBasePtr[x + thisOffset - doubleFrom - 2] +
                 (unsigned char)fromBasePtr[x + thisOffset - doubleFrom - fromByteCount] +
                 (unsigned char)fromBasePtr[x + thisOffset + doubleFrom + fromByteCount]) >>
                5;
        }
    });

    for (y = filterHeight - 1; y > filterHeight - 4; y--) {
        offset1 = y * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
        }
        offset1 += fromByteCount;
    }
#else
    unsigned short x, y;
    long offset1, offset2, offset3;

    offset1 = 0;
    for (y = 0; y < 3; y++) {
        for (x = 0; x < filterWidth; x++) {
            fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 + fromByteCount];
        }
        offset1 += fromByteCount;
    }
    offset2 = (fromByteCount * 2);
    offset3 = (fromByteCount * 3);
    for (y = 3; y < filterHeight - 3; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + 1] = (unsigned char)fromBasePtr[offset1 + 2];
        fromBasePtr[offset1 + 2] = (unsigned char)fromBasePtr[offset1 + 3];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        fromBasePtr[offset1 + filterWidth - 2] = (unsigned char)fromBasePtr[offset1 + filterWidth - 3];
        fromBasePtr[offset1 + filterWidth - 3] = (unsigned char)fromBasePtr[offset1 + filterWidth - 4];
        for (x = 3; x < filterWidth - 3; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 16 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount - 1] * 2 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount - 1] * 2 +
                 (unsigned char)fromBasePtr[x + offset1 - fromByteCount + 1] * 2 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount + 1] * 2 + (unsigned char)fromBasePtr[x + offset1 - 3] +
                 (unsigned char)fromBasePtr[x + offset1 + 3] + (unsigned char)fromBasePtr[x + offset1 + offset2 - 2] +
                 (unsigned char)fromBasePtr[x + offset1 + offset2 + 2] + (unsigned char)fromBasePtr[x + offset1 - offset2 + 2] +
                 (unsigned char)fromBasePtr[x + offset1 - offset2 - 2] +
                 (unsigned char)fromBasePtr[x + offset1 - offset2 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + offset2 + fromByteCount]) >>
                5;
        }
        offset1 += fromByteCount;
    }
    for (y = filterHeight - 1; y > filterHeight - 4; y--) {
        offset1 = y * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
        }
        offset1 += fromByteCount;
    }
#endif
}

void FilterBlurFast(short filterWidth, short filterHeight) {
    int x;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = (unsigned char)fromBasePtr[x + fromByteCount];
    }
    long offset1 = fromByteCount;
#if USE_GCD
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = fromByteCount * (current + 1);
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        int x;
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 24 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 24 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 24 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 24) >>
                8;
        }
        offset1 += fromByteCount;
    });
    offset1 = fromByteCount * (filterHeight - 1);
#else
    int y;
    for (y = 1; y < filterHeight - 1; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 24 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 24 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 24 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 24) >>
                8;
        }
        offset1 += fromByteCount;
    }
#endif
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
}

void FilterBlurSink(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 + fromByteCount];
    }
    offset1 = fromByteCount;
    for (y = 1; y < 3; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 96 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
    for (y = 3; y < (filterHeight - 1); y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[offset1 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 8 + (unsigned char)fromBasePtr[offset1 + x - 1] * 2 +
                 (unsigned char)fromBasePtr[offset1 + x + 1] * 2 + (unsigned char)fromBasePtr[offset1 + x - fromByteCount] +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount] +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount - fromByteCount] +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount - fromByteCount - fromByteCount]) >>
                4;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
}

void FilterWindWest(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1, offset2;

    offset1 = offset2 = 0;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount];
    }
    offset1 = fromByteCount;
    offset2 = blurByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        blurBasePtr[offset2] = blurBasePtr[offset2 + 1] = 0;
        for (x = 2; x < 4; x++) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x - 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x - 2] * 16) >>
                8;
        }
        for (x = 4; x < 8; x++) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x - 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x - 2] * 16 + (unsigned char)fromBasePtr[offset1 + x - 4] * 8) >>
                8;
        }
        for (x = 8; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x - 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x - 2] * 16 + (unsigned char)fromBasePtr[offset1 + x - 4] * 8 +
                 (unsigned char)fromBasePtr[offset1 + x - 6] * 8 + (unsigned char)fromBasePtr[offset1 + x - 8] * 16 +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount] * 8 +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount] * 8) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount];
    }
    BlurToOffBlit();
}

void FilterWindEast(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1, offset2;

    offset1 = offset2 = 0;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount];
    }
    offset1 = fromByteCount;
    offset2 = blurByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        blurBasePtr[offset2 + (filterWidth - 1)] = blurBasePtr[offset2 + (filterWidth - 2)] = 0;
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset1 + 1];
        for (x = (filterWidth - 3); x > (filterWidth - 5); x--) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x + 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x + 2] * 16) >>
                8;
        }
        for (x = (filterWidth - 5); x > (filterWidth - 9); x--) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x + 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x + 2] * 16 + (unsigned char)fromBasePtr[offset1 + x + 4] * 8) >>
                8;
        }
        for (x = (filterWidth - 9); x > 0; x--) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x + 1] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x + 2] * 16 + (unsigned char)fromBasePtr[offset1 + x + 4] * 8 +
                 (unsigned char)fromBasePtr[offset1 + x + 6] * 8 + (unsigned char)fromBasePtr[offset1 + x + 8] * 16 +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount] * 8 +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount] * 8) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
    BlurToOffBlit();
}

void FilterSmearHoriz(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 + fromByteCount];
    }
    offset1 += fromByteCount;
    for (y = 1; y < (filterHeight - 1); y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        // left edge
        for (x = 1; x < 6; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 16 + (unsigned char)fromBasePtr[x + offset1 + 2] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 3] * 8 + (unsigned char)fromBasePtr[x + offset1 + 4] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 5] * 8 + (unsigned char)fromBasePtr[x + offset1 + 6] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 16) >>
                8;
        }
        // right edge
        for (x = filterWidth - 6; x < (filterWidth - 1); x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 + 1] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 - 1] * 16 + (unsigned char)fromBasePtr[x + offset1 - 2] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - 3] * 8 + (unsigned char)fromBasePtr[x + offset1 - 4] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - 5] * 8 + (unsigned char)fromBasePtr[x + offset1 - 6] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 16) >>
                8;
        }
        // everything inbetween
        for (x = 6; x < filterWidth - 6; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 8 + (unsigned char)fromBasePtr[x + offset1 - 2] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 2] * 8 + (unsigned char)fromBasePtr[x + offset1 - 3] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 3] * 8 + (unsigned char)fromBasePtr[x + offset1 - 4] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 4] * 8 + (unsigned char)fromBasePtr[x + offset1 - 5] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 5] * 8 + (unsigned char)fromBasePtr[x + offset1 - 6] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + 6] * 8 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 16) >>
                8;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
}

void FilterSmearVert(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 + fromByteCount];
    }
    offset1 += fromByteCount;
    // top edge
    for (y = 1; y < 6; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < (filterWidth - 1); x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 2)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 3)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 4)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 5)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 6)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - 1] * 16 + (unsigned char)fromBasePtr[x + offset1 + 1] * 16) >>
                8;
        }
        offset1 += fromByteCount;
    }
    // middle
    for (y = 6; y < (filterHeight - 7); y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < (filterWidth - 1); x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 2)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 2)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 3)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 3)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 4)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 4)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 5)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 5)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 6)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 + (fromByteCount * 6)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - 1] * 16 + (unsigned char)fromBasePtr[x + offset1 + 1] * 16) >>
                8;
        }
        offset1 += fromByteCount;
    }
    // bottom edge
    for (y = filterHeight - 7; y < (filterHeight - 1); y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < (filterWidth - 1); x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 16 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 2)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 3)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 4)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 5)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - (fromByteCount * 6)] * 8 +
                 (unsigned char)fromBasePtr[x + offset1 - 1] * 16 + (unsigned char)fromBasePtr[x + offset1 + 1] * 16) >>
                8;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
}

void FilterSmearDiag(short filterWidth, short filterHeight) {
    unsigned short x, y;
    long offset1, offset2;

    offset1 = 1;
    offset2 = 0;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + fromByteCount + x];
    }
    offset1 = fromByteCount;
    offset2 = blurByteCount;
    for (y = 1; y < 3; y++) {
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset2 + 1];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            blurBasePtr[x + offset2] =
                ((unsigned char)fromBasePtr[x + offset1] * 96 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (y = 3; y < filterHeight - 3; y++) {
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset2 + 1];
        blurBasePtr[offset2 + 1] = (unsigned char)fromBasePtr[offset2 + 2];
        blurBasePtr[offset2 + 2] = (unsigned char)fromBasePtr[offset2 + 3];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        blurBasePtr[offset2 + filterWidth - 2] = (unsigned char)fromBasePtr[offset2 + filterWidth - 3];
        blurBasePtr[offset2 + filterWidth - 3] = (unsigned char)fromBasePtr[offset2 + filterWidth - 4];
        for (x = 3; x < filterWidth - 3; x++) {
            blurBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset1 + x - fromByteCount - 1] * 2 +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount - fromByteCount - 2] +
                 (unsigned char)fromBasePtr[offset1 + x - fromByteCount - fromByteCount - fromByteCount - 3] +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount + 1] * 2 +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount + fromByteCount + 2] +
                 (unsigned char)fromBasePtr[offset1 + x + fromByteCount + fromByteCount + fromByteCount + 3] +
                 (unsigned char)fromBasePtr[offset1 + x] * 8) >>
                4;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (y = filterHeight - 3; y < filterHeight - 1; y++) {
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset2 + 1];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            blurBasePtr[x + offset2] =
                ((unsigned char)fromBasePtr[x + offset1] * 96 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 - fromByteCount + x];
    }
    BlurToOffBlit();
}

void FilterZoomInFast(short filterWidth, short filterHeight) {
    long ye;
    unsigned short x, y, xm, ym;
    long offset1, offset2;
    long *xeArray;

    xm = (float)filterWidth * .05;
    ym = (float)filterHeight * .05;
    offset2 = 0;

    // build xeArray
    xeArray = (long *)NewPtr(filterWidth * sizeof(long));
    if (!xeArray)
        return;
    for (x = 0; x < filterWidth; x++)
        xeArray[x] = ((float)x * .9) + xm;

    // Scale bitmap to blur buffer
    for (y = 0; y < filterHeight; y++) {
        ye = ((float)y * .9) + ym;
        offset1 = ye * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + xeArray[x]];
            if (y == ye && x == xeArray[x])
                blurBasePtr[offset2 + x] = 0;
        }
        offset2 += blurByteCount;
    }
    DisposePtr((Ptr)xeArray);
    BlurToOffBlit();

    // then blur
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 4 + (unsigned char)fromBasePtr[x + offset1 - 1] +
                 (unsigned char)fromBasePtr[x + offset1 + 1] + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount]) >>
                3;
        }
        offset1 += fromByteCount;
    }
}

void FilterZoomInSlow(short filterWidth, short filterHeight) {
#if USE_GCD
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long xe, ye;
        unsigned short x, y, xf, yf, xm, ym;
        long val, offset1, offset2;
        y = current;
        ye = (y * 96) + (filterHeight * 2);    // source y * 100
        yf = ye % 100;                         // fraction of source (0-99)
        ym = ye / 100;                         // actual source y pixel
        offset1 = ym * fromByteCount;
        offset2 = y * blurByteCount;
        for (x = 0; x < filterWidth; x++) {
            xe = (x * 96) + (filterWidth * 2);    // source x * 100
            xf = xe % 100;                        // fraction of source (0-99)
            xm = xe / 100;                        // actual source x pixel

            val = (((unsigned char)fromBasePtr[offset1 + xm] * (100 - xf)) + ((unsigned char)fromBasePtr[offset1 + xm + 1] * xf)) *
                  (100 - yf);

            val += (((unsigned char)fromBasePtr[offset1 + xm + fromByteCount] * (100 - xf)) +
                    ((unsigned char)fromBasePtr[offset1 + xm + fromByteCount + 1] * xf)) *
                   yf;

            blurBasePtr[offset2 + x] = val / 10000;
        }
    });
    BlurToOffBlit();
    // post-fade
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset = current * fromByteCount;
        int x;
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset + x])
                fromBasePtr[offset + x]--;
        }
    });
#else
    long xe, ye;
    unsigned short x, y, xf, yf, xm, ym;
    long val, offset1, offset2;
    // zoom x,y=destination pixel, xm,ym=source pixel
    for (y = 0; y < filterHeight; y++) {
        ye = (y * 96) + (filterHeight * 2);    // source y * 100
        yf = ye % 100;                         // fraction of source (0-99)
        ym = ye / 100;                         // actual source y pixel
        offset1 = ym * fromByteCount;
        offset2 = y * blurByteCount;
        for (x = 0; x < filterWidth; x++) {
            xe = (x * 96) + (filterWidth * 2);    // source x * 100
            xf = xe % 100;                        // fraction of source (0-99)
            xm = xe / 100;                        // actual source x pixel

            val = (((unsigned char)fromBasePtr[offset1 + xm] * (100 - xf)) + ((unsigned char)fromBasePtr[offset1 + xm + 1] * xf)) *
                  (100 - yf);

            val += (((unsigned char)fromBasePtr[offset1 + xm + fromByteCount] * (100 - xf)) +
                    ((unsigned char)fromBasePtr[offset1 + xm + fromByteCount + 1] * xf)) *
                   yf;

            blurBasePtr[offset2 + x] = val / 10000;
        }
    }
    BlurToOffBlit();
    // post-fade
    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset1 + x])
                fromBasePtr[offset1 + x]--;
        }
        offset1 += fromByteCount;
    }
#endif
}

void FilterZoomOutSlow(short filterWidth, short filterHeight) {
    long xe, ye;
    unsigned short x, y, xf, yf, xm, ym;
    long val, offset1, offset2;

    // first clear "blur"
    offset2 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = 0;
        }
        offset2 += blurByteCount;
    }

    // now zoom out (shrink to 98% of original size, bicubic interp)
    for (y = 1; y < filterHeight - 1; y++) {
        ye = (y * 96) + (filterHeight * 2);    // dest y * 100
        yf = ye % 100;                         // fraction of dest (0-99)
        ym = ye / 100;                         // actual dest y pixel
        offset1 = y * fromByteCount;
        offset2 = ym * blurByteCount;
        for (x = 1; x < filterWidth - 1; x++) {
            xe = (x * 96) + (filterWidth * 2);    // dest x * 100
            xf = xe % 100;                        // fraction of dest (0-99)
            xm = xe / 100;                        // actual dest x pixel

            val =
                (((unsigned char)fromBasePtr[offset1 + x] * xf) + ((unsigned char)fromBasePtr[offset1 + x + 1] * (100 - xf))) * yf;

            val += (((unsigned char)fromBasePtr[offset1 + x + fromByteCount] * xf) +
                    ((unsigned char)fromBasePtr[offset1 + x + fromByteCount + 1] * (100 - xf))) *
                   (100 - yf);

            blurBasePtr[offset2 + xm] = val / 10000;
        }
    }
    BlurToOffBlit();
    // dim everything
    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset1 + x])
                fromBasePtr[offset1 + x]--;
        }
        offset1 += fromByteCount;
    }
}

void FilterSpreadHoriz(short filterWidth, short filterHeight) {
    unsigned short x, y, xm;
    long offset1, offset2;
    long *xeArray;

    xm = (float)filterWidth * .05;
    offset2 = 0;

    // build xeArray
    xeArray = (long *)NewPtr(filterWidth * sizeof(long));
    if (!xeArray)
        return;
    for (x = 0; x < filterWidth; x++)
        xeArray[x] = ((float)x * .9) + xm;

    for (y = 0; y < filterHeight; y++) {
        offset1 = y * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + xeArray[x]];
            if (x == xeArray[x]) {
                if (blurBasePtr[offset2 + x])
                    blurBasePtr[offset2 + x]--;
            }
        }
        offset2 += blurByteCount;
    }
    DisposePtr((Ptr)xeArray);

    BlurToOffBlit();
    // then blur
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
}

void FilterSpreadVert(short filterWidth, short filterHeight) {
    long ye;
    unsigned short x, y, ym;
    long offset1, offset2;

    ym = (float)filterHeight * .05;
    offset2 = 0;
    for (y = 0; y < filterHeight; y++) {
        ye = ((float)y * .9) + ym;
        offset1 = ye * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x];
            if (y == ye) {
                if (blurBasePtr[offset2 + x])
                    blurBasePtr[offset2 + x]--;
            }
        }
        offset2 += blurByteCount;
    }
    BlurToOffBlit();
    // then blur
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
}

void FilterWaterfall(short filterWidth, short filterHeight) {
#if USE_GCD
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long ye = ((float)current * .96);
        long offset1 = ye * fromByteCount;
        long offset2 = current * blurByteCount;
        int x;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x];
            if (current == ye) {
                if (blurBasePtr[offset2 + x])
                    blurBasePtr[offset2 + x]--;
            }
        }
    });
    BlurToOffBlit();
    // then blur
    int x;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = 0;
    }
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        int y = current + 1;
        long offset1 = fromByteCount * y;
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        int x;
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
    });
#else
    long ye;
    unsigned short x, y;
    long offset1, offset2;

    offset2 = 0;
    for (y = 0; y < filterHeight; y++) {
        ye = ((float)y * .96);
        offset1 = ye * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x];
            if (y == ye) {
                if (blurBasePtr[offset2 + x])
                    blurBasePtr[offset2 + x]--;
            }
        }
        offset2 += blurByteCount;
    }
    BlurToOffBlit();
    // then blur
    offset1 = 0;
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = 0;
    }
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 128 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
#endif
}

void FilterFester(short filterWidth, short filterHeight) {
#if USE_GCD
    long offset1 = 0;
    long offset2 = 0;

    int x;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x + fromByteCount];
    }
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = fromByteCount * (current + 1);
        long offset2 = blurByteCount * (current + 1);
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset1 + 1];
        blurBasePtr[offset2 + 1] = (unsigned char)fromBasePtr[offset1 + 2];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        blurBasePtr[offset2 + filterWidth - 2] = (unsigned char)fromBasePtr[offset1 + filterWidth - 3];
        int x;
        for (x = 2; x < filterWidth - 2; x++) {
            long val;
            val = ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x - 1] * 32 +
                   (unsigned char)fromBasePtr[offset1 + x + 1] * 32 + (unsigned char)fromBasePtr[offset1 + x - 2] +
                   (unsigned char)fromBasePtr[offset1 + x + 2] + (unsigned char)fromBasePtr[offset1 + x - fromByteCount] * 32 +
                   (unsigned char)fromBasePtr[offset1 + x + fromByteCount] * 32);
            if (val > 65535)
                val = 0;
            blurBasePtr[offset2 + x] = val >> 8;
        }
    });
    offset1 = fromByteCount * (filterHeight - 1);
    offset2 = blurByteCount * (filterHeight - 1);
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x - fromByteCount];
    }
#else
    unsigned short x, y;
    long val, offset1, offset2;

    offset1 = offset2 = 0;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x + fromByteCount];
    }
    offset1 = fromByteCount;
    offset2 = blurByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset1 + 1];
        blurBasePtr[offset2 + 1] = (unsigned char)fromBasePtr[offset1 + 2];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        blurBasePtr[offset2 + filterWidth - 2] = (unsigned char)fromBasePtr[offset1 + filterWidth - 3];
        for (x = 2; x < filterWidth - 2; x++) {
            val = ((unsigned char)fromBasePtr[offset1 + x] * 128 + (unsigned char)fromBasePtr[offset1 + x - 1] * 32 +
                   (unsigned char)fromBasePtr[offset1 + x + 1] * 32 + (unsigned char)fromBasePtr[offset1 + x - 2] +
                   (unsigned char)fromBasePtr[offset1 + x + 2] + (unsigned char)fromBasePtr[offset1 + x - fromByteCount] * 32 +
                   (unsigned char)fromBasePtr[offset1 + x + fromByteCount] * 32);
            if (val > 65535)
                val = 0;
            blurBasePtr[offset2 + x] = val >> 8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[offset2 + x] = (unsigned char)fromBasePtr[offset1 + x - fromByteCount];
    }
#endif
    BlurToOffBlit();
}

void FilterFade(short filterWidth, short filterHeight) {
#if USE_GCD
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset = current * fromByteCount;
        int x;
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset + x])
                fromBasePtr[offset + x]--;
        }
    });
#else
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset1 + x])
                fromBasePtr[offset1 + x]--;
        }
        offset1 += fromByteCount;
    }
#endif
}

void FilterKaleidoscope(short filterWidth, short filterHeight) {
    long xe, ye;
    unsigned short x, y;
    long offset1, offset2, offset3;

    xe = filterWidth / 2;
    ye = filterHeight / 2;
    for (y = 0; y < filterHeight; y++) {
        offset1 = y * blurByteCount;
        offset2 = y * fromByteCount;
        offset3 = (filterHeight - (y + 1)) * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            blurBasePtr[offset1 + x] =
                ((unsigned char)fromBasePtr[offset2 + x] | (unsigned char)fromBasePtr[offset2 + (filterWidth - (x + 1))] |
                 (unsigned char)fromBasePtr[offset3 + x] | (unsigned char)fromBasePtr[offset3 + (filterWidth - (x + 1))]);
        }
    }
    BlurToOffBlit();
    // then blur
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = (unsigned char)fromBasePtr[x + fromByteCount];
    }
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 100 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[offset1 + x] = (unsigned char)fromBasePtr[offset1 + x - fromByteCount];
    }
}

void FilterMelt(short filterWidth, short filterHeight) {
    long xe;
    unsigned short x, y;
    long offset1, offset2;

    x = RandNum(1, filterWidth - 1);
    xe = RandNum(1, filterWidth - 1);
    if (x > xe) {
        y = x;
        x = xe;
        xe = y;
    }
    y = RandNum(2, filterHeight - 4);
    xe++;
    xe -= x;
    offset1 = (blurByteCount * (filterHeight - 1)) + x;
    offset2 = offset1 - fromByteCount;
    for (x = filterHeight; x > y; x--) {   // slide x,y - xe,xe down one pixel
        BlockMoveData(fromBasePtr + offset2, fromBasePtr + offset1, xe);
        offset1 -= fromByteCount;
        offset2 -= fromByteCount;
    }
    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {   // dim everything
        for (x = 0; x < filterWidth; x++) {
            if (fromBasePtr[offset1 + x])
                fromBasePtr[offset1 + x]--;
        }
        offset1 += fromByteCount;
    }
}

void FilterWatercolor(short filterWidth, short filterHeight) {
    long offset1, offset2;

    int x;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x] = (unsigned char)fromBasePtr[x + fromByteCount];
    }
#if USE_GCD
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = fromByteCount * (current + 1);
        long offset2 = blurByteCount * (current + 1);
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset1 + 1];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        int x;
        for (x = 1; x < filterWidth - 1; x++) {
            blurBasePtr[x + offset2] =
                ((unsigned char)fromBasePtr[x + offset1] * 252 + (unsigned char)fromBasePtr[x + offset1 - 1] +
                 (unsigned char)fromBasePtr[x + offset1 + 1] + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount]) >>
                8;
        }
    });
    offset1 = fromByteCount * (filterHeight - 1);
    offset2 = blurByteCount * (filterHeight - 1);
#else
    int y;
    offset1 = fromByteCount;
    offset2 = blurByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        blurBasePtr[offset2] = (unsigned char)fromBasePtr[offset1 + 1];
        blurBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            blurBasePtr[x + offset2] =
                ((unsigned char)fromBasePtr[x + offset1] * 252 + (unsigned char)fromBasePtr[x + offset1 - 1] +
                 (unsigned char)fromBasePtr[x + offset1 + 1] + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount]) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += blurByteCount;
    }
#endif
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x + offset2] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
    BlurToOffBlit();
}

void FilterShimmer(short filterWidth, short filterHeight) {
    long offset1, offset2;

    int x;
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x] = RandNum(1, 9);
    }
#if USE_GCD
    dispatch_apply(filterHeight - 2, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = blurByteCount * (current + 1);
        long offset2 = fromByteCount * (current + 1);
        blurBasePtr[offset1] = (unsigned char)fromBasePtr[offset2 + 1];
        blurBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        int x;
        for (x = 1; x < filterWidth - 1; x++) {
            switch (blurBasePtr[x]) {
                case 1: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount - 1]; break;
                case 2: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount]; break;
                case 3: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount + 1]; break;
                case 4: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - 1]; break;
                case 5: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + 1]; break;
                case 6: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount - 1]; break;
                case 7: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount]; break;
                case 8: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount + 1]; break;
            }
            blurBasePtr[x + offset1] =
                (unsigned char)(((unsigned char)blurBasePtr[x + offset1] + (unsigned char)fromBasePtr[x + offset2]) / 2);
        }
    });
    offset1 = blurByteCount * (filterHeight - 1);
    offset2 = fromByteCount * (filterHeight - 1);
#else
    offset1 = blurByteCount;
    offset2 = fromByteCount;
    int y;
    for (y = 1; y < filterHeight - 1; y++) {
        blurBasePtr[offset1] = (unsigned char)fromBasePtr[offset2 + 1];
        blurBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            switch (blurBasePtr[x]) {
                case 1: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount - 1]; break;
                case 2: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount]; break;
                case 3: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - fromByteCount + 1]; break;
                case 4: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 - 1]; break;
                case 5: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + 1]; break;
                case 6: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount - 1]; break;
                case 7: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount]; break;
                case 8: blurBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset2 + fromByteCount + 1]; break;
            }
            blurBasePtr[x + offset1] =
                (unsigned char)(((unsigned char)blurBasePtr[x + offset1] + (unsigned char)fromBasePtr[x + offset2]) / 2);
        }
        offset1 += blurByteCount;
        offset2 += fromByteCount;
    }
#endif
    for (x = 0; x < filterWidth; x++) {
        blurBasePtr[x] = (unsigned char)blurBasePtr[x + blurByteCount];
        blurBasePtr[x + offset1] = (unsigned char)blurBasePtr[x + offset1 - blurByteCount];
    }
    BlurToOffBlit();
}

void FilterSplitHoriz(short filterWidth, short filterHeight) {
    unsigned short xe, x, y;
    long offset1;

    xe = filterWidth / 2;
    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < xe; x++) {
            fromBasePtr[offset1 + x] = (unsigned char)fromBasePtr[offset1 + x + 1];
        }
        for (x = filterWidth; x > xe; x--) {
            fromBasePtr[offset1 + x] = (unsigned char)fromBasePtr[offset1 + x - 1];
        }
        offset1 += fromByteCount;
    }
    // then blur
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x] = (unsigned char)fromBasePtr[x + fromByteCount];
    }
    offset1 = fromByteCount;
    for (y = 1; y < filterHeight - 1; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[x + offset1] =
                ((unsigned char)fromBasePtr[x + offset1] * 100 + (unsigned char)fromBasePtr[x + offset1 - 1] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + 1] * 32 + (unsigned char)fromBasePtr[x + offset1 - fromByteCount] * 32 +
                 (unsigned char)fromBasePtr[x + offset1 + fromByteCount] * 32) >>
                8;
        }
        offset1 += fromByteCount;
    }
    for (x = 0; x < filterWidth; x++) {
        fromBasePtr[x + offset1] = (unsigned char)fromBasePtr[x + offset1 - fromByteCount];
    }
}

void FilterSplitVert(short filterWidth, short filterHeight) {
    long ye;
    unsigned short x, y;
    long offset1, offset2;

    ye = filterHeight / 2;
    offset1 = 0;
    offset2 = fromByteCount;
    for (y = 1; y <= ye; y++) {
        fromBasePtr[offset1] = (unsigned char)fromBasePtr[offset1 + 1];
        fromBasePtr[offset1 + filterWidth - 1] = (unsigned char)fromBasePtr[offset1 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[offset1 + x] =
                ((unsigned char)fromBasePtr[offset1 + x] * 32 + (unsigned char)fromBasePtr[offset2 + x] * 64 +
                 (unsigned char)fromBasePtr[offset2 + x + 1] * 64 + (unsigned char)fromBasePtr[offset2 + x - 1] * 64) >>
                8;
        }
        offset1 += fromByteCount;
        offset2 += fromByteCount;
    }
    fromBasePtr[offset1] = 0;
    fromBasePtr[offset1 + filterWidth - 1] = 0;
    fromBasePtr[offset2] = 0;
    fromBasePtr[offset2 + filterWidth - 1] = 0;
    offset1 = (filterHeight - 2) * fromByteCount;
    offset2 = offset1 + fromByteCount;
    for (y = filterHeight - 2; y > ye; y--) {
        fromBasePtr[offset2] = (unsigned char)fromBasePtr[offset2 + 1];
        fromBasePtr[offset2 + filterWidth - 1] = (unsigned char)fromBasePtr[offset2 + filterWidth - 2];
        for (x = 1; x < filterWidth - 1; x++) {
            fromBasePtr[offset2 + x] =
                ((unsigned char)fromBasePtr[offset2 + x] * 32 + (unsigned char)fromBasePtr[offset1 + x] * 64 +
                 (unsigned char)fromBasePtr[offset1 + x + 1] * 64 + (unsigned char)fromBasePtr[offset1 + x - 1] * 64) >>
                8;
        }
        offset1 -= fromByteCount;
        offset2 -= fromByteCount;
    }
    offset1 = ye * fromByteCount;
    offset2 = offset1 + fromByteCount;
    for (x = 1; x < filterWidth - 1; x++) {
        fromBasePtr[x + offset1] =
            ((unsigned char)fromBasePtr[offset1 + x + 1] * 32 + (unsigned char)fromBasePtr[offset1 + x - 1] * 32 +
             (unsigned char)fromBasePtr[offset2 + x - 1] * 64 + (unsigned char)fromBasePtr[offset2 + x + 1] * 64 +
             (unsigned char)fromBasePtr[offset2 + x] * 16 + (unsigned char)fromBasePtr[offset1 + x] * 32) >>
            8;
        fromBasePtr[x + offset2] =
            ((unsigned char)fromBasePtr[offset2 + x + 1] * 32 + (unsigned char)fromBasePtr[offset2 + x - 1] * 32 +
             (unsigned char)fromBasePtr[offset1 + x - 1] * 64 + (unsigned char)fromBasePtr[offset1 + x + 1] * 64 +
             (unsigned char)fromBasePtr[offset1 + x] * 16 + (unsigned char)fromBasePtr[offset2 + x] * 32) >>
            8;
    }
}

void FilterErase(short filterWidth, short filterHeight) {
#if USE_GCD
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        long offset1 = fromByteCount * current;
        memset(fromBasePtr + offset1, 0, filterWidth);
    });
#else
    unsigned short x, y;
    long offset1;

    offset1 = 0;
    for (y = 0; y < filterHeight; y++) {
        for (x = 0; x < filterWidth; x++) {
            fromBasePtr[offset1 + x] = 0;
        }
        offset1 += fromByteCount;
    }
#endif
}

void CompileCustomFilter(void) {
    short x, y, c;

    c = 0;
    numFilterPoints = 0;

    for (y = 0; y < 7; y++) {
        for (x = 0; x < 7; x++) {
            if (curSet.customfilter.fVal[c] > 0) {
                filterWeight[numFilterPoints] = curSet.customfilter.fVal[c];
                filterOffset[numFilterPoints] = ((y - 6) * fromByteCount) + (x - 3);
                numFilterPoints++;
            }
            c++;
        }
    }
}

void DoCustomFilter(void) {
    CompileCustomFilter();
    int filterWidth = prefs.winxsize;
    int filterHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        filterWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        filterHeight = (prefs.winysize / 2) + 1;
#if USE_GCD
    dispatch_apply(filterHeight, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
        int y = current;
        long offset1 = y * blurByteCount;
        long offset2 = y * fromByteCount;
        int x;
        for (x = 0; x < filterWidth; x++) {
            long accum = 0;
            if (x < 3 || x > (filterWidth - 4) || y < 3 || y > (filterHeight - 4)) {   // Do unaccelerated
                char c = 0;
                int cy;
                for (cy = y - 3; cy < y + 4; cy++) {
                    offset2 = cy * fromByteCount;
                    int cx;
                    for (cx = x - 3; cx < x + 4; cx++) {
                        if (curSet.customfilter.fVal[c]) {
                            if (cx >= 0 && cx < filterWidth && cy >= 0 && cy < filterHeight) {
                                accum += ((unsigned char)fromBasePtr[offset2 + cx] * curSet.customfilter.fVal[c]);
                            }
                        }
                        c++;
                    }
                }
            } else {   // use compiled filter list
                int i;
                for (i = 0; i < numFilterPoints; i++) {
                    accum += ((unsigned char)fromBasePtr[offset2 + x + filterOffset[i]] * filterWeight[i]);
                }
            }
            if (accum) {
                blurBasePtr[offset1 + x] = (unsigned char)(accum / curSet.customfilter.target);
            } else {
                blurBasePtr[offset1 + x] = 0;
            }
        }
    });
#else
    long accum, offset1, offset2;
    char c;
    short i, x, y, cx, cy;
    for (y = 0; y < filterHeight; y++) {
        offset1 = y * blurByteCount;
        offset2 = y * fromByteCount;
        for (x = 0; x < filterWidth; x++) {
            accum = 0;
            if (x < 3 || x > (filterWidth - 4) || y < 3 || y > (filterHeight - 4)) {   // Do unaccelerated
                c = 0;
                for (cy = y - 3; cy < y + 4; cy++) {
                    offset2 = cy * fromByteCount;
                    for (cx = x - 3; cx < x + 4; cx++) {
                        if (curSet.customfilter.fVal[c]) {
                            if (cx >= 0 && cx < filterWidth && cy >= 0 && cy < filterHeight) {
                                accum += ((unsigned char)fromBasePtr[offset2 + cx] * curSet.customfilter.fVal[c]);
                            }
                        }
                        c++;
                    }
                }
            } else {   // use compiled filter list
                for (i = 0; i < numFilterPoints; i++) {
                    accum += ((unsigned char)fromBasePtr[offset2 + x + filterOffset[i]] * filterWeight[i]);
                }
            }
            if (accum) {
                blurBasePtr[offset1 + x] = (unsigned char)(accum / curSet.customfilter.target);
            } else {
                blurBasePtr[offset1 + x] = 0;
            }
        }
    }
#endif
    BlurToOffBlit();
}

#define ID_EC_OK 1
#define ID_EC_CANCEL 2
#define ID_EC_SAVE 3
#define ID_EC_LOAD 4
#define ID_EC_FIRST 5
#define ID_EC_LAST 53
#define ID_EC_TARGET 54
#define ID_EC_EQUALS 56
#define ID_EC_TOTAL 57
#define ID_EC_APPLY 59

void EditCustomFilter(void) {
    Rect myRect;
    Boolean dialogDone;
    DialogPtr theDialog;
    short itemHit, temp, c;
    ControlHandle ctrl;
    long number;
    Str255 tempStr;
    struct setinformation backup;

    StopAutoChanges();
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    BlockMoveData(&curSet.customfilter, &editfilter, sizeof(struct pixeltoyfilter));
    EnsureSystemPalette();
    theDialog = GetNewDialog(DLG_CUSTOMFILTER, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    if (prefs.windowPosition[WINCUSTFILTER].h != 0 || prefs.windowPosition[WINCUSTFILTER].v != 0) {
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINCUSTFILTER].h, prefs.windowPosition[WINCUSTFILTER].v, false);
    }

    SetPortDialogPort(theDialog);
    ResetCursor();

    // set matrix values
    for (c = ID_EC_FIRST; c <= ID_EC_LAST; c++) {
        temp = c - ID_EC_FIRST;
        NumToString(editfilter.fVal[temp], tempStr);
        if (editfilter.fVal[temp] == 0)
            tempStr[0] = 0;
        GetDialogItemAsControl(theDialog, c, &ctrl);
        SetDialogItemText((Handle)ctrl, tempStr);
    }
    // set target
    NumToString(editfilter.target, tempStr);
    GetDialogItemAsControl(theDialog, ID_EC_TARGET, &ctrl);
    SetDialogItemText((Handle)ctrl, tempStr);
    ShowFilterTotal(theDialog, ID_EC_TOTAL);
    SelectDialogItemText(theDialog, 29, 0, 32767);    // select center pixel
    ForceOnScreen(GetDialogWindow(theDialog));
    // Disable Apply button
    GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
    DeactivateControl(ctrl);
    ShowWindow(GetDialogWindow(theDialog));
    SetDialogDefaultItem(theDialog, ID_EC_OK);
    dialogDone = false;
    ConfigureFilter(ID_EC_OK, ID_EC_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_EC_OK:
                BlockMoveData(&editfilter, &curSet.customfilter, sizeof(struct pixeltoyfilter));
                CurSetTouched();
                dialogDone = true;
                break;
            case ID_EC_CANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case ID_EC_LOAD:
                if (DoLoadFilterDialog()) {
                    // show new values
                    for (c = ID_EC_FIRST; c <= ID_EC_LAST; c++) {
                        temp = c - ID_EC_FIRST;
                        NumToString(editfilter.fVal[temp], tempStr);
                        if (editfilter.fVal[temp] == 0)
                            tempStr[0] = 0;
                        GetDialogItemAsControl(theDialog, c, &ctrl);
                        SetDialogItemText((Handle)ctrl, tempStr);
                    }
                    // show new target
                    NumToString(editfilter.target, tempStr);
                    GetDialogItemAsControl(theDialog, ID_EC_TARGET, &ctrl);
                    SetDialogItemText((Handle)ctrl, tempStr);
                    // show new total
                    ShowFilterTotal(theDialog, ID_EC_TOTAL);
                    SelectDialogItemText(theDialog, 29, 0, 32767);    // select center pixel
                    // Activate the Apply button
                    GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
                    ActivateControl(ctrl);
                }
                break;
            case ID_EC_SAVE: DoSaveFilterDialog(); break;
            case ID_EC_EQUALS:    // set target = total
                number = ShowFilterTotal(theDialog, ID_EC_TOTAL);
                if (editfilter.target != number) {
                    editfilter.target = number;
                    NumToString(editfilter.target, tempStr);
                    GetDialogItemAsControl(theDialog, ID_EC_TARGET, &ctrl);
                    SetDialogItemText((Handle)ctrl, tempStr);
                    ShowFilterTotal(theDialog, ID_EC_TOTAL);
                    SelectDialogItemText(theDialog, ID_EC_TARGET, 0, 32767);
                    // Activate Apply button
                    GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
                    if (!IsControlActive(ctrl))
                        ActivateControl(ctrl);
                }
                break;
            case ID_EC_APPLY:
                BlockMoveData(&editfilter, &curSet.customfilter, sizeof(struct pixeltoyfilter));
                // Disable Apply button
                GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
                DeactivateControl(ctrl);
                break;
            case ID_EC_TARGET:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, tempStr);
                StringToNum(tempStr, &number);
                if (editfilter.target != number) {
                    if (number < 0) {
                        editfilter.target = 0;
                        SetDialogItemText((Handle)ctrl, "\p0");
                    }
                    editfilter.target = number;
                    ShowFilterTotal(theDialog, ID_EC_TOTAL);
                    // Activate the Apply button
                    GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
                    if (!IsControlActive(ctrl))
                        ActivateControl(ctrl);
                }
                break;
            default:
                if (itemHit >= ID_EC_FIRST && itemHit <= ID_EC_LAST) {
                    GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                    GetDialogItemText((Handle)ctrl, tempStr);
                    StringToNum(tempStr, &number);
                    if (number < 1) {
                        number = 0;
                        SetDialogItemText((Handle)ctrl, "\p");
                    }
                    if (number > 255) {
                        number = 255;
                        SetDialogItemText((Handle)ctrl, "\p255");
                    }
                    if (editfilter.fVal[itemHit - ID_EC_FIRST] != (short)number) {
                        editfilter.fVal[itemHit - ID_EC_FIRST] = (short)number;
                        ShowFilterTotal(theDialog, ID_EC_TOTAL);
                        // Activate the Apply button
                        GetDialogItemAsControl(theDialog, ID_EC_APPLY, &ctrl);
                        if (!IsControlActive(ctrl))
                            ActivateControl(ctrl);
                    }
                }
                break;
        }
    }
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINCUSTFILTER].h = myRect.left;
    prefs.windowPosition[WINCUSTFILTER].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINCUSTFILTER]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen) { /*MyShowMenuBar();*/
        MyHideMenuBar();
    }
}

Boolean DoLoadFilterDialog(void) {
    OSErr err;
    FSSpec fss;
    Boolean result;

    result = false;
    ResetCursor();
    err = CocoaChooseFile(CFSTR("ChooseFilterFile"), CFSTR("CFLT"), &fss);
    if (noErr == err) {
        LoadFilter(fss);
        result = true;
    }
    return result;
}

void LoadFilter(FSSpec fss) {
    unsigned char version;
    short refNum;
    long inOutCount, recordSize;
    OSErr err;
    CursHandle watch;
    Ptr tempPtr;

    watch = GetCursor(watchCursor);
    SetCursor(*watch);
    err = FSpOpenDF(&fss, fsRdWrPerm, &refNum);
    if (!err)
        err = GetEOF(refNum, &inOutCount);
    if (!err) {
        tempPtr = NewPtrClear(inOutCount);
        if (tempPtr == nil)
            StopError(64, memFullErr);
    }
    if (!err)
        err = FSRead(refNum, &inOutCount, tempPtr);
    if (!err)
        err = FSClose(refNum);
    if (!err) {
        version = tempPtr[0];
        if (version != FILTERVERSION) {
            DoStandardAlert(kAlertNoteAlert, 17);
        } else {
            recordSize = (tempPtr[1] * 256) + (unsigned char)tempPtr[2];
            if (recordSize > sizeof(struct pixeltoyfilter))
                recordSize = sizeof(struct pixeltoyfilter);
            BlockMoveData(tempPtr + 3, &editfilter, recordSize);
            RemapFilterBtoN(&editfilter);
        }
    }
    DisposePtr(tempPtr);
    ResetCursor();
    if (err)
        StopError(62, err);
}

Boolean DoSaveFilterDialog(void) {
    Boolean result = false;

    ResetCursor();
    FSSpec fss;
    // default filename should be "PixelToy Filter"
    OSErr err =
        CocoaSaveFile(CFSTR("SaveFilterTitle"), CFSTR("SaveFilterMessage"), CFSTR("cflt"), (OSType)'BtP3', (OSType)'CFLT', &fss);
    if (noErr == err) {
        result = true;
        SaveFilter(fss);
    }
    return result;
}

void SaveFilter(FSSpec fss) {
    short refNum;
    long inOutCount;
    OSErr err;
    CursHandle watch;
    Ptr outPtr;

    watch = GetCursor(watchCursor);
    SetCursor(*watch);
    err = FSpOpenDF(&fss, fsRdWrPerm, &refNum);
    inOutCount = sizeof(struct pixeltoyfilter);
    outPtr = NewPtr(inOutCount + 3);
    outPtr[0] = FILTERVERSION;
    outPtr[1] = (unsigned char)(inOutCount / 256);
    outPtr[2] = (unsigned char)(inOutCount & 0xFF);
    RemapFilterNtoB(&editfilter);
    BlockMoveData(&editfilter, outPtr + 3, inOutCount);
    RemapFilterBtoN(&editfilter);
    inOutCount += 3;
    err = FSWrite(refNum, &inOutCount, outPtr);    // write filter
    if (err)
        StopError(63, err);
    err = FSClose(refNum);
    DisposePtr(outPtr);
    ResetCursor();
}

long ShowFilterTotal(DialogPtr theDialog, short item) {
    short c;
    long percentage, total = 0;
    Str255 out, str;
    ControlHandle ctrl;

    for (c = 0; c < 49; c++) {
        total += editfilter.fVal[c];
    }
    NumToString(total, out);
    out[++out[0]] = ' ';
    out[++out[0]] = '(';
    percentage = (total * 100) / editfilter.target;
    NumToString(percentage, str);
    BlockMoveData(str + 1, out + out[0] + 1, str[0]);
    out[0] += str[0];
    out[++out[0]] = '%';
    if (percentage > 100)
        out[++out[0]] = '!';
    out[++out[0]] = ')';
    GetDialogItemAsControl(theDialog, item, &ctrl);
    SetDialogItemText((Handle)ctrl, out);
    return total;
}

void SetDefaultCustomFilter(void) {
    short c;
    for (c = 0; c < 49; c++) {
        curSet.customfilter.fVal[c] = 0;
    }
    curSet.customfilter.fVal[24] = 40;
    curSet.customfilter.fVal[30] = 10;
    curSet.customfilter.fVal[31] = 32;
    curSet.customfilter.fVal[32] = 10;
    curSet.customfilter.fVal[38] = 20;
    curSet.customfilter.fVal[45] = 10;
    curSet.customfilter.target = 120;
}

void BlurToOffBlit(void) {
    if (LockAllWorlds()) {
        if (blurByteCount == fromByteCount)
            BlockMoveData(blurBasePtr, fromBasePtr, prefs.winysize * blurByteCount);
        else {
#if USE_GCD
            dispatch_apply(prefs.winysize, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(size_t current) {
                BlockMoveData(blurBasePtr + (blurByteCount * current), fromBasePtr + (fromByteCount * current), fromByteCount);
            });
#else
            register Ptr toCopy, fromCopy;
            register short ypos;

            if (LockAllWorlds()) {
                fromCopy = blurBasePtr;
                toCopy = fromBasePtr;

                ypos = prefs.winysize;

                while (ypos > 0) {
                    BlockMoveData(fromCopy, toCopy, fromByteCount);
                    fromCopy += blurByteCount;
                    toCopy += fromByteCount;
                    ypos--;
                }
            }
#endif
        }
    }
    UnlockAllWorlds();
}

void DoMirroring(void) {
    register Ptr p, ps;
    short s, x, xe, y, ye;

    if (LockAllWorlds()) {
        if (curSet.verticalMirror) {
            xe = prefs.winxsize;
            ye = prefs.winysize;
            if (curSet.horizontalMirror)
                ye = (prefs.winysize / 2) + 1;
            p = fromBasePtr;
            for (y = 0; y < ye; y++) {
                x = (prefs.winxsize / 2) + 1;
                s = x - 1;
                while (x < xe) {
                    p[x++] = (unsigned char)p[s--];
                }
                p += fromByteCount;
            }
        }
        if (curSet.horizontalMirror) {
            y = (prefs.winysize / 2) + 1;
            p = fromBasePtr + (y * fromByteCount);
            ps = p - fromByteCount;
            while (y < prefs.winysize) {
                BlockMoveData(ps, p, fromByteCount);
                y++;
                p += fromByteCount;
                ps -= fromByteCount;
            }
        }
        UnlockAllWorlds();
    }
}
