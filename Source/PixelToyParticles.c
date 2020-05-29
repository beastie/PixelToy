//  PixelToy Particles

#define MAXPSPEED 16384
#define MAXWANDERSPEED 100
#define DELTAVISUALSCALE 8
//#include <profiler.h>

#include "PixelToyParticles.h"

#include "Defs&Structs.h"
#include "PixelToyMain.h"
#include "PixelToyAutoPilot.h"
#include "PixelToyIF.h"
#include "PixelToyPalette.h"
#include "PixelToySets.h"
#include "PixelToySound.h"

extern CGrafPtr /*mainWorld,*/ offWorld;
//extern GDHandle                    mainDevice;
extern struct preferences prefs;
extern struct setinformation curSet;
extern ModalFilterUPP DialogFilterProc;
extern WindowPtr gMainWindow;
extern long maxparticles;
extern short fromByteCount;
extern Ptr fromBasePtr;
extern PaletteHandle winPalette;
extern struct PTParticle *ptparticle;
extern Boolean gDone;

char curGen;
Rect pgListRect;

/*******************************
 *  Local Function Prototypes  *
 *******************************/
void ShowGenValues(DialogPtr theDialog, char gen);
void BlendAllGenerators(void);
void HandleParticlePreviewImageClick(DialogPtr theDialog, Rect *rect);
void PreviewParticleGenerators(Rect *drawRect);
void DrawParticleList(Rect *drawRect);
void DrawParticles(Boolean snowPass);

#define ID_PSAVE 1
#define ID_PCANCEL 2
#define ID_PNEW 3
#define ID_PDUPLICATE 4
#define ID_PDELETE 5
#define ID_PLIST 6
#define ID_PPREVIEW 7
#define ID_PNAME 8
#define ID_PBEHAVIOR 9
#define ID_PGENACTION 10
#define ID_PRATE 11
#define ID_PGRAVITY 12
#define ID_PSIZE 13
#define ID_PSPOUTWIDTH 14
#define ID_PSPOUTHEIGHT 15
#define ID_PSPRAY 16
#define ID_PBRIGHTNESS 17
#define ID_PBOUNCEWALLS 18
#define ID_PSOLIDITY 19
#define ID_PBLEND 43

