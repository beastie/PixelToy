// PixelToy Bitmap code

#include "PixelToyBitmap.h"

#include "Defs&Structs.h"
#include "CocoaBridge.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToySound.h"

extern struct setinformation curSet;
extern struct preferences prefs;
extern ModalFilterUPP DialogFilterProc;
extern PaletteHandle winPalette;
extern CTabHandle mainColorTable;
extern WindowPtr gMainWindow;
extern CGrafPtr offWorld;
extern Boolean gDone;
extern FSSpec curSetsSpec;
extern Ptr fromBasePtr;
extern short fromByteCount, gSysVersionMajor;

CGrafPtr bmWorld[MAXIMAGES], bmMaskWorld[MAXIMAGES];
Boolean iinvert[MAXIMAGES], minvert[MAXIMAGES];
FSSpec ifss[MAXIMAGES], mfss[MAXIMAGES];
Rect bitmapListRect, previewRect;
short imageRef[MAXIMAGES], maskRef[MAXIMAGES], idepth[MAXIMAGES];
char curImage;

Movie bmMovie[MAXIMAGES];
TimeValue movieTime[MAXIMAGES];

#define IDB_SAVE 1
#define IDB_CANCEL 2
#define IDB_LIST 3
#define IDB_NEW 4
#define IDB_DELETE 5
#define IDB_DUPLICATE 6
#define IDB_TOFRONT 7
#define IDB_TOBACK 8
#define IDB_PREVIEW 9
#define IDB_1BIT 10
#define IDB_8BIT 11
#define IDB_PREVIEWIMAGE 12
// primary image box = 13
#define IDB_IMAGEFILE 14
#define IDB_IMAGESELECT 15
#define IDB_INVERT 16
#define IDB_PROPORTION 17
#define IDB_ACTION 19
#define IDB_TRANSPARENT 20
#define IDB_MASKSUB 21
#define IDB_PREVIEWMASK 22
#define IDB_MASKFILE 23
#define IDB_MASKSELECT 24
#define IDB_INVERTMASK 25
#define IDB_BRIGHTSUB 26
#define IDB_FIXED 27
#define IDB_BRIGHTSLIDER 28
#define IDB_PULSING 29
#define IDB_REACTSOUND 30
#define IDB_ONTOP 31
#define IDB_RESETSIZE 32
#define IDB_SWAP0AND255 33

void ImageEditor(void) {
    DialogPtr theDialog;
    ControlHandle ctrl;
    Rect rect;
    Point point;
    Boolean dialogDone, storeDoodle /*, selected, shownAlert*/;
    FSSpec fss;
    OSErr err;
    ComponentInstance gi;
    float xdiv, ydiv;
    struct setinformation backup;
    struct PTImageGen bmTemp;
    short c, itemHit, temp, avail, thisOne, stageWidth, stageHeight;

    SaveContext();
    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;
    StopAutoChanges();
    storeDoodle = curSet.action[ACTDOODLE];
    curSet.action[ACTDOODLE] = false;
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    EnsureSystemPalette();
    for (c = MAXIMAGES - 1; c >= 0; c--) {
        if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
            rect = curSet.image[c].place;
            if (rect.right < rect.left + 200 || rect.bottom < rect.top + 200)
                ResetImageSize(c, 0);
        }
    }

    theDialog = GetNewDialog(DLG_IMAGEEDITOR, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    if (prefs.windowPosition[WINIMAGEOPTS].h != 0 || prefs.windowPosition[WINIMAGEOPTS].v != 0)
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINIMAGEOPTS].h, prefs.windowPosition[WINIMAGEOPTS].v, false);
    SetPortDialogPort(theDialog);
    ResetCursor();

    curImage = -1;
    avail = 0;
    for (c = MAXIMAGES - 1; c >= 0; c--) {
        if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
            avail++;
            curImage = c;
        }
    }
    if (avail > 15) {   // dim New button since we're all full
        GetDialogItemAsControl(theDialog, IDB_NEW, &ctrl);
        DeactivateControl(ctrl);
        GetDialogItemAsControl(theDialog, IDB_DUPLICATE, &ctrl);
        DeactivateControl(ctrl);
    }

    if (curImage == -1)
        curImage = CreateNewImage();
    Handle nullHandle = nil;
    GetDialogItem(theDialog, IDB_LIST, &temp, &nullHandle, &bitmapListRect);
    GetDialogItem(theDialog, IDB_PREVIEW, &temp, &nullHandle, &previewRect);
    xdiv = stageWidth / (float)(previewRect.right - previewRect.left);
    ydiv = stageHeight / (float)(previewRect.bottom - previewRect.top);
    if (ydiv > xdiv) {    // make narrower
        previewRect.right = previewRect.left + (stageWidth / ydiv);
    } else {    // make shorter
        previewRect.top = previewRect.bottom - (stageHeight / xdiv);
    }

    ShowImageValues(theDialog, curImage);
    SetSliderActions(theDialog, DLG_IMAGEEDITOR);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    SetDialogDefaultItem(theDialog, IDB_SAVE);
    UpdateSliderDisplay(theDialog);
    DrawImageList(&bitmapListRect);
    PreviewImageParameters(&previewRect);
    DrawCurrentImagePreviews(theDialog);
    dialogDone = false;
    ConfigureFilter(IDB_SAVE, IDB_CANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case IDB_SAVE:
                dialogDone = true;
                CurSetTouched();
                break;
            case IDB_CANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case IDB_NEW:
            case IDB_DUPLICATE:
                if (true) {
                    avail = 0;
                    for (c = 0; c < MAXIMAGES; c++) {
                        if (curSet.image[c].active != IMAGEACTIVE && curSet.image[c].active != MOVIEACTIVE)
                            avail++;
                    }
                    if (avail > 0) {
                        c = curImage;    // remember original for duplicate
                        curImage = CreateNewImage();
                        if (avail < 2) {   // dim New button since we're all full
                            GetDialogItemAsControl(theDialog, IDB_NEW, &ctrl);
                            DeactivateControl(ctrl);
                            GetDialogItemAsControl(theDialog, IDB_DUPLICATE, &ctrl);
                            DeactivateControl(ctrl);
                        }
                        if (itemHit == IDB_DUPLICATE) {
                            curSet.image[curImage] = curSet.image[c];
                            if (!OptionIsPressed()) {
                                OffsetRect(&curSet.image[curImage].place, 500, 500);
                            }
                        }
                    } else {
                        SysBeep(1);
                    }
                }
                break;
            case IDB_DELETE:
                avail = 0;
                for (c = 0; c < MAXIMAGES; c++) {
                    if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE)
                        avail++;
                }
                if (avail > 1) {
                    for (c = (curImage + 1); c < MAXIMAGES; c++) {
                        curSet.image[c - 1] = curSet.image[c];
                    }
                    curSet.image[MAXIMAGES - 1].active = false;
                    while (!(curSet.image[curImage].active == IMAGEACTIVE || curSet.image[curImage].active == MOVIEACTIVE)) {
                        curImage--;
                        if (curImage < 0)
                            curImage = MAXIMAGES - 1;
                    }
                    GetDialogItemAsControl(theDialog, IDB_NEW, &ctrl);
                    ActivateControl(ctrl);
                    GetDialogItemAsControl(theDialog, IDB_DUPLICATE, &ctrl);
                    ActivateControl(ctrl);
                } else {
                    DoStandardAlert(kAlertStopAlert, 23);
                }
                break;
            case IDB_TOFRONT:
                bmTemp = curSet.image[curImage];
                curSet.image[curImage].active = false;
                // eliminate emptiness
                c = 0;
                temp = c;
                while (c < MAXIMAGES) {
                    if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
                        if (c != temp) {
                            curSet.image[temp] = curSet.image[c];
                            curSet.image[c].active = false;
                        }
                        temp++;
                    }
                    c++;
                }
                // stick item at end of list
                curSet.image[temp] = bmTemp;
                curImage = temp;
                break;
            case IDB_TOBACK:
                bmTemp = curSet.image[curImage];
                curSet.image[curImage].active = false;
                // sort everything to end of list
                c = MAXIMAGES - 1;
                temp = c;
                while (c >= 0) {
                    if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
                        if (c != temp) {
                            curSet.image[temp] = curSet.image[c];
                            curSet.image[c].active = false;
                        }
                        temp--;
                    }
                    c--;
                }
                // stick item at beginning
                curSet.image[0] = bmTemp;
                curImage = 0;
                // eliminate emptiness
                c = 0;
                temp = c;
                while (c < MAXIMAGES) {
                    if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
                        if (c != temp) {
                            curSet.image[temp] = curSet.image[c];
                            curSet.image[c].active = false;
                        }
                        temp++;
                    }
                    c++;
                }
                break;
            case IDB_LIST:
                GetMouse(&point);
                temp = (point.v - bitmapListRect.top) / 15;
                avail = -1;
                thisOne = -1;
                for (c = 0; c < MAXIMAGES; c++) {
                    if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
                        avail++;
                        if (temp == avail)
                            thisOne = c;
                    }
                }
                if (thisOne != curImage && thisOne != -1)
                    curImage = thisOne;
                break;
            case IDB_PREVIEW: HandleBitmapPreviewImageClick(theDialog, &previewRect); break;
            case IDB_IMAGESELECT:
            case IDB_MASKSELECT:
                InitCursor();
                err = CocoaChooseFile(CFSTR("ChooseBitmapImageFile"), NULL, &fss);
                if (noErr == err) {
                    err = GetGraphicsImporterForFile(&fss, &gi);
                    if (noErr == err) {   // probably, get its bounds
                        err = GraphicsImportGetBoundsRect(gi, &rect);
                        if (!err)
                            curSet.image[curImage].active = IMAGEACTIVE;
                        CloseComponent(gi);
                    }
                    if (noErr == err) {
                        if (itemHit == IDB_IMAGESELECT) {
                            BlockMoveData(&fss, &curSet.image[curImage].imagefss, sizeof(FSSpec));
                            curSet.image[curImage].orgwidth = rect.right;
                            curSet.image[curImage].orgheight = rect.bottom;
                            ResetImageSize(curImage, 400);
                        } else {
                            BlockMoveData(&fss, &curSet.image[curImage].maskfss, sizeof(FSSpec));
                        }
                    }
                }
                break;
            case IDB_RESETSIZE:
                if (OptionIsPressed()) {
                    for (c = 0; c < MAXIMAGES; c++) {
                        if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE)
                            ResetImageSize(c, 0);
                    }
                } else {
                    ResetImageSize(curImage, 0);
                }
                break;
            case IDB_ONTOP: curSet.image[curImage].drawAfterActions = !curSet.image[curImage].drawAfterActions; break;
            case IDB_INVERT: curSet.image[curImage].invert = !curSet.image[curImage].invert; break;
            case IDB_SWAP0AND255:
                prefs.exchange0and255 = !prefs.exchange0and255;
                for (c = 0; c < MAXIMAGES; c++) {
                    if (curSet.image[c].depth == 8)
                        ResetBMWorld(c, SB_IMAGE);
                }
                break;
            case IDB_INVERTMASK: curSet.image[curImage].invertMask = !curSet.image[curImage].invertMask; break;
            case IDB_ACTION:
                GetDialogItemAsControl(theDialog, IDB_ACTION, &ctrl);
                curSet.image[curImage].action = GetControlValue(ctrl) - 1;
                break;
            case IDB_1BIT:
            case IDB_8BIT:
                if (itemHit == IDB_1BIT) {
                    curSet.image[curImage].depth = 1;
                } else {
                    curSet.image[curImage].depth = 8;
                }
                break;
            case IDB_FIXED: curSet.image[curImage].britemode = 0; break;
            case IDB_PULSING: curSet.image[curImage].britemode = 1; break;
            case IDB_REACTSOUND: curSet.image[curImage].britemode = 2; break;
            case IDB_TRANSPARENT: curSet.image[curImage].transparent = !curSet.image[curImage].transparent; break;
            case IDB_PROPORTION: prefs.imagesNotProportional = !prefs.imagesNotProportional; break;
        }
        SetPortDialogPort(theDialog);
        ShowImageValues(theDialog, curImage);
        DrawImageList(&bitmapListRect);
        PreviewImageParameters(&previewRect);
        UpdateSliderDisplay(theDialog);
        DrawCurrentImagePreviews(theDialog);
        UpdateCurrentSetNeedsSoundInputMessage();
    }
    curSet.action[ACTDOODLE] = storeDoodle;
    GetWindowPortBounds(GetDialogWindow(theDialog), &rect);
    prefs.windowPosition[WINIMAGEOPTS].h = rect.left;
    prefs.windowPosition[WINIMAGEOPTS].v = rect.top;
    LocalToGlobal(&prefs.windowPosition[WINIMAGEOPTS]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen)
        MyHideMenuBar();
}

void ShowImageValues(DialogPtr theDialog, short b) {
    ControlHandle ctrl;
    Str255 undefStr;

    if (!(curSet.image[b].active == IMAGEACTIVE || curSet.image[b].active == MOVIEACTIVE))
        return;

    GetDialogItemAsControl(theDialog, IDB_1BIT, &ctrl);
    SetControlValue(ctrl, curSet.image[b].depth == 1);

    GetDialogItemAsControl(theDialog, IDB_8BIT, &ctrl);
    SetControlValue(ctrl, curSet.image[b].depth == 8);

    GetIndString(undefStr, BASERES, 31);
    GetDialogItemAsControl(theDialog, IDB_IMAGEFILE, &ctrl);
    if (curSet.image[b].imagefss.name[0] > 0) {
        SetDialogItemText((Handle)ctrl, curSet.image[b].imagefss.name);
    } else {
        SetDialogItemText((Handle)ctrl, undefStr);
    }

    GetDialogItemAsControl(theDialog, IDB_INVERT, &ctrl);
    SetControlValue(ctrl, curSet.image[b].invert);

    GetDialogItemAsControl(theDialog, IDB_PROPORTION, &ctrl);
    SetControlValue(ctrl, !prefs.imagesNotProportional);

    GetDialogItemAsControl(theDialog, IDB_ONTOP, &ctrl);
    SetControlValue(ctrl, curSet.image[curImage].drawAfterActions);

    GetDialogItemAsControl(theDialog, IDB_ACTION, &ctrl);
    SetControlValue(ctrl, curSet.image[b].action + 1);

    GetDialogItemAsControl(theDialog, IDB_TRANSPARENT, &ctrl);
    SetControlValue(ctrl, curSet.image[b].transparent);

    GetDialogItemAsControl(theDialog, IDB_MASKSUB, &ctrl);
    if (curSet.image[b].depth == 8 && curSet.image[b].transparent) {
        if (!IsControlActive(ctrl))
            ActivateControl(ctrl);
    } else {
        if (IsControlActive(ctrl))
            DeactivateControl(ctrl);
    }

    GetDialogItemAsControl(theDialog, IDB_MASKFILE, &ctrl);
    if (curSet.image[b].maskfss.name[0] > 0) {
        SetDialogItemText((Handle)ctrl, curSet.image[b].maskfss.name);
    } else {
        SetDialogItemText((Handle)ctrl, undefStr);
    }

    GetDialogItemAsControl(theDialog, IDB_INVERTMASK, &ctrl);
    SetControlValue(ctrl, curSet.image[b].invertMask);

    GetDialogItemAsControl(theDialog, IDB_BRIGHTSUB, &ctrl);
    if (curSet.image[b].depth == 1) {
        if (!IsControlActive(ctrl))
            ActivateControl(ctrl);
    } else {
        if (IsControlActive(ctrl))
            DeactivateControl(ctrl);
    }

    GetDialogItemAsControl(theDialog, IDB_BRIGHTSLIDER, &ctrl);
    SetControlValue(ctrl, curSet.image[b].brightness);
    if (curSet.image[b].britemode == 0) {
        if (!IsControlActive(ctrl))
            ActivateControl(ctrl);
    } else {
        if (IsControlActive(ctrl))
            DeactivateControl(ctrl);
    }

    GetDialogItemAsControl(theDialog, IDB_FIXED, &ctrl);
    SetControlValue(ctrl, curSet.image[b].britemode == 0);

    GetDialogItemAsControl(theDialog, IDB_PULSING, &ctrl);
    SetControlValue(ctrl, curSet.image[b].britemode == 1);

    GetDialogItemAsControl(theDialog, IDB_REACTSOUND, &ctrl);
    SetControlValue(ctrl, curSet.image[b].britemode == 2);
}