void ParticleEditor(void) {
    Point mouse;
    Rect previewRect, myRect;
    Boolean dialogDone, storeDoodle;
    DialogPtr theDialog;
    short avail, c, itemHit, temp;
    ControlHandle ctrl;
    Str255 str;
    long num;
    struct setinformation backup;

    storeDoodle = curSet.action[ACTDOODLE];
    curSet.action[ACTDOODLE] = false;
    StopAutoChanges();
    EnsureSystemPalette();
    BlockMoveData(&curSet, &backup, sizeof(struct setinformation));
    theDialog = GetNewDialog(DLG_PARTICLEDITOR, nil, (WindowPtr)-1);
    SetPalette(GetDialogWindow(theDialog), winPalette, false);
    ActivatePalette(GetDialogWindow(theDialog));
    SaveContext();
    if (prefs.windowPosition[WINPARTICLEOPTS].h != 0 || prefs.windowPosition[WINPARTICLEOPTS].v != 0)
        MoveWindow(GetDialogWindow(theDialog), prefs.windowPosition[WINPARTICLEOPTS].h, prefs.windowPosition[WINPARTICLEOPTS].v,
                   false);
    ForceOnScreen(GetDialogWindow(theDialog));
    ShowWindow(GetDialogWindow(theDialog));
    SetPortDialogPort(theDialog);
    ResetCursor();
    curGen = -1;
    for (c = MAXGENERATORS - 1; c >= 0; c--) {
        if (curSet.pg_pbehavior[c] > 0)
            curGen = c;
    }
    if (curGen == -1)
        curGen = CreateNewGenerator();
    // draw initial list
    GetDialogItem(theDialog, ID_PLIST, &temp, &ctrl, &pgListRect);
    GetDialogItem(theDialog, ID_PPREVIEW, &temp, &ctrl, &previewRect);
    DrawParticleList(&pgListRect);
    PreviewParticleGenerators(&previewRect);
    ShowGenValues(theDialog, curGen);
    SetSliderActions(theDialog, DLG_PARTICLEDITOR);
    UpdateSliderDisplay(theDialog);

    SetDialogDefaultItem(theDialog, ID_PSAVE);
    dialogDone = false;
    ConfigureFilter(ID_PSAVE, ID_PCANCEL);
    while (!dialogDone && !gDone) {
        ModalDialog(DialogFilterProc, &itemHit);
        switch (itemHit) {
            case ID_PSAVE:
                dialogDone = true;
                CurSetTouched();
                break;
            case ID_PCANCEL:
                BlockMoveData(&backup, &curSet, sizeof(struct setinformation));
                dialogDone = true;
                break;
            case ID_PBLEND:
                BlendAllGenerators();
                PreviewParticleGenerators(&previewRect);
                ShowGenValues(theDialog, curGen);
                UpdateSliderDisplay(theDialog);
                break;
            case ID_PNEW:
            case ID_PDUPLICATE: {
                avail = 0;    // avail = # of generators NOT IN USE
                for (c = 0; c < MAXGENERATORS; c++) {
                    if (curSet.pg_pbehavior[c] == 0)
                        avail++;
                }
                if (avail > 0) {
                    c = CreateNewGenerator();
                    if (itemHit == ID_PDUPLICATE) {
                        curSet.pg_pbehavior[c] = curSet.pg_pbehavior[curGen];
                        curSet.pg_genaction[c] = curSet.pg_genaction[curGen];
                        curSet.pg_solid[c] = curSet.pg_solid[curGen];
                        curSet.pg_rate[c] = curSet.pg_rate[curGen];
                        curSet.pg_gravity[c] = curSet.pg_gravity[curGen];
                        curSet.pg_walls[c] = curSet.pg_walls[curGen];
                        curSet.pg_xloc[c] = curSet.pg_xloc[curGen];
                        curSet.pg_yloc[c] = curSet.pg_yloc[curGen];
                        curSet.pg_xlocv[c] = curSet.pg_xlocv[curGen];
                        curSet.pg_ylocv[c] = curSet.pg_ylocv[curGen];
                        curSet.pg_dx[c] = curSet.pg_dx[curGen];
                        curSet.pg_dy[c] = curSet.pg_dy[curGen];
                        curSet.pg_dxv[c] = curSet.pg_dxv[curGen];
                        curSet.pg_dyv[c] = curSet.pg_dyv[curGen];
                        curSet.pg_size[c] = curSet.pg_size[curGen];
                        if (!OptionIsPressed()) {
                            if (curSet.pg_xloc[c] < 31000)
                                curSet.pg_xloc[c] += 1000;
                            if (curSet.pg_yloc[c] < 31000)
                                curSet.pg_yloc[c] += 1000;
                        }
                    }
                    curGen = c;
                } else {
                    SysBeep(1);
                }
            }
                DrawParticleList(&pgListRect);
                PreviewParticleGenerators(&previewRect);
                ShowGenValues(theDialog, curGen);
                UpdateSliderDisplay(theDialog);
                break;
            case ID_PDELETE:
                avail = 0;    // avail = # of generators IN USE
                for (c = 0; c < MAXGENERATORS; c++) {
                    if (curSet.pg_pbehavior[c] > 0)
                        avail++;
                }
                if (avail > 1) {
                    curSet.pg_pbehavior[curGen] = 0;
                    for (num = 0; num < maxparticles; num++) {
                        if (ptparticle[num].gen == curGen + 1)
                            ptparticle[num].gen = 0;
                    }
                    while (curSet.pg_pbehavior[curGen] == 0) {
                        curGen--;
                        if (curGen < 0)
                            curGen = MAXGENERATORS - 1;
                    }
                    DrawParticleList(&pgListRect);
                    PreviewParticleGenerators(&previewRect);
                    ShowGenValues(theDialog, curGen);
                    UpdateSliderDisplay(theDialog);
                } else {
                    DoStandardAlert(kAlertStopAlert, 5);
                    DrawParticleList(&pgListRect);
                    PreviewParticleGenerators(&previewRect);
                }
                break;
            case ID_PLIST:
                GetMouse(&mouse);
                temp = (mouse.v - pgListRect.top) / 15;
                avail = -1;
                for (c = 0; c < MAXGENERATORS; c++) {
                    if (curSet.pg_pbehavior[c] > 0) {
                        avail++;
                        if (temp == avail && c != curGen) {
                            curGen = c;
                            DrawParticleList(&pgListRect);
                            PreviewParticleGenerators(&previewRect);
                            ShowGenValues(theDialog, curGen);
                            UpdateSliderDisplay(theDialog);
                        }
                    }
                }
                break;
            case ID_PNAME:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                GetDialogItemText((Handle)ctrl, str);
                if (str[0] > 62)
                    str[0] = 62;
                BlockMoveData(str, curSet.pg_name[curGen], str[0] + 1);
                DrawParticleList(&pgListRect);
                break;
            // Popups
            case ID_PBEHAVIOR:
            case ID_PGENACTION:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                c = GetControlValue(ctrl);
                if (itemHit == ID_PGENACTION)
                    curSet.pg_genaction[curGen] = c;
                if (itemHit == ID_PBEHAVIOR) {
                    curSet.pg_pbehavior[curGen] = c;
                    if (c == 4)
                        curSet.pg_solid[curGen] = 252;
                    ShowGenValues(theDialog, curGen);
                }
                PreviewParticleGenerators(&previewRect);
                break;
            // Checkboxes
            case ID_PSOLIDITY:
            case ID_PBOUNCEWALLS:
                GetDialogItemAsControl(theDialog, itemHit, &ctrl);
                c = GetControlValue(ctrl);
                SetControlValue(ctrl, !c);
                if (itemHit == ID_PSOLIDITY)
                    curSet.pg_solid[curGen] = 255 - (!c) * 3;
                if (itemHit == ID_PBOUNCEWALLS)
                    curSet.pg_walls[curGen] = !curSet.pg_walls[curGen];
                break;
            case ID_PPREVIEW:    // read mouse location to move generator or "target" speed
                HandleParticlePreviewImageClick(theDialog, &previewRect);
                break;
            default:
                DrawParticleList(&pgListRect);
                PreviewParticleGenerators(&previewRect);
                break;
        }
        UpdateCurrentSetNeedsSoundInputMessage();
    }
    // Remove any particles without corresponding generators
    for (num = 0; num < maxparticles; num++) {
        if (ptparticle[num].gen) {
            if (!curSet.pg_pbehavior[ptparticle[num].gen - 1])
                ptparticle[num].gen = 0;
        }
    }
    curSet.action[ACTDOODLE] = storeDoodle;
    GetWindowPortBounds(GetDialogWindow(theDialog), &myRect);
    prefs.windowPosition[WINPARTICLEOPTS].h = myRect.left;
    prefs.windowPosition[WINPARTICLEOPTS].v = myRect.top;
    LocalToGlobal(&prefs.windowPosition[WINPARTICLEOPTS]);
    RestoreContext();
    DisposeDialog(theDialog);
    PopPal();
    ResumeAutoChanges();
    if (prefs.fullScreen)
        MyHideMenuBar();
}

void SetLiveParticleSliderValue(short item, short value) {
    short i;

    if (item == ID_PRATE) {
        curSet.pg_rate[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_rate[i] = value;
        }
    }

    if (item == ID_PGRAVITY) {
        curSet.pg_gravity[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_gravity[i] = value;
        }
    }

    if (item == ID_PSPOUTWIDTH) {
        curSet.pg_xlocv[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_xlocv[i] = value;
        }
    }

    if (item == ID_PSPOUTHEIGHT) {
        curSet.pg_ylocv[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_ylocv[i] = value;
        }
    }

    if (item == ID_PSPRAY) {
        curSet.pg_dxv[curGen] = value;
        curSet.pg_dyv[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0) {
                    curSet.pg_dxv[i] = value;
                    curSet.pg_dyv[i] = value;
                }
        }
    }

    if (item == ID_PSIZE) {
        curSet.pg_size[curGen] = value;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_size[i] = value;
        }
    }

    if (item == ID_PBRIGHTNESS) {
        curSet.pg_brightness[curGen] = value | 0xBF00;
        if (OptionIsPressed()) {
            for (i = 0; i < MAXGENERATORS; i++)
                if (curSet.pg_pbehavior[i] != 0)
                    curSet.pg_brightness[i] = value | 0xBF00;
        }
    }
}

void ShowGenValues(DialogPtr theDialog, char gen) {
    Boolean activate;
    int c, inUse;
    ControlHandle ctrl;

    if (curSet.pg_pbehavior[gen] == 0)
        return;
    // set up name and select it
    GetDialogItemAsControl(theDialog, ID_PNAME, &ctrl);
    SetDialogItemText((Handle)ctrl, curSet.pg_name[gen]);
    SelectDialogItemText(theDialog, ID_PNAME, 0, 32767);
    // set particle behavior
    GetDialogItemAsControl(theDialog, ID_PBEHAVIOR, &ctrl);
    SetControlValue(ctrl, curSet.pg_pbehavior[gen]);
    // set generator action
    GetDialogItemAsControl(theDialog, ID_PGENACTION, &ctrl);
    SetControlValue(ctrl, curSet.pg_genaction[gen]);
    // set solidity
    GetDialogItemAsControl(theDialog, ID_PSOLIDITY, &ctrl);
    SetControlValue(ctrl, (curSet.pg_solid[gen] < 255));
    if (curSet.pg_pbehavior[gen] == 4) {
        DeactivateControl(ctrl);
    } else {
        ActivateControl(ctrl);
    }
    // set rate
    GetDialogItemAsControl(theDialog, ID_PRATE, &ctrl);
    SetControlValue(ctrl, curSet.pg_rate[gen]);
    // set gravity
    GetDialogItemAsControl(theDialog, ID_PGRAVITY, &ctrl);
    SetControlValue(ctrl, curSet.pg_gravity[gen]);
    // set posn randomnesss
    GetDialogItemAsControl(theDialog, ID_PSPOUTWIDTH, &ctrl);
    SetControlValue(ctrl, curSet.pg_xlocv[gen]);
    GetDialogItemAsControl(theDialog, ID_PSPOUTHEIGHT, &ctrl);
    SetControlValue(ctrl, curSet.pg_ylocv[gen]);
    // set speed randomnesss
    GetDialogItemAsControl(theDialog, ID_PSPRAY, &ctrl);
    SetControlValue(ctrl, curSet.pg_dxv[gen]);
    // set size
    GetDialogItemAsControl(theDialog, ID_PSIZE, &ctrl);
    SetControlValue(ctrl, curSet.pg_size[gen]);
    // set bounce walls/ceiling checkbox
    GetDialogItemAsControl(theDialog, ID_PBOUNCEWALLS, &ctrl);
    SetControlValue(ctrl, curSet.pg_walls[gen]);
    // set brightness
    GetDialogItemAsControl(theDialog, ID_PBRIGHTNESS, &ctrl);
    SetControlValue(ctrl, curSet.pg_brightness[gen] & 0x00FF);
    UpdateSliderDisplay(theDialog);
    // Disable Add/Copy/Blend All as necessary
    inUse = 0;
    for (c = 0; c < MAXGENERATORS; c++) {
        if (curSet.pg_pbehavior[c] > 0)
            inUse++;
    }
    activate = (inUse < MAXGENERATORS);    // disable add and copy if no more room!
    GetDialogItemAsControl(theDialog, ID_PNEW, &ctrl);
    if (activate)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);
    GetDialogItemAsControl(theDialog, ID_PDUPLICATE, &ctrl);
    if (activate)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);
    GetDialogItemAsControl(theDialog, ID_PBLEND, &ctrl);
    if (inUse > 2)
        ActivateControl(ctrl);
    else
        DeactivateControl(ctrl);
}

char CreateNewGenerator(void) {
    Str255 str;
    char avail = 0, c, numeral;
    Boolean nameGood = false;

    for (c = 0; c < MAXGENERATORS; c++) {
        if (curSet.pg_pbehavior[c] != 0)
            avail++;
    }

    avail++;
    numeral = 1;
    while (!nameGood) {
        // create name
        GetIndString(str, BASERES, 26);
        if (numeral > 9) {
            str[++str[0]] = '1';
            str[++str[0]] = '0' + (numeral - 10);
        } else {
            str[++str[0]] = '0' + numeral;
        }
        nameGood = true;
        // check if any other generators have this name
        for (c = 0; c < MAXGENERATORS; c++) {
            if (curSet.pg_pbehavior[c] != 0) {
                if (EqualString(str, curSet.pg_name[c], false, false))
                    nameGood = false;
            }
        }
        if (!nameGood)
            numeral++;
    }
    c = 0;
    while (curSet.pg_pbehavior[c] > 0)
        c++;
    BlockMoveData(str, curSet.pg_name[c], str[0] + 1);
    curSet.pg_pbehavior[c] = 1;     // fountain
    curSet.pg_genaction[c] = 1;     // stationary
    curSet.pg_brightness[c] = 0;    // force unkeyed default validation
    ValidateParticleGeneratorBrightnesses(&curSet);
    curSet.pg_solid[c] = 255;
    curSet.pg_rate[c] = 25;
    curSet.pg_size[c] = 10;
    curSet.pg_gravity[c] = 40;
    curSet.pg_walls[c] = true;
    curSet.pg_xloc[c] = 16384;
    curSet.pg_yloc[c] = 30000;
    curSet.pg_xlocv[c] = 4;
    curSet.pg_ylocv[c] = 4;
    curSet.pg_dx[c] = 0;
    curSet.pg_dy[c] = -1400;
    curSet.pg_dxv[c] = 20;
    curSet.pg_dyv[c] = 20;
    return c;
}