void DrawCurrentImagePreviews(DialogPtr theDialog) {
    CGrafPtr previewWorld, off32World;
    ControlHandle ctrl;
    Rect rect1, rect2, iRect, pRect;
    Rect rect3, rect4;
    float ratio;
    short temp, which, useImage;
    RGBColor color;
    OSErr err;

    which = curImage;
    RefreshBMWorlds();
    GetDialogItem(theDialog, IDB_PREVIEWIMAGE, &temp, &ctrl, &rect1);
    GetDialogItem(theDialog, IDB_PREVIEWMASK, &temp, &ctrl, &rect2);
    pRect = rect1;
    OffsetRect(&pRect, -pRect.left, -pRect.top);
    err = NewGWorld(&previewWorld, 16, &pRect, nil, nil, 0);
    if (err)
        return;
    if (!LockPixels(GetGWorldPixMap(previewWorld))) {
        DisposeGWorld(previewWorld);
        return;
    }
    useImage = imageRef[which];
    GetPortBounds(bmWorld[useImage], &rect3);
    err = NewGWorld(&off32World, 32, &rect3, nil, nil, 0);
    if (!err)
        LockPixels(GetGWorldPixMap(off32World));
    SaveContext();
    SetGWorld(previewWorld, nil);
    color.red = color.green = color.blue = 49152;
    // Main image preview
    RGBForeColor(&color);
    PaintRect(&pRect);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (bmWorld[useImage]) {
        iRect = pRect;
        ratio = (float)rect3.right / (float)rect3.bottom;
        if (ratio > 1) {
            temp = (iRect.bottom - iRect.top) - ((iRect.right - iRect.left) / ratio);
            iRect.top += (temp / 2);
            iRect.bottom -= (temp / 2);
        }
        if (ratio < 1) {
            temp = (iRect.right - iRect.left) - ((iRect.bottom - iRect.top) * ratio);
            iRect.left += (temp / 2);
            iRect.right -= (temp / 2);
        }
        if (!err) {   // use off32World as interim
            GetPortBounds(off32World, &rect4);
            SetGWorld(off32World, nil);
            CopyBits(GetPortBitMapForCopyBits(bmWorld[useImage]), GetPortBitMapForCopyBits(off32World), &rect3, &rect4, srcCopy,
                     nil);
            SetGWorld(previewWorld, nil);
            CopyBits(GetPortBitMapForCopyBits(off32World), GetPortBitMapForCopyBits(previewWorld), &rect4, &iRect, srcCopy, nil);
        } else {   // go direct from bmWorld to previewWorld
            CopyBits(GetPortBitMapForCopyBits(bmWorld[useImage]), GetPortBitMapForCopyBits(previewWorld), &rect3, &iRect, srcCopy,
                     nil);
        }
    }
    SetPortDialogPort(theDialog);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(GetDialogPort(theDialog)), &pRect, &rect1, srcCopy,
             nil);
    // Mask image preview
    SetGWorld(previewWorld, nil);
    useImage = maskRef[which];
    RGBForeColor(&color);
    PaintRect(&pRect);
    ForeColor(blackColor);
    BackColor(whiteColor);
    if (curSet.image[which].depth == 8 && curSet.image[which].transparent && (bmMaskWorld[useImage])) {
        iRect = pRect;
        GetPortBounds(bmMaskWorld[useImage], &rect3);
        ratio = (float)rect3.right / (float)rect3.bottom;
        if (ratio > 1) {
            temp = (iRect.bottom - iRect.top) - ((iRect.right - iRect.left) / ratio);
            iRect.top += (temp / 2);
            iRect.bottom -= (temp / 2);
        }
        if (ratio < 1) {
            temp = (iRect.right - iRect.left) - ((iRect.bottom - iRect.top) * ratio);
            iRect.left += (temp / 2);
            iRect.right -= (temp / 2);
        }
        if (!err) {   // use off32World as interim
            GetPortBounds(off32World, &rect4);
            SetGWorld(off32World, nil);
            CopyBits(GetPortBitMapForCopyBits(bmMaskWorld[useImage]), GetPortBitMapForCopyBits(off32World), &rect3, &rect4, srcCopy,
                     nil);
            SetGWorld(previewWorld, nil);
            DrawThemeListBoxFrame(&rect1, kThemeStateActive);
            CopyBits(GetPortBitMapForCopyBits(off32World), GetPortBitMapForCopyBits(previewWorld), &rect4, &iRect, srcCopy, nil);
        } else {   // go direct from bmWorld to previewWorld
            CopyBits(GetPortBitMapForCopyBits(bmMaskWorld[useImage]), GetPortBitMapForCopyBits(previewWorld), &rect3, &iRect,
                     srcCopy, nil);
        }
    }
    SetPortDialogPort(theDialog);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(GetDialogPort(theDialog)), &pRect, &rect2, srcCopy,
             nil);
    RestoreContext();
    DisposeGWorld(previewWorld);
    if (!err)
        DisposeGWorld(off32World);
}

void SetLiveImageSliderValue(short item, short value) {
    short i;

    if (item == IDB_BRIGHTSLIDER) {
        curSet.image[curImage].brightness = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXIMAGES; i++) {
                if (curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE)
                    curSet.image[i].brightness = value;
            }
        }
    }
}

char CreateNewImage(void) {
    Rect defRect;
    PicHandle pict;
    short width, height, stageWidth, stageHeight;
    float xdiv, ydiv;
    char avail = 0, c, i;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;
    for (c = 0; c < MAXIMAGES; c++) {
        if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE)
            avail++;
    }
    if (avail < MAXIMAGES) {
        c = 0;
        while (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE)
            c++;
        curSet.image[c].active = IMAGEACTIVE;
        curSet.image[c].imagefss.name[0] = 0;
        curSet.image[c].action = 0;
        for (i = 0; i < 8; i++) {
            curSet.image[c].actionVar[i] = 0;
        }
        curSet.image[c].depth = 8;
        curSet.image[c].invert = false;
        curSet.image[c].transparent = false;
        curSet.image[c].drawAfterActions = false;
        // 1 bit options
        curSet.image[c].britemode = 0;
        curSet.image[c].brightness = 100;
        // 8 bit options
        curSet.image[c].maskfss.name[0] = 0;
        curSet.image[c].invertMask = false;
        // set up nice default size/placement
        SetDefaultImageAndMask(c);
        if (!curSet.image[c].imagefss.name[0]) {   // no default image file was available, use resource
            pict = GetPicture(BASERES + 4);
            if (pict) {
                defRect.left = EndianS16_BtoN((*pict)->picFrame.left);
                defRect.top = EndianS16_BtoN((*pict)->picFrame.top);
                defRect.right = EndianS16_BtoN((*pict)->picFrame.right);
                defRect.bottom = EndianS16_BtoN((*pict)->picFrame.bottom);
                ReleaseResource((Handle)pict);
                curSet.image[c].orgwidth = defRect.right;
                curSet.image[c].orgheight = defRect.bottom;
                xdiv = 16384.0 / (float)stageWidth;
                ydiv = 16384.0 / (float)stageHeight;
                width = defRect.right * xdiv;
                height = defRect.bottom * ydiv;
                curSet.image[c].place.left = (stageWidth * xdiv) / 2 - width / 2;
                curSet.image[c].place.right = curSet.image[c].place.left + width;
                curSet.image[c].place.top = (stageHeight * ydiv) / 2 - height / 2;
                curSet.image[c].place.bottom = curSet.image[c].place.top + height;
            } else {
                curSet.image[c].orgwidth = 32;
                curSet.image[c].orgheight = 32;
                SetRect(&curSet.image[c].place, 6192, 6192, 10192, 10192);
            }
        }
        return c;
    }
    return 0;
}