void BlendAllGenerators(void) {
    int first = -1, last, i, numGens = 0;
    float yloc, d_yloc, xloc, d_xloc, xlocv, d_xlocv, ylocv, d_ylocv;
    float dx, d_dx, dy, d_dy, dxv, d_dxv, dyv, d_dyv;
    float bright, d_bright, rate, d_rate, size, d_size, gravity, d_gravity;

    for (i = 0; i < MAXGENERATORS; i++) {
        if (curSet.pg_pbehavior[i] > 0) {
            numGens++;
            if (first == -1)
                first = i;
            last = i;
        }
    }

    // source location
    yloc = curSet.pg_yloc[first];
    d_yloc = (float)(curSet.pg_yloc[last] - yloc) / (float)(numGens - 1);
    xloc = curSet.pg_xloc[first];
    d_xloc = (float)(curSet.pg_xloc[last] - xloc) / (float)(numGens - 1);
    // source size
    xlocv = curSet.pg_xlocv[first];
    d_xlocv = (float)(curSet.pg_xlocv[last] - xlocv) / (float)(numGens - 1);
    ylocv = curSet.pg_ylocv[first];
    d_ylocv = (float)(curSet.pg_ylocv[last] - ylocv) / (float)(numGens - 1);
    // direction/force
    dx = curSet.pg_dx[first];
    d_dx = (float)(curSet.pg_dx[last] - dx) / (float)(numGens - 1);
    dy = curSet.pg_dy[first];
    d_dy = (float)(curSet.pg_dy[last] - dy) / (float)(numGens - 1);
    // spray
    dxv = curSet.pg_dxv[first];
    d_dxv = (float)(curSet.pg_dxv[last] - dxv) / (float)(numGens - 1);
    dyv = curSet.pg_dyv[first];
    d_dyv = (float)(curSet.pg_dyv[last] - dyv) / (float)(numGens - 1);
    // then the obvious ones...
    gravity = curSet.pg_gravity[first];
    d_gravity = (float)(curSet.pg_gravity[last] - gravity) / (float)(numGens - 1);
    bright = curSet.pg_brightness[first] & 0x00FF;
    d_bright = (float)((curSet.pg_brightness[last] & 0x00FF) - bright) / (float)(numGens - 1);
    rate = curSet.pg_rate[first];
    d_rate = (float)(curSet.pg_rate[last] - rate) / (float)(numGens - 1);
    size = curSet.pg_size[first];
    d_size = (float)(curSet.pg_size[last] - size) / (float)(numGens - 1);

    for (i = first + 1; i < last; i++) {
        if (curSet.pg_pbehavior[i] > 0) {
            xloc += d_xloc;
            curSet.pg_xloc[i] = (short)xloc;
            yloc += d_yloc;
            curSet.pg_yloc[i] = (short)yloc;
            xlocv += d_xlocv;
            curSet.pg_xlocv[i] = (short)xlocv;
            ylocv += d_ylocv;
            curSet.pg_ylocv[i] = (short)ylocv;
            dx += d_dx;
            curSet.pg_dx[i] = (short)dx;
            dy += d_dy;
            curSet.pg_dy[i] = (short)dy;
            dxv += d_dxv;
            curSet.pg_dxv[i] = (short)dxv;
            dyv += d_dyv;
            curSet.pg_dyv[i] = (short)dyv;
            bright += d_bright;
            curSet.pg_brightness[i] = (short)bright | 0xBF00;
            gravity += d_gravity;
            curSet.pg_gravity[i] = (short)gravity;
            rate += d_rate;
            curSet.pg_rate[i] = (short)rate;
            size += d_size;
            curSet.pg_size[i] = (short)size;
        }
    }
}

// otherwise-ignored high byte of brightness value used as key to indicate if a value has been set or not.
void ValidateParticleGeneratorBrightnesses(void *mem) {
    struct setinformation *aSet = mem;
    int i;
    for (i = 0; i < MAXGENERATORS; i++) {
        if (aSet->pg_pbehavior[i] > 0) {
            if (((unsigned short)aSet->pg_brightness[i] & 0xFF00) != 0xBF00) {
                aSet->pg_brightness[i] = 100 | 0xBF00;
            }
        }
    }
}

void HandleParticlePreviewImageClick(DialogPtr theDialog, Rect *rect) {
    Boolean selected, movingTarget;
    long lx, ly, lBound, rBound, tBound, bBound;
    short x, y, gx, gy, lastx, lasty, c, avail, offx, offy;
    float xmult, ymult;
    Point mouse;

    xmult = (32768.0 / (rect->right - rect->left));
    ymult = (32768.0 / (rect->bottom - rect->top));
    lastx = lasty = -1;
    selected = movingTarget = false;
    GetMouse(&mouse);
    PreviewParticleGenerators(rect);
    x = (mouse.h - rect->left) * xmult;
    y = (mouse.v - rect->top) * ymult;
    avail = -1;
    for (c = MAXGENERATORS - 1; c >= 0; c--) {
        if (curSet.pg_pbehavior[c] > 0) {
            gx = curSet.pg_xloc[c];
            gy = curSet.pg_yloc[c];
            // define selection area
            if (curSet.pg_pbehavior[c] != 3) {
                lBound = gx - (curSet.pg_xlocv[c] * 160);
                rBound = gx + (curSet.pg_xlocv[c] * 160);
                tBound = gy - (curSet.pg_ylocv[c] * 160);
                bBound = gy + (curSet.pg_ylocv[c] * 160);
            } else {   // snow, just source point
                lBound = rBound = gx;
                tBound = bBound = gy;
            }
            lBound -= (xmult * 3);
            rBound += (xmult * 3);
            tBound -= (ymult * 3);
            bBound += (ymult * 3);
            if (x >= lBound && x <= rBound && y >= tBound && y <= bBound) {
                selected = true;
                avail = c;
            }
            // select end point?
            if (curSet.pg_pbehavior[c] != 3) {
                gx = curSet.pg_xloc[c] + (curSet.pg_dx[c] * DELTAVISUALSCALE);
                gy = curSet.pg_yloc[c] + (curSet.pg_dy[c] * DELTAVISUALSCALE);
                if (x > (gx - xmult * 3) && y > (gy - ymult * 3) && x < (gx + xmult * 3) && y < (gy + ymult * 3)) {
                    selected = movingTarget = true;
                    avail = c;
                }
            }
        }
    }
    if (curGen != avail && avail != -1) {
        curGen = avail;
        DrawParticleList(&pgListRect);
        PreviewParticleGenerators(rect);
        ShowGenValues(theDialog, curGen);
        UpdateSliderDisplay(theDialog);
    }
    offx = offy = 0;
    if (selected && !movingTarget) {
        offx = (mouse.h - rect->left) * xmult - curSet.pg_xloc[curGen];
        offy = (mouse.v - rect->top) * ymult - curSet.pg_yloc[curGen];
    }
    while (selected && StillDown()) {
        GetMouse(&mouse);
        lx = (mouse.h - rect->left) * xmult - offx;
        ly = (mouse.v - rect->top) * ymult - offy;
        if (lx < 256)
            lx = 256;
        if (lx > 32512)
            lx = 32512;
        if (ly < 256)
            ly = 256;
        if (ly > 32512)
            ly = 32512;
        x = (short)lx;
        y = (short)ly;
        if (x != lastx || y != lasty) {
            if (movingTarget) {
                curSet.pg_dx[curGen] = ((x - curSet.pg_xloc[curGen]) / DELTAVISUALSCALE);
                curSet.pg_dy[curGen] = ((y - curSet.pg_yloc[curGen]) / DELTAVISUALSCALE);
            } else {
                curSet.pg_xloc[curGen] = x;
                curSet.pg_yloc[curGen] = y;
            }
            PreviewParticleGenerators(rect);
            lastx = x;
            lasty = y;
        }
    }
}

void PreviewParticleGenerators(Rect *drawRect) {
    Rect previewRect, rect;
    OSErr error;
    GDHandle curDevice;
    CGrafPtr curPort, curWorld, previewWorld;
    short dx, dy, gx, gy, g;
    RGBColor greyColor;
    float xmult, ymult;

    previewRect.left = previewRect.top = 0;
    previewRect.right = (drawRect->right - drawRect->left);
    previewRect.bottom = (drawRect->bottom - drawRect->top);
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
    TextSize(9);
    BackColor(blackColor);
    EraseRect(&previewRect);

    greyColor.red = greyColor.green = greyColor.blue = 32768;
    xmult = (32768.0 / previewRect.right);
    ymult = (32768.0 / previewRect.bottom);
    for (g = MAXGENERATORS - 1; g >= 0; g--) {
        if (curSet.pg_pbehavior[g]) {
            gx = curSet.pg_xloc[g] / xmult;
            gy = curSet.pg_yloc[g] / ymult;
            if (g == curGen) {
                ForeColor(whiteColor);
            } else {
                RGBForeColor(&greyColor);
            }
            rect.left = gx - 2;
            rect.right = rect.left + 5;
            rect.top = gy - 2;
            rect.bottom = rect.top + 5;
            PaintOval(&rect);
            if (curSet.pg_pbehavior[g] != 3) {   // not snow, draw source area
                // show source area
                rect.left = gx - ((curSet.pg_xlocv[g] * 160) / xmult);
                rect.right = gx + ((curSet.pg_xlocv[g] * 160) / xmult);
                rect.top = gy - ((curSet.pg_ylocv[g] * 160) / ymult);
                rect.bottom = gy + ((curSet.pg_ylocv[g] * 160) / ymult);
                rect.right++;
                rect.bottom++;
                FrameRect(&rect);
            }
            // spew direction
            if (curSet.pg_pbehavior[g] != 3 && curSet.pg_genaction[g] != 3 && curSet.pg_genaction[g] != 2) {
                dx = gx + ((curSet.pg_dx[g] / xmult) * DELTAVISUALSCALE);
                dy = gy + ((curSet.pg_dy[g] / ymult) * DELTAVISUALSCALE);
                rect.left = dx - 2;
                rect.right = rect.left + 5;
                rect.top = dy - 2;
                rect.bottom = rect.top + 5;
                FrameOval(&rect);
                MoveTo(gx, gy);
                LineTo(dx, dy);
            }
        }
    }
    ForeColor(whiteColor);
    FrameRect(&previewRect);
    ForeColor(blackColor);
    BackColor(whiteColor);
    InsetRect(&previewRect, 2, 2);
    DrawThemeListBoxFrame(&previewRect, kThemeStateActive);
    InsetRect(&previewRect, -2, -2);
    SetGWorld(curWorld, curDevice);
    SetPort(curPort);
    CopyBits(GetPortBitMapForCopyBits(previewWorld), GetPortBitMapForCopyBits(curPort), &previewRect, drawRect, srcCopy, nil);
    DisposeGWorld(previewWorld);
}