void SetDefaultImageAndMask(short i) {
    Str255 str;
    FSSpec fss;
    OSErr err;
    Rect rect;
    ComponentInstance gi;
    short width, height;

    GetIndString(str, BASERES, 39);
    err = FSMakeFSSpec(0, nil, str, &fss);
    if (!err) {
        err = GetGraphicsImporterForFile(&fss, &gi);
        if (!err)
            err = GraphicsImportGetBoundsRect(gi, &rect);
        CloseComponent(gi);
        if (!err) {   // got FSSpec and dimensions of default image, run with it
            BlockMoveData(&fss, &curSet.image[i].imagefss, sizeof(FSSpec));
            curSet.image[i].orgwidth = rect.right;
            curSet.image[i].orgheight = rect.bottom;
            ResetImageSize(i, 400);
            width = curSet.image[i].place.right - curSet.image[i].place.left;
            curSet.image[i].place.left = 8192 - (width / 2);
            curSet.image[i].place.right = 8192 + (width / 2);
            height = curSet.image[i].place.bottom - curSet.image[i].place.top;
            curSet.image[i].place.top = 8192 - (height / 2);
            curSet.image[i].place.bottom = 8192 + (height / 2);
        }
    }
    GetIndString(str, BASERES, 40);
    err = FSMakeFSSpec(0, nil, str, &fss);
    if (!err) {
        BlockMoveData(&fss, &curSet.image[i].maskfss, sizeof(FSSpec));
        curSet.image[i].transparent = true;
    }
}