void DrawParticleList(Rect *drawRect) {
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
    ForeColor(blackColor);
    BackColor(whiteColor);
    TextMode(srcOr);
    EraseRect(&listRect);
    y = 14;
    LMGetHiliteRGB(&highlightColor);
    if (highlightColor.red < 24576)
        highlightColor.red = 24576;
    if (highlightColor.green < 24576)
        highlightColor.green = 24576;
    if (highlightColor.blue < 24576)
        highlightColor.blue = 24576;
    for (c = 0; c < MAXGENERATORS; c++) {
        if (curSet.pg_pbehavior[c]) {
            BlockMoveData(curSet.pg_name[c], displayStr, curSet.pg_name[c][0] + 1);
            while (StringWidth(displayStr) > listRect.right - 6) {
                displayStr[0]--;
                displayStr[displayStr[0]] = 201;
            }    // 201 = MacRoman ellipsis
            MoveTo(4, y);
            DrawString(displayStr);
            if (curGen == c) {
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

void ResetParticles(void) {
    long p;

    for (p = 0; p < maxparticles; p++) {
        ptparticle[p].gen = 0;
    }
}

void DoParticles(void) {
    Point mouse;
    char g;
    short brite, sx, sy, size, c, rate, dist, tx, ty, tox, toy, dx, dy;
    Boolean occ[9];
    Boolean target, dugout;
    long p, freeParticle, value, offset;
    float xs, ys, x, y, ox, oy, fx, fy;
    short useWidth, useHeight;
    short psize[MAXGENERATORS];

    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    DrawParticles(true);    // get the current snow drawn
    if (!LockAllWorlds())
        return;
    // add any new particles from generators
    SaveContext();
    SetPortWindowPort(gMainWindow);
    GetMouse(&mouse);
    RestoreContext();
    if (prefs.lowQualityMode) {
        mouse.h /= 2;
        mouse.v /= 2;
    }
    if (prefs.highQualityMode) {
        mouse.h *= 2;
        mouse.v *= 2;
    }
    xs = (32768.0 / useWidth);
    ys = (32768.0 / useHeight);
    dist = RecentVolume();
    for (g = 0; g < MAXGENERATORS; g++) {
        if (curSet.pg_pbehavior[g]) {
            psize[g] = curSet.pg_size[g] / 2;
            if (!prefs.highQualityMode && !prefs.lowQualityMode)
                psize[g] /= 2;
            if (prefs.lowQualityMode)
                psize[g] /= 4;
            if (psize[g] < 2)
                psize[g] = 1;
            switch (curSet.pg_genaction[g]) {
                case 1:    // stationary
                    break;
                case 2:    // follow mouse
                    dx = ((mouse.h * xs) - curSet.pg_xloc[g]) / DELTAVISUALSCALE;
                    dy = ((mouse.v * ys) - curSet.pg_yloc[g]) / DELTAVISUALSCALE;
                    curSet.pg_dx[g] += (dx - curSet.pg_dx[g]) / 8;
                    curSet.pg_dy[g] += (dy - curSet.pg_dy[g]) / 8;
                    if (curSet.pg_dx[g] < -1600)
                        curSet.pg_dx[g] = -1600;
                    if (curSet.pg_dx[g] > 1600)
                        curSet.pg_dx[g] = 1600;
                    if (curSet.pg_dy[g] < -1600)
                        curSet.pg_dy[g] = -1600;
                    if (curSet.pg_dy[g] > 1600)
                        curSet.pg_dy[g] = 1600;
                    break;
                case 3:    // random movement
                    curSet.pg_dx[g] += RandVariance(xs / 2);
                    curSet.pg_dy[g] += RandVariance(ys / 2);
                    if (curSet.pg_dx[g] < -1600)
                        curSet.pg_dx[g] = -1600;
                    if (curSet.pg_dx[g] > 1600)
                        curSet.pg_dx[g] = 1600;
                    if (curSet.pg_dy[g] < -1600)
                        curSet.pg_dy[g] = -1600;
                    if (curSet.pg_dy[g] > 1600)
                        curSet.pg_dy[g] = 1600;
                    break;
                case 4:    // react to sound
                    break;
            }
            rate = 0;
            if (curSet.pg_rate[g])
                rate = curSet.pg_rate[g] / 2 + 1;
            if (curSet.pg_genaction[g] == 4 && dist >= 0) {
                rate = rate * dist / 128;
            }
            for (c = 0; c < rate; c++) {
                freeParticle = -1;
                p = 0;
                while (freeParticle == -1 && p < maxparticles) {
                    if (ptparticle[p].gen == 0)
                        freeParticle = p;
                    p++;
                }
                if (freeParticle != -1) {   // generate a new particle
                    p = freeParticle;
                    ptparticle[p].gen = g + 1;
                    if (curSet.pg_pbehavior[g] == 3) {   // snow
                        ptparticle[p].x = RandNum(xs, 32767 - xs);
                        ptparticle[p].y = RandNum(ys, (ys * 16));
                        ptparticle[p].dx = 0;
                        ptparticle[p].dy = ys;
                    } else {   // not snow
                        value = curSet.pg_xloc[g] + RandVariance(curSet.pg_xlocv[g] * 160);
                        if (value < 0)
                            value = 0;
                        if (value > 32767)
                            value = 32767;
                        ptparticle[p].x = value;
                        value = curSet.pg_yloc[g] + RandVariance(curSet.pg_ylocv[g] * 160);
                        if (value < 0)
                            value = 0;
                        if (value > 32767)
                            value = 32767;
                        ptparticle[p].y = value;
                        if (curSet.pg_genaction[g] == 4 && dist >= 0) {   // react to sound
                            ptparticle[p].dx = (curSet.pg_dx[g] / 32) * dist + RandVariance(curSet.pg_dxv[g] * 4);
                            ptparticle[p].dy = (curSet.pg_dy[g] / 32) * dist + RandVariance(curSet.pg_dyv[g] * 4);
                        } else {
                            ptparticle[p].dx = curSet.pg_dx[g] + RandVariance(curSet.pg_dxv[g] * 4);
                            ptparticle[p].dy = curSet.pg_dy[g] + RandVariance(curSet.pg_dyv[g] * 4);
                        }
                    }
                }
            }
        }
    }

    // update all particle positions
    for (p = 0; p < maxparticles; p++) {
        if (ptparticle[p].gen) {
            x = (float)ptparticle[p].x;
            ox = x;
            y = (float)ptparticle[p].y;
            oy = y;
            g = ptparticle[p].gen - 1;
            brite = (int)((float)(curSet.pg_brightness[g] & 0x00FF) * 2.55);
            if (brite > 255)
                brite = 255;
            // update position
            x += ptparticle[p].dx;
            y += ptparticle[p].dy;
            switch (curSet.pg_pbehavior[g]) {
                case 1:    // Water
                    ptparticle[p].dy += curSet.pg_gravity[g];
                    if (ptparticle[p].dy > 2048)
                        ptparticle[p].dy = 2048;
                    break;
                case 2:    // Bugs
                    ptparticle[p].dx += RandVariance(xs);
                    ptparticle[p].dy += RandVariance(ys);
                    ptparticle[p].dy += curSet.pg_gravity[g];
                    if (ptparticle[p].dx > MAXWANDERSPEED)
                        ptparticle[p].dx -= (MAXWANDERSPEED / 8);
                    if (ptparticle[p].dy > MAXWANDERSPEED)
                        ptparticle[p].dy -= (MAXWANDERSPEED / 8);
                    if (ptparticle[p].dx < -MAXWANDERSPEED)
                        ptparticle[p].dx += (MAXWANDERSPEED / 8);
                    if (ptparticle[p].dy < -MAXWANDERSPEED)
                        ptparticle[p].dy += (MAXWANDERSPEED / 8);
                    break;
                case 3:    // Falling snow
                    ptparticle[p].dx = 0;
                    ptparticle[p].dy = ys;
                    break;
                case 4:    // Repulsion
                    tx = x / xs;
                    ty = y / ys;
                    tox = ox / xs;
                    toy = oy / ys;
                    dist = 4;
                    for (dy = ty - dist; dy <= ty + dist; dy++) {
                        for (dx = tx - dist; dx <= tx + dist; dx++) {
                            if (dx != tx && dy != ty && dx > 0 && dx < prefs.winxsize && dy > 0 && dy < prefs.winysize) {
                                target = ((unsigned char)fromBasePtr[dy * fromByteCount + dx] > curSet.pg_solid[g]);
                                if (target) {
                                    if (dx <= tx) {
                                        ptparticle[p].dx += ((dist - (tx - dx)) * (xs / 64));
                                    } else {
                                        ptparticle[p].dx -= ((dist - (dx - tx)) * (xs / 64));
                                    }
                                    if (dy <= ty) {
                                        ptparticle[p].dy += ((dist - (ty - dy)) * (ys / 64));
                                    } else {
                                        ptparticle[p].dy -= ((dist - (dy - ty)) * (ys / 64));
                                    }
                                }
                            }
                        }
                    }
                    ptparticle[p].dy += curSet.pg_gravity[g];
                    if (ptparticle[p].dy > 2048)
                        ptparticle[p].dy = 2048;
                    break;
            }
            if (curSet.pg_walls[g]) {   // bounce off walls & ceiling (but not floor)
                if (x < xs) {
                    x += xs;
                    ptparticle[p].dx = -ptparticle[p].dx;
                }
                if (y < ys) {
                    y += ys;
                    ptparticle[p].dy = -ptparticle[p].dy;
                    if (ptparticle[p].dy < ys)
                        ptparticle[p].dy = ys;
                }
                if (x > (32767 - xs)) {
                    x = (32767 - xs) - (x - (32767 - xs));
                    ptparticle[p].dx = -ptparticle[p].dx;
                }
                if (y > (32767 - ys))
                    ptparticle[p].gen = 0;    // turn off particle
            } else {
                if (x < xs || y < ys || x > (32767 - xs) || y > (32767 - ys))
                    ptparticle[p].gen = 0;
            }

            if (ptparticle[p].gen) {
                sx = (short)(x / xs);
                sy = (short)(y / ys);
                size = psize[g];
                // **************************************************************************************************************
                if (curSet.pg_solid[g] < 255) {
                    switch (curSet.pg_pbehavior[g]) {
                        short ycheck;
                        //   X
                        // 0 @ 2    X = dot, @ = definitely blocked
                        case 3:    // falling snow
                            ycheck = (ptparticle[p].y / ys);
                            if (size == 1)
                                ycheck++;
                            else if (size < 4)
                                ycheck += 2;
                            else
                                ycheck += ((size / 2) + 1);
                            target = 0;
                            offset = ycheck * fromByteCount + sx;
                            if (ycheck < prefs.winysize - 1)
                                target = ((unsigned char)fromBasePtr[offset] > curSet.pg_solid[g]);
                            if (target) {
                                occ[0] = occ[2] = 0;
                                if (sx > size) {
                                    occ[0] = ((unsigned char)fromBasePtr[offset - size] > curSet.pg_solid[g]);
                                }
                                if (sx < prefs.winxsize - size) {
                                    occ[2] = ((unsigned char)fromBasePtr[offset + size] > curSet.pg_solid[g]);
                                }

                                if (!occ[0] && occ[2])
                                    x -= (xs * size);    // roll to the left
                                if (occ[0] && !occ[2])
                                    x += (xs * size);    // roll to the right
                                if (occ[0] && occ[2]) {
                                    y = oy;
                                }    // just sit there.
                                // if clear on left and right, randomly pick a direction.
                                if (!occ[0] && !occ[2]) {
                                    if (Random() % 1) {
                                        x += xs;
                                    } else {
                                        x -= xs;
                                    }
                                }
                            }
                            break;
                        default:    // standard particle handling
                            target = ((unsigned char)fromBasePtr[sy * fromByteCount + sx] > curSet.pg_solid[g]);
                            // occ values (actual pixel is # 4)
                            // 0 1 2
                            // 3 4 5
                            // 6 7 8
                            if (target) {   // there's something where particle wants to be
                                occ[0] = occ[1] = occ[2] = occ[3] = occ[4] = occ[5] = occ[6] = occ[7] = occ[8] = 0;
                                // fx & fy represent the difference between current loc and desired loc.
                                fx = x - ox;
                                fy = y - oy;
                                // halve the distance until we're less than a screen pixel of difference.
                                while (fx > xs || fx < -xs || fy > ys || fy < -ys) {
                                    fx *= .5;
                                    fy *= .5;
                                }
                                dugout = false;
                                while (!dugout && (x != ox && y != oy)) {
                                    short screenx = x / xs;
                                    short screeny = y / ys;
                                    target = ((unsigned char)fromBasePtr[screeny * fromByteCount + screenx] > curSet.pg_solid[g]);
                                    if (target) {
                                        x -= fx;
                                        y -= fy;
                                    } else {
                                        dugout = true;
                                    }
                                }
                                // determine what the pixel's surroundings are in order to determine bounce
                                offset = ((short)(y / ys) - 1) * fromByteCount + (short)(x / xs) - 1;
                                if ((y / ys) >= 1) {
                                    if ((x / xs) >= 1)
                                        occ[0] = ((unsigned char)fromBasePtr[offset] > curSet.pg_solid[g]);
                                    occ[1] = ((unsigned char)fromBasePtr[offset + 1] > curSet.pg_solid[g]);
                                    if ((x / xs) < prefs.winxsize - 2)
                                        occ[2] = ((unsigned char)fromBasePtr[offset + 2] > curSet.pg_solid[g]);
                                }
                                offset += fromByteCount;
                                if ((x / xs) >= 1)
                                    occ[3] = ((unsigned char)fromBasePtr[offset] > curSet.pg_solid[g]);
                                occ[4] = ((unsigned char)fromBasePtr[offset + 1] > curSet.pg_solid[g]);
                                if ((x / xs) < prefs.winxsize - 2)
                                    occ[5] = ((unsigned char)fromBasePtr[offset + 2] > curSet.pg_solid[g]);
                                if ((y / ys) < prefs.winysize - 2) {
                                    offset += fromByteCount;
                                    if ((x / xs) >= 1)
                                        occ[6] = ((unsigned char)fromBasePtr[offset] > curSet.pg_solid[g]);
                                    occ[7] = ((unsigned char)fromBasePtr[offset + 1] > curSet.pg_solid[g]);
                                    if ((x / xs) < prefs.winxsize - 2)
                                        occ[8] = ((unsigned char)fromBasePtr[offset + 2] > curSet.pg_solid[g]);
                                }
                                // a flat ceiling, reverse vertical direction
                                if (occ[0] && occ[1] && occ[2]) {
                                    if (ptparticle[p].dy < 0)
                                        ptparticle[p].dy = -ptparticle[p].dy * .75;
                                }
                                // a flat floor, reverse vertical direction
                                else if (occ[6] && occ[7] && occ[8]) {
                                    if (ptparticle[p].dy > 0)
                                        ptparticle[p].dy = -ptparticle[p].dy * .75;
                                }
                                // a flat left wall, reverse horizontal direction (at least 1 pixel movement to right)
                                else if (occ[0] && occ[3] && occ[6]) {
                                    ptparticle[p].dx = -ptparticle[p].dx * .75;
                                    if (ptparticle[p].dx < xs)
                                        ptparticle[p].dx = xs;
                                }
                                // a flat right wall, reverse horizontal direction (at least 1 pixel movement to left)
                                else if (occ[2] && occ[5] && occ[8]) {
                                    ptparticle[p].dx = -ptparticle[p].dx * .75;
                                    if (ptparticle[p].dx > -xs)
                                        ptparticle[p].dx = -xs;
                                }
                                // a \ diagonal, so reverse vert motion and kick to right (if necessary)
                                else if (occ[3] && occ[6] && occ[7]) {
                                    if (ptparticle[p].dy > 0)
                                        ptparticle[p].dy = -ptparticle[p].dy * .75;
                                    if (ptparticle[p].dx < xs)
                                        ptparticle[p].dx = xs;
                                    ptparticle[p].dx *= 2;
                                }
                                // a / diagonal, so reverse vert motion and kick to left (if necessary)
                                else if (occ[7] && occ[5] && occ[8]) {
                                    if (ptparticle[p].dy > 0)
                                        ptparticle[p].dy = -ptparticle[p].dy * .75;
                                    if (ptparticle[p].dx > -xs)
                                        ptparticle[p].dx = -xs;
                                    ptparticle[p].dx *= 2;
                                }
                                // another diagonal, so bounce reverse horiz & vert motion
                                else if ((occ[3] || occ[5]) && (occ[1] || occ[7])) {
                                    ptparticle[p].dx = -ptparticle[p].dx * .75;
                                    ptparticle[p].dy = -ptparticle[p].dy * .75;
                                }
                                // something else -- (throws up hands) just slow it down.
                                else {
                                    if (ptparticle[p].dx < -xs || ptparticle[p].dx > xs)
                                        ptparticle[p].dx *= .75;
                                    if (ptparticle[p].dy < -ys || ptparticle[p].dy > ys)
                                        ptparticle[p].dy *= .75;
                                }

                                // no matter what, if it's on another pixel make sure it at least moves 1 pixel upward
                                // and at least .5 pixel left or right
                                if (occ[7]) {
                                    if (ptparticle[p].dy > -ys)
                                        ptparticle[p].dy = -ys;
                                    if (ptparticle[p].dx > -50 && ptparticle[p].dx < 50) {   // ensure there is horiz movement
                                        if (Random() % 1)
                                            ptparticle[p].dx = -50;
                                        else
                                            ptparticle[p].dx = 50;
                                    }
                                }
                            }
                            break;
                    }
                }
                ptparticle[p].x = x;
                ptparticle[p].y = y;
            }
        }
    }
    UnlockAllWorlds();
    DrawParticles(false);    // draw everything but the snow
}

void DrawParticles(Boolean snowPass) {
    Rect offRect;
    float xs, ys;
    short brite, lastBrite = -1;
    short sx, sy, useWidth, useHeight;
    long p;
    char g;
    short size, psize[MAXGENERATORS];
    Boolean hasSnow = false, hasNonSnow = false;

    // first, determine if there are any snow / non-snow generators
    for (g = 0; g < MAXGENERATORS; g++) {
        if (curSet.pg_pbehavior[g]) {
            if (curSet.pg_pbehavior[g] == 3)
                hasSnow = true;
            else
                hasNonSnow = true;
            psize[g] = curSet.pg_size[g] / 2;
            if (!prefs.highQualityMode && !prefs.lowQualityMode)
                psize[g] /= 2;
            if (prefs.lowQualityMode)
                psize[g] /= 4;
            if (psize[g] < 2)
                psize[g] = 1;
        }
    }
    if (snowPass && !hasSnow)
        return;
    if (!snowPass && !hasNonSnow)
        return;

    if (!LockAllWorlds())
        return;
    GetPortBounds(offWorld, &offRect);
    useWidth = prefs.winxsize;
    useHeight = prefs.winysize;
    if (curSet.verticalMirror && curSet.constrainMirror)
        useWidth = (prefs.winxsize / 2) + 1;
    if (curSet.horizontalMirror && curSet.constrainMirror)
        useHeight = (prefs.winysize / 2) + 1;

    SaveContext();
    SetPortWindowPort(gMainWindow);
    SetGWorld(offWorld, nil);
    xs = (32768.0 / useWidth);
    ys = (32768.0 / useHeight);
    for (p = 0; p < maxparticles; p++) {
        g = ptparticle[p].gen - 1;
        if (ptparticle[p].gen) {
            Boolean isSnow = (curSet.pg_pbehavior[g] == 3);
            if (isSnow == snowPass) {
                size = psize[g];
                brite = (int)((float)(curSet.pg_brightness[g] & 0x00FF) * 2.55);
                if (brite > 255)
                    brite = 255;
                sx = (float)ptparticle[p].x / xs;
                sy = (float)ptparticle[p].y / ys;

                if (sy < 0)
                    sy = 0;
                if (sy >= offRect.bottom)
                    sy = offRect.bottom - 1;
                if (sx < 0)
                    sx = 0;
                if (sx >= offRect.right)
                    sx = offRect.right - 1;

                if (size < 2)
                    fromBasePtr[sy * fromByteCount + sx] = brite;
                else if (size < 3) {
                    fromBasePtr[sy * fromByteCount + sx] = brite;
                    if (sx < offRect.right - 1)
                        fromBasePtr[sy * fromByteCount + sx + 1] = brite;
                    if (sy < offRect.bottom - 1) {
                        fromBasePtr[(sy + 1) * fromByteCount + sx] = brite;
                        if (sx < offRect.right - 1)
                            fromBasePtr[(sy + 1) * fromByteCount + sx + 1] = brite;
                    }
                } else if (size < 4) {
                    Boolean doLeft = (sx > 0);
                    Boolean doRight = (sx < offRect.right - 1);
                    Boolean doTop = (sy > 0);
                    Boolean doBottom = (sy < offRect.bottom - 1);
                    fromBasePtr[sy * fromByteCount + sx] = brite;    // middle dot
                    if (doTop)
                        fromBasePtr[(sy - 1) * fromByteCount + sx] = brite;
                    if (doBottom)
                        fromBasePtr[(sy + 1) * fromByteCount + sx] = brite;
                    if (doRight) {
                        fromBasePtr[sy * fromByteCount + sx + 1] = brite;
                        if (doTop)
                            fromBasePtr[(sy - 1) * fromByteCount + sx + 1] = brite;
                        if (doBottom)
                            fromBasePtr[(sy + 1) * fromByteCount + sx + 1] = brite;
                    }
                    if (doLeft) {
                        fromBasePtr[sy * fromByteCount + sx - 1] = brite;
                        if (doTop)
                            fromBasePtr[(sy - 1) * fromByteCount + sx - 1] = brite;
                        if (doBottom)
                            fromBasePtr[(sy + 1) * fromByteCount + sx - 1] = brite;
                    }
                } else {
                    Rect rect;
                    if (brite != lastBrite) {
                        RGBColor color;
                        lastBrite = brite;
                        Index2Color(brite, &color);
                        RGBForeColor(&color);
                    }
                    rect.left = (ptparticle[p].x / xs) - (size / 2);
                    rect.top = (ptparticle[p].y / ys) - (size / 2);
                    rect.right = rect.left + size;
                    rect.bottom = rect.top + size;
                    PaintOval(&rect);
                }
            }
        }
    }
    UnlockAllWorlds();
    RestoreContext();
}

long LongAbsolute(long n) {
    if (n < 0)
        return -n;
    else
        return n;
}