void DrawImageList(Rect *drawRect) {
    Rect listRect, hiliteRect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short c, y;
    Str255 displayStr;
    RGBColor highlightColor;

    listRect.left = listRect.top = 0;
    listRect.right = (drawRect->right - drawRect->left);
    listRect.bottom = (drawRect->bottom - drawRect->top);
    GetPort(&curPort);
    GetGWorld(&curWorld, &curDevice);
    error = NewGWorld(&previewWorld, 16, &listRect, nil, nil, 0);
    if (error != noErr)
        StopError(33, error);
    if (!LockPixels(GetGWorldPixMap(previewWorld))) {
        DisposeGWorld(previewWorld);
        return;
    }
    SetGWorld(previewWorld, nil);
    BackColor(whiteColor);
    ForeColor(blackColor);
    TextMode(srcOr);
    EraseRect(&listRect);
    LMGetHiliteRGB(&highlightColor);
    if (highlightColor.red < 24576)
        highlightColor.red = 24576;
    if (highlightColor.green < 24576)
        highlightColor.green = 24576;
    if (highlightColor.blue < 24576)
        highlightColor.blue = 24576;
    y = 14;
    for (c = 0; c < MAXIMAGES; c++) {
        if (curSet.image[c].active == IMAGEACTIVE || curSet.image[c].active == MOVIEACTIVE) {
            if (curSet.image[c].imagefss.name[0] == 0) {
                GetIndString(displayStr, BASERES, 31);
            } else {
                BlockMoveData(curSet.image[c].imagefss.name, displayStr, curSet.image[c].imagefss.name[0] + 1);
                if (displayStr[displayStr[0] - 4] == '.')
                    displayStr[0] -= 5;    // trim .PICT .TIFF etc
                if (displayStr[displayStr[0] - 3] == '.')
                    displayStr[0] -= 4;    // trim .JPG .GIF etc
            }
            while (StringWidth(displayStr) > listRect.right - 6) {
                displayStr[0]--;
                displayStr[displayStr[0]] = 0xC9; // É
            }
            MoveTo(4, y);
            DrawString(displayStr);
            if (curImage == c) {
                hiliteRect.left = listRect.left + 2;
                hiliteRect.right = listRect.right - 2;
                hiliteRect.top = (y - 12);
                hiliteRect.bottom = hiliteRect.top + 15;
                RGBForeColor(&highlightColor);
                PenMode(adMin);
                PaintRect(&hiliteRect);
                ForeColor(blackColor);
                PenMode(srcCopy);
            }
            y += 15;
        }
    }
    FrameRect(&listRect);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &listRect, drawRect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void HandleBitmapPreviewImageClick(DialogPtr theDialog, Rect *rect) {
    Boolean resize = false;
    short i, sel = -1, stageWidth, stageHeight;
    Point start, point;
    Rect bRect;
    float xm, ym, f;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;
    GetMouse(&start);
    start.h -= rect->left;
    start.v -= rect->top;

    xm = 16384.0 / (float)(rect->right - rect->left);
    ym = 16384.0 / (float)(rect->bottom - rect->top);

    i = MAXIMAGES - 1;
    while (i >= 0 && sel == -1) {   // first try for a resize box
        if (curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE) {
            bRect = curSet.image[i].place;
            bRect.right /= xm;
            bRect.bottom /= ym;
            bRect.left = bRect.right - 5;
            bRect.top = bRect.bottom - 5;
            if (PtInRect(start, &bRect)) {
                sel = i;
                resize = true;
            }
        }
        i--;
    }
    if (sel == -1) {   // they didn't click a resize box, try whole boxes
        i = MAXIMAGES - 1;
        while (i >= 0 && sel == -1) {
            if (curSet.image[i].active == IMAGEACTIVE) {
                bRect = curSet.image[i].place;
                bRect.left /= xm;
                bRect.right /= xm;
                bRect.top /= ym;
                bRect.bottom /= ym;
                if (PtInRect(start, &bRect)) {
                    sel = i;
                    resize = false;
                }
            }
            i--;
        }
    }
    if (sel == -1)
        return;
    if (sel != curImage) {
        curImage = sel;
        SetPortDialogPort(theDialog);
        DrawImageList(&bitmapListRect);
        ShowImageValues(theDialog, curImage);
        DrawCurrentImagePreviews(theDialog);
        PreviewImageParameters(&previewRect);
        UpdateSliderDisplay(theDialog);
    }
    bRect = curSet.image[curImage].place;
    while (StillDown()) {
        GetMouse(&point);
        if (PtInRect(point, rect)) {
            point.h -= rect->left;
            point.v -= rect->top;
            curSet.image[curImage].place = bRect;
            if (resize) {
                curSet.image[curImage].place.right += ((point.h - start.h) * xm);
                if (curSet.image[curImage].place.right < bRect.left + (5 * xm))
                    curSet.image[curImage].place.right = bRect.left + 5 * xm;
                if (!prefs.imagesNotProportional) {
                    f = (float)curSet.image[curImage].orgwidth / (float)curSet.image[curImage].orgheight;
                    f /= ((float)stageWidth / (float)stageHeight);
                    curSet.image[curImage].place.bottom =
                        curSet.image[curImage].place.top +
                        (curSet.image[curImage].place.right - curSet.image[curImage].place.left) / f;
                } else {
                    curSet.image[curImage].place.bottom += ((point.v - start.v) * ym);
                    if (curSet.image[curImage].place.bottom < bRect.top + (5 * ym))
                        curSet.image[curImage].place.bottom = bRect.top + 5 * ym;
                }
            } else {
                OffsetRect(&curSet.image[curImage].place, (point.h - start.h) * xm, (point.v - start.v) * ym);
            }
            PreviewImageParameters(&previewRect);
        }
    }
}

void PreviewImageParameters(Rect *rect) {
    Rect previewRect, bRect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short i, genevaFnum;
    float xm, ym;
    RGBColor grey, darkGrey;
    Str32 str;

    previewRect.left = previewRect.top = 0;
    previewRect.right = (rect->right - rect->left);
    previewRect.bottom = (rect->bottom - rect->top);
    GetPort(&curPort);
    GetGWorld(&curWorld, &curDevice);
    error = NewGWorld(&previewWorld, 16, &previewRect, nil, nil, 0);
    if (error != noErr)
        StopError(33, error);
    if (!LockPixels(GetGWorldPixMap(previewWorld))) {
        DisposeGWorld(previewWorld);
        return;
    }
    SetGWorld(previewWorld, nil);
    BackColor(blackColor);
    EraseRect(&previewRect);
    grey.red = grey.green = grey.blue = 32767;
    darkGrey.red = darkGrey.green = darkGrey.blue = 16384;
    xm = 16384.0 / (float)previewRect.right;
    ym = 16384.0 / (float)previewRect.bottom;
    GetFNum("\pGeneva", &genevaFnum);
    TextFont(genevaFnum);
    TextSize(9);
    TextMode(srcOr);
    for (i = 0; i < MAXIMAGES; i++) {
        if (curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE) {
            if (i == curImage) {
                ForeColor(whiteColor);
            } else {
                RGBForeColor(&grey);
            }
            bRect = curSet.image[i].place;
            bRect.left /= xm;
            bRect.top /= ym;
            bRect.right /= xm;
            bRect.bottom /= ym;
            if (curSet.image[i].imagefss.name[0] == 0) {
                GetIndString(str, BASERES, 31);
            } else {
                BlockMoveData(curSet.image[i].imagefss.name, str, curSet.image[i].imagefss.name[0] + 1);
                if (str[str[0] - 4] == '.')
                    str[0] -= 5;    // trim .PICT .TIFF etc
                if (str[str[0] - 3] == '.')
                    str[0] -= 4;    // trim .JPG .GIF etc
            }
            while (StringWidth(str) > (bRect.right - bRect.left) - 2 && str[0] > 1) {
                str[--str[0]] = 0xC9; // É
            }
            if (str[0] > 1) {
                MoveTo(bRect.left + (bRect.right - bRect.left) / 2 - (StringWidth(str) / 2),
                       bRect.top + (bRect.bottom - bRect.top) / 2 + 3);
                DrawString(str);
            }
            FrameRect(&bRect);
            bRect.left = bRect.right - 5;
            bRect.top = bRect.bottom - 5;
            PaintRect(&bRect);
        }
    }
    ForeColor(blackColor);
    BackColor(whiteColor);
    FrameRect(&previewRect);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curWorld), &previewRect, rect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void ResetImageSize(short i, short margin) {
    float xdiv, ydiv;
    long width, height;
    short stageWidth, stageHeight;

    stageWidth = prefs.winxsize;
    stageHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        stageWidth = (prefs.winxsize / 2) + 2;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        stageHeight = (prefs.winysize / 2) + 2;

    xdiv = 16384.0 / (float)stageWidth;
    ydiv = 16384.0 / (float)stageHeight;
    width = curSet.image[i].orgwidth * xdiv;
    height = curSet.image[i].orgheight * ydiv;

    if (width > (16384 - margin * 2)) {
        xdiv = (float)width / (float)height;
        width = (16384 - margin * 2);
        height = width / xdiv;
    }
    if (height > (16384 - margin * 2)) {
        ydiv = (float)width / (float)height;
        height = (16384 - margin * 2);
        width = height * ydiv;
    }

    curSet.image[i].place.right = curSet.image[i].place.left + width;
    curSet.image[i].place.bottom = curSet.image[i].place.top + height;

    if (curSet.image[i].place.left < margin)
        OffsetRect(&curSet.image[i].place, -curSet.image[i].place.left + margin, 0);
    if (curSet.image[i].place.top < margin)
        OffsetRect(&curSet.image[i].place, 0, -curSet.image[i].place.top + margin);
    if (curSet.image[i].place.right > 16384 - margin)
        OffsetRect(&curSet.image[i].place, (16384 - margin) - curSet.image[i].place.right, 0);
    if (curSet.image[i].place.bottom > 16384 - margin)
        OffsetRect(&curSet.image[i].place, 0, (16384 - margin) - curSet.image[i].place.bottom);
}

#define BB_STATIONARY 0
#define BB_JITTER 1
#define BB_ZEROG 2
#define BB_GRAVITY 3
#define BB_WANDER 4
#define BB_MOUSE 5
#define BB_VIBRATE 6

void DoImages(Boolean afterActions) {
    static short pulseIndex = 0;
    static short bmX[MAXIMAGES], bmY[MAXIMAGES];
    static short dx[MAXIMAGES], dy[MAXIMAGES];
    short i, index, x, y, ybegin, yend;
    Rect bRect, cRect;
    PixMapHandle bmpm, maskpm;
    Ptr bmptr, maskptr, offptr;
    Point point;
    short bmcount, maskcount, bx, bit, useImage, useMask;
    short useWidth, useHeight;

    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    RefreshBMWorlds();
    if (!LockAllWorlds())
        return;
    for (i = 0; i < MAXIMAGES; i++) {
        if ((curSet.image[i].active == IMAGEACTIVE || curSet.image[i].active == MOVIEACTIVE) &&
            curSet.image[i].drawAfterActions == afterActions) {
            SetImageRect(i, 0, 0, &bRect);
            //**************************** Process Actions ***************************
            switch (curSet.image[i].action) {
                case BB_STATIONARY:
                case BB_JITTER:
                case BB_VIBRATE:
                    x = 0;
                    dx[i] = 0;
                    dy[i] = 0;
                    if (curSet.image[i].action == BB_JITTER)
                        x = 250;
                    if (curSet.image[i].action == BB_VIBRATE)
                        x = 100;
                    SetImageRect(i, curSet.image[i].place.left + RandVariance(x), curSet.image[i].place.top + RandVariance(x),
                                 &bRect);
                    break;
                case BB_ZEROG:                       // bouncing (zero-g)
                case BB_GRAVITY:                     // bouncing (gravity)
                case BB_WANDER:                      // wandering
                    if (dx[i] == 0 && dy[i] == 0) {  // initialise
                        dx[i] = RandVariance(500);
                        dy[i] = RandVariance(500);
                        bmX[i] = curSet.image[i].place.left;
                        bmY[i] = curSet.image[i].place.top;
                    }
                    bmX[i] += dx[i];
                    bmY[i] += dy[i];
                    if (curSet.image[i].action == BB_WANDER) {
                        dx[i] += RandVariance(50);
                        dy[i] += RandVariance(50);
                    }
                    if (curSet.image[i].action == BB_GRAVITY)
                        dy[i] += 10;
                    x = curSet.image[i].place.right - curSet.image[i].place.left;
                    y = curSet.image[i].place.bottom - curSet.image[i].place.top;
                    if (curSet.image[i].action == BB_WANDER) {   // wander bumps,
                        if (bmX[i] < 0) {
                            bmX[i] = 0;
                            dx[i] = 100;
                        }
                        if (bmY[i] < 0) {
                            bmY[i] = 0;
                            dy[i] = 100;
                        }
                        if (bmX[i] > (16384 - x)) {
                            bmX[i] = 16384 - x;
                            dx[i] = -100;
                        }
                        if (bmY[i] > (16384 - y)) {
                            bmY[i] = 16384 - y;
                            dy[i] = -100;
                        }
                    } else {   // everything else bounces
                        if (bmX[i] < 0) {
                            bmX[i] = 0;
                            dx[i] = (-dx[i]) - 20;
                        }
                        if (bmY[i] < 0) {
                            bmY[i] = 0;
                            dy[i] = (-dy[i]) - 20;
                            if (dy[i] < 20)
                                dy[i] = 20;
                        }
                        if (bmX[i] > 16384 - x) {
                            bmX[i] = 16384 - x;
                            dx[i] = -dx[i] + 20;
                        }
                        if (bmY[i] > 16384 - y) {
                            bmY[i] = 16384 - y;
                            dy[i] = -dy[i] * .75;
                            if (curSet.image[i].action == BB_GRAVITY && dy[i] > -160)
                                dy[i] = -RandNum(40, 500);
                        }
                    }
                    while (dx[i] < 80 && dx[i] > -80) {
                        dx[i] = RandVariance(500);
                    }
                    if (curSet.image[i].action == BB_ZEROG) {
                        while (dy[i] < 80 && dy[i] > -80)
                            dy[i] = RandVariance(500);
                    }
                    SetImageRect(i, bmX[i], bmY[i], &bRect);
                    break;
                case BB_MOUSE:
                    GetMouse(&point);
                    LocalToGlobal(&point);
                    x = point.h - prefs.windowPosition[WINMAIN].h;
                    y = point.v - prefs.windowPosition[WINMAIN].v;
                    if (prefs.lowQualityMode) {
                        x /= 2;
                        y /= 2;
                    }
                    if (prefs.highQualityMode) {
                        x *= 2;
                        y *= 2;
                    }
                    x -= (bRect.right / 2);
                    y -= (bRect.bottom / 2);
                    OffsetRect(&bRect, x, y);
                    break;
            }
            //***************************** Draw Bitmaps ******************************
            useImage = imageRef[i];
            useMask = maskRef[i];
            if (useImage >= 0 && bmWorld[useImage]) {
                GetPortBounds(bmWorld[useImage], &cRect);
                if (curSet.image[i].depth == 1) {   //********** 1-BIT **********
                    pulseIndex += 2;
                    if (pulseIndex > 510)
                        pulseIndex = 1;
                    if (curSet.image[i].britemode == 1) {   // pulse
                        index = pulseIndex;
                        if (index > 254)
                            index = 510 - index;
                    } else if (curSet.image[i].britemode == 2) {   // react sound
                        index = RecentVolume() * 2;
                        if (index < 0 || index > 255)
                            index = 255;
                    } else {   // fixed
                        index = curSet.image[i].brightness * 2.56;
                        if (index < 0 || index > 255)
                            index = 255;
                    }
                    bmpm = GetGWorldPixMap(bmWorld[useImage]);
                    bmptr = GetPixBaseAddr(bmpm);
                    bmcount = (0x7FFF & (**bmpm).rowBytes);
                    offptr = fromBasePtr + (bRect.top * fromByteCount) + bRect.left;
                    ybegin = 0;
                    if (bRect.top < 0) {
                        ybegin = -bRect.top;
                        offptr += ybegin * fromByteCount;
                        bmptr += ybegin * bmcount;
                    }
                    yend = cRect.bottom;
                    if (bRect.bottom > useHeight)
                        yend -= (bRect.bottom - useHeight);
                    if (curSet.image[i].transparent) {   //********* 1-BIT TRANSPARENT **********
                        for (y = ybegin; y < yend; y++) {
                            bit = 7;
                            bx = 0;
                            for (x = 0; x < cRect.right; x++) {
                                if (bRect.left + x >= 0 && bRect.left + x < useWidth) {
                                    if (!((unsigned char)bmptr[bx] & (1 << bit)))
                                        offptr[x] = index;
                                }
                                bit--;
                                if (bit < 0) {
                                    bx++;
                                    bit = 7;
                                }
                            }
                            offptr += fromByteCount;
                            bmptr += bmcount;
                        }
                    } else {   //********* 1-BIT SOLID **********
                        for (y = ybegin; y < yend; y++) {
                            bit = 7;
                            bx = 0;
                            for (x = 0; x < cRect.right; x++) {
                                if (bRect.left + x >= 0 && bRect.left + x < useWidth) {
                                    offptr[x] = !(((unsigned char)bmptr[bx] & (1 << bit)) > 0) * index;
                                }
                                bit--;
                                if (bit < 0) {
                                    bx++;
                                    bit = 7;
                                }
                            }
                            offptr += fromByteCount;
                            bmptr += bmcount;
                        }
                    }
                } else {   //********** 8-BIT **********
                    bmpm = GetGWorldPixMap(bmWorld[useImage]);
                    bmptr = GetPixBaseAddr(bmpm);
                    bmcount = (0x7FFF & (**bmpm).rowBytes);
                    offptr = fromBasePtr + (bRect.top * fromByteCount) + bRect.left;
                    yend = cRect.bottom;
                    if (bRect.bottom > useHeight)
                        yend -= (bRect.bottom - useHeight);
                    if (curSet.image[i].transparent && bmMaskWorld[useMask]) {    //********* 8-BIT TRANSPARENT **********
                        maskpm = GetGWorldPixMap(bmMaskWorld[useMask]);
                        maskptr = GetPixBaseAddr(maskpm);
                        maskcount = (0x7FFF & (**maskpm).rowBytes);
                        ybegin = 0;
                        if (bRect.top < 0) {
                            ybegin = -bRect.top;
                            offptr += ybegin * fromByteCount;
                            bmptr += ybegin * bmcount;
                            maskptr += ybegin * maskcount;
                        }
                        for (y = ybegin; y < yend; y++) {
                            bit = 7;
                            bx = 0;
                            for (x = 0; x < cRect.right; x++) {
                                if (bRect.left + x >= 0 && bRect.left + x < useWidth) {
                                    if (!((unsigned char)maskptr[bx] & (1 << bit)))
                                        offptr[x] = (unsigned char)bmptr[x];
                                }
                                bit--;
                                if (bit < 0) {
                                    bx++;
                                    bit = 7;
                                }
                            }
                            offptr += fromByteCount;
                            bmptr += bmcount;
                            maskptr += maskcount;
                        }
                    } else {   //********* 8-BIT SOLID **********
                        ybegin = 0;
                        if (bRect.top < 0) {
                            ybegin = -bRect.top;
                            offptr += ybegin * fromByteCount;
                            bmptr += ybegin * bmcount;
                        }
                        index = cRect.right;
                        if (bRect.left < 0) {
                            offptr -= bRect.left;
                            bmptr -= bRect.left;
                            index += bRect.left;
                        }
                        if (bRect.right > useWidth) {
                            index -= (bRect.right - useWidth);
                        }
                        for (y = ybegin; y < yend; y++) {
                            BlockMoveData(bmptr, offptr, index);
                            offptr += fromByteCount;
                            bmptr += bmcount;
                        }
                    }
                }
            } else {
                SaveContext();
                SetGWorld(offWorld, nil);
                PenSize(PixelSize(2), PixelSize(2));
                FrameRect(&bRect);
                RestoreContext();
            }
        }
    }
    UnlockAllWorlds();
}

void RefreshBMWorlds(void) {
    Boolean valid, iUsed[MAXIMAGES], mUsed[MAXIMAGES];
    Rect rect, cRect;
    short i, r;

    for (i = 0; i < MAXIMAGES; i++) {
        iUsed[i] = mUsed[i] = false;
    }
    /******************** ASSIGN EXISTING BITMAPS ********************/
    for (i = 0; i < MAXIMAGES; i++) {
        if (curSet.image[i].active == IMAGEACTIVE) {
            SetImageRect(i, 0, 0, &rect);
            imageRef[i] = -1;
            r = 0;
            while (r < MAXIMAGES && imageRef[i] == -1) {
                valid = (bmWorld[r] != nil);    // bitmap exists
                if (valid)
                    valid = (iinvert[r] == curSet.image[i].invert);    // invert state same
                if (valid)
                    valid = (idepth[r] == curSet.image[i].depth);    // bit depth
                if (valid)
                    GetPortBounds(bmWorld[r], &cRect);
                if (valid)
                    valid = (rect.right == cRect.right);    // image width
                if (valid)
                    valid = (rect.bottom == cRect.bottom);    // height
                if (valid)
                    valid = EqualFSS(&ifss[r], &curSet.image[i].imagefss);    // same source
                if (valid) {
                    imageRef[i] = r;
                    iUsed[r] = true;
                }
                r++;
            }
            maskRef[i] = -1;
            r = 0;
            if (curSet.image[i].depth == 8 && curSet.image[i].transparent) {   // needs a mask?
                while (r < MAXIMAGES && maskRef[i] == -1) {
                    valid = (bmMaskWorld[r] != nil);    // bitmap exists
                    if (valid)
                        valid = (minvert[r] == curSet.image[i].invertMask);    // invert state same?
                    if (valid)
                        GetPortBounds(bmMaskWorld[r], &cRect);
                    if (valid)
                        valid = (rect.right == cRect.right);
                    if (valid)
                        valid = (rect.bottom == cRect.bottom);
                    if (valid)
                        valid = EqualFSS(&mfss[r], &curSet.image[i].maskfss);
                    if (valid) {
                        maskRef[i] = r;
                        mUsed[r] = true;
                    }
                    r++;
                }
            }
        }
    }
    // Destroy unused bitmaps
    for (r = 0; r < MAXIMAGES; r++) {
        if (!iUsed[r])
            ResetBMWorld(r, SB_IMAGE);
        if (!mUsed[r])
            ResetBMWorld(r, SB_MASK);
    }
    // Create new bitmaps
    for (i = 0; i < MAXIMAGES; i++) {
        if (curSet.image[i].active == IMAGEACTIVE) {
            SetImageRect(i, 0, 0, &rect);
            if (imageRef[i] < 0) {   // needs a new home
                r = 0;
                while (r < MAXIMAGES && imageRef[i] == -1) {
                    valid = (bmWorld[r] != nil);    // bitmap exists
                    if (valid)
                        valid = (iinvert[r] == curSet.image[i].invert);    // invert state same
                    if (valid)
                        valid = (idepth[r] == curSet.image[i].depth);    // bit depth
                    if (valid)
                        GetPortBounds(bmWorld[r], &cRect);
                    if (valid)
                        valid = (rect.right == cRect.right);    // image width
                    if (valid)
                        valid = (rect.bottom == cRect.bottom);    // height
                    if (valid)
                        valid = EqualFSS(&ifss[r], &curSet.image[i].imagefss);    // same source
                    if (valid)
                        imageRef[i] = r;
                    r++;
                }
                if (imageRef[i] < 0) {
                    r = 0;
                    while (bmWorld[r] != nil) {
                        r++;
                    }
                    imageRef[i] = r;
                    iinvert[r] = curSet.image[i].invert;
                    idepth[r] = curSet.image[i].depth;
                    BlockMoveData(&curSet.image[i].imagefss, &ifss[r], sizeof(FSSpec));
                    SetupBMWorld(i, SB_IMAGE);
                }
            }
            if (maskRef[i] < 0 && curSet.image[i].depth == 8 && curSet.image[i].transparent) {   // needs a new home
                r = 0;
                while (r < MAXIMAGES && maskRef[i] == -1) {
                    valid = (bmMaskWorld[r] != nil);    // bitmap exists
                    if (valid)
                        valid = (minvert[r] == curSet.image[i].invertMask);    // invert state same?
                    if (valid)
                        GetPortBounds(bmMaskWorld[r], &cRect);
                    if (valid)
                        valid = (rect.right == cRect.right);
                    if (valid)
                        valid = (rect.bottom == cRect.bottom);
                    if (valid)
                        valid = EqualFSS(&mfss[r], &curSet.image[i].maskfss);
                    if (valid)
                        maskRef[i] = r;
                    r++;
                }
                if (maskRef[i] < 0) {
                    r = 0;
                    while (bmMaskWorld[r] != nil) {
                        r++;
                    }
                    maskRef[i] = r;
                    minvert[r] = curSet.image[i].invertMask;
                    BlockMoveData(&curSet.image[i].maskfss, &mfss[r], sizeof(FSSpec));
                    SetupBMWorld(i, SB_MASK);
                }
            }
        }
    }
}

// i = # in user list
// which = SB_IMAGE, SB_MASK, or SB_BOTH
void SetupBMWorld(short i, short which) {
    static CTabHandle greyColors = nil;
    ComponentInstance gi;
    PixMapHandle bmpm;
    PicHandle pict;
    RGBColor color;
    FSSpec fss;
    OSErr err;
    Rect bRect;
    Ptr bitmapPtr;
    Str32 /*eStr="\pError #",*/ str;
    short ts, destImage, destMask, bytecount, x, y;

    destImage = imageRef[i];
    destMask = maskRef[i];
    SetImageRect(i, 0, 0, &bRect);
    if (bRect.right <= bRect.left || bRect.bottom <= bRect.top) {
        imageRef[i] = -1;
        maskRef[i] = -1;
        return;
    }
    SaveContext();
    if (!bmWorld[destImage] && (which == SB_IMAGE || which == SB_BOTH)) {   // create it
        if (curSet.image[i].depth == 8) {
            if (!greyColors)
                greyColors = GetCTable(BASERES + 2);
            err = NewGWorld(&bmWorld[destImage], 8, &bRect, greyColors, nil, 0);
        } else {
            err = NewGWorld(&bmWorld[destImage], 1, &bRect, nil, nil, 0);
        }
        if (!err) {
            bmpm = GetGWorldPixMap(bmWorld[destImage]);
            PTLockPixels(bmpm);
            SetGWorld(bmWorld[destImage], nil);
            ForeColor(blackColor);
            BackColor(whiteColor);
            PaintRect(&bRect);
            OSErr orgFSSErr = noErr;
            if (curSet.image[i].imagefss.name[0]) {
                err = GetGraphicsImporterForFile(&curSet.image[i].imagefss, &gi);
                orgFSSErr = err;
                if (err) {
                    BlockMoveData(&curSet.image[i].imagefss, &fss, sizeof(FSSpec));
                    ChangeToCurSetsDirectory(&fss);
                    err = GetGraphicsImporterForFile(&fss, &gi);
                }
                if (err) {
                    BlockMoveData(&curSet.image[i].imagefss, &fss, sizeof(FSSpec));
                    if (ChangeToPicturesDirectory(&fss))
                        err = GetGraphicsImporterForFile(&fss, &gi);
                }
                if (err) {
                    BlockMoveData(&curSet.image[i].imagefss, &fss, sizeof(FSSpec));
                    ChangeToApplicationDirectory(&fss);
                    err = GetGraphicsImporterForFile(&fss, &gi);
                }
                if (err) {
                    BlockMoveData(&curSet.image[i].imagefss, &fss, sizeof(FSSpec));
                    if (ChangeToAppResourcesDirectory(&fss))
                        err = GetGraphicsImporterForFile(&fss, &gi);
                }
                if (!err)
                    err = GraphicsImportSetGWorld(gi, bmWorld[destImage], nil);
                if (!err)
                    err = GraphicsImportSetBoundsRect(gi, &bRect);
                color.red = color.green = color.blue = 0;
                if (!err)
                    err = GraphicsImportSetGraphicsMode(gi, srcCopy, &color);
                if (!err)
                    err = GraphicsImportDraw(gi);
                CloseComponent(gi);
            }
            if (err) {   // couldn't find image at org. place or with active sets
                ForeColor(whiteColor);
                TextMode(srcOr);
                ErrorToString(orgFSSErr, str);
                ts = 36;
                TextSize(ts);
                while (StringWidth(str) > bRect.right) {
                    TextSize(ts--);
                }
                MoveTo((bRect.right / 2) - (StringWidth(str) / 2), (bRect.bottom / 2) + (ts / 2));
                DrawString(str);
                FrameRect(&bRect);
                ForeColor(blackColor);
            }
            if (!curSet.image[i].imagefss.name[0]) {   // draw default stand-in
                pict = GetPicture(BASERES + 4);
                if (pict) {
                    DrawPicture(pict, &bRect);
                    ReleaseResource((Handle)pict);
                }
                bitmapPtr = GetPixBaseAddr(bmpm);
                prefs.exchange0and255 = ((unsigned char)bitmapPtr[0] == 255);
            }
            if (curSet.image[i].depth == 8 && prefs.exchange0and255) {
                bitmapPtr = GetPixBaseAddr(bmpm);
                bytecount = (0x7FFF & (**bmpm).rowBytes);
                for (y = bRect.top; y < bRect.bottom; y++) {
                    for (x = bRect.left; x < bRect.right; x++) {
                        if ((unsigned char)bitmapPtr[x] == 0)
                            bitmapPtr[x] = 255;
                        else if ((unsigned char)bitmapPtr[x] == 255)
                            bitmapPtr[x] = 0;
                    }
                    bitmapPtr += bytecount;
                }
            }
            if (curSet.image[i].invert)
                InvertRect(&bRect);
        }
        ResetCursor();
    }
    /* If 8-bit transparent, need mask world */
    if (curSet.image[i].depth == 8 && curSet.image[i].transparent) {
        if (!bmMaskWorld[destMask] && (which == SB_MASK || which == SB_BOTH)) {
            err = NewGWorld(&bmMaskWorld[destMask], 1, &bRect, nil, nil, 0);
            if (!err) {
                bmpm = GetGWorldPixMap(bmMaskWorld[destMask]);
                PTLockPixels(bmpm);
                SetGWorld(bmMaskWorld[destMask], nil);
                ForeColor(blackColor);
                BackColor(whiteColor);
                PaintRect(&bRect);
                if (curSet.image[i].maskfss.name[0]) {
                    err = GetGraphicsImporterForFile(&curSet.image[i].maskfss, &gi);
                    if (err) {
                        BlockMoveData(&curSet.image[i].maskfss, &fss, sizeof(FSSpec));
                        ChangeToCurSetsDirectory(&fss);
                        err = GetGraphicsImporterForFile(&fss, &gi);
                    }
                    if (err) {
                        BlockMoveData(&curSet.image[i].maskfss, &fss, sizeof(FSSpec));
                        if (ChangeToPicturesDirectory(&fss))
                            err = GetGraphicsImporterForFile(&fss, &gi);
                    }
                    if (err) {
                        BlockMoveData(&curSet.image[i].maskfss, &fss, sizeof(FSSpec));
                        ChangeToApplicationDirectory(&fss);
                        err = GetGraphicsImporterForFile(&fss, &gi);
                    }
                    if (err) {
                        BlockMoveData(&curSet.image[i].maskfss, &fss, sizeof(FSSpec));
                        if (ChangeToAppResourcesDirectory(&fss))
                            err = GetGraphicsImporterForFile(&fss, &gi);
                    }
                    if (!err)
                        err = GraphicsImportSetBoundsRect(gi, &bRect);
                    color.red = color.green = color.blue = 0;
                    if (!err)
                        err = GraphicsImportSetGraphicsMode(gi, srcCopy, &color);
                    if (!err)
                        err = GraphicsImportDraw(gi);
                    CloseComponent(gi);
                }
                if (err) {
                    EraseRect(&bRect);
                    TextMode(srcOr);
                    ErrorToString(err, str);
                    ts = 36;
                    TextSize(ts);
                    while (StringWidth(str) > bRect.right) {
                        TextSize(ts--);
                    }
                    MoveTo((bRect.right / 2) - (StringWidth(str) / 2), (bRect.bottom / 2) + (ts / 2));
                    DrawString(str);
                    FrameRect(&bRect);
                }
                if (!curSet.image[i].maskfss.name[0]) {   // draw internal stand-in
                    pict = GetPicture(BASERES + 5);
                    if (pict) {
                        DrawPicture(pict, &bRect);
                        ReleaseResource((Handle)pict);
                    }
                }
                if (curSet.image[i].invertMask)
                    InvertRect(&bRect);
            }
            ResetCursor();
        }
    }
    RestoreContext();
}

void ResetBMWorld(short i, short which) {
    if (which == SB_IMAGE || which == SB_BOTH) {
        if (bmWorld[i]) {
            DisposeGWorld(bmWorld[i]);
            bmWorld[i] = 0;
        }
    }
    if (which == SB_MASK || which == SB_BOTH) {
        if (bmMaskWorld[i]) {
            DisposeGWorld(bmMaskWorld[i]);
            bmMaskWorld[i] = 0;
        }
    }
}

void SetImageRect(short i, short x, short y, Rect *rect) {
    float f, useWidth, useHeight;

    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    f = 16384.0 / useWidth;
    if (f == 0)
        f = 0.01;
    rect->left = x / f;
    rect->right = rect->left + (curSet.image[i].place.right - curSet.image[i].place.left) / f;

    f = 16384.0 / useHeight;
    if (f == 0)
        f = 0.01;
    rect->top = y / f;
    rect->bottom = rect->top + (curSet.image[i].place.bottom - curSet.image[i].place.top) / f;
}

void ChangeToCurSetsDirectory(FSSpec *fss) {   // if file isn't where it specifies, try home dir
    fss->parID = curSetsSpec.parID;
    fss->vRefNum = curSetsSpec.vRefNum;
}

void ChangeToApplicationDirectory(FSSpec *fss) {
    fss->parID = 0;
    fss->vRefNum = 0;
    BlockMove(fss->name + 1, fss->name + 2, fss->name[0] + 1);
    fss->name[0]++;
    fss->name[1] = ':';
}

Boolean ChangeToAppResourcesDirectory(FSSpec *fss) {
    Boolean result = true;
    static short sResourceVRefNum = -666;
    static long sResourceParID = -666;
    if (sResourceVRefNum == -666) {
        result = false;
        CFURLRef resourcesURL = ResourcesDirectoryURL();
        CFURLRef ourResURL = CFURLCreateCopyAppendingPathComponent(nil, resourcesURL, CFSTR("Default.stng"), false);
        FSRef asFSRef;
        FSSpec asFSSpec;
        if (CFURLGetFSRef(ourResURL, &asFSRef)) {
            if (FSGetCatalogInfo(&asFSRef, 0, NULL, NULL, &asFSSpec, NULL) == noErr) {
                result = true;
                sResourceVRefNum = asFSSpec.vRefNum;
                sResourceParID = asFSSpec.parID;
            }
        }
    }
    if (result) {
        fss->parID = sResourceParID;
        fss->vRefNum = sResourceVRefNum;
    }
    return result;
}

// returns false if Pictures directory can't be found (i.e. not Mac OS X)
Boolean ChangeToPicturesDirectory(FSSpec *fss) {
    short foundVRefNum;
    long foundDirID;
    OSErr err = FindFolder(kUserDomain, kPictureDocumentsFolderType, false, &foundVRefNum, &foundDirID);
    if (err == noErr) {
        fss->parID = foundDirID;
        fss->vRefNum = foundVRefNum;
        return true;
    }
    return false;
}

