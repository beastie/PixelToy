// CarbonShunt.h

Rect* GetPortBounds(CGrafPtr port, Rect* rect);
Rect* GetWindowPortBounds(WindowRef window, Rect *bounds);
void SetPortDialogPort(DialogRef dialog);
CGrafPtr GetDialogPort(DialogRef dialog);
BitMap* GetQDGlobalsScreenBits(BitMap*  screenBits);
const BitMap* GetPortBitMapForCopyBits(CGrafPtr port);
Cursor* GetQDGlobalsArrow(Cursor* arrow);
RGBColor* GetPortForeColor(CGrafPtr port, RGBColor* foreColor);
RGBColor* GetPortBackColor(CGrafPtr port, RGBColor* backColor);
OSErr AEGetDescData(const AEDesc* theAEDesc, void* dataPtr, Size maximumSize);
MenuRef GetControlPopupMenuHandle(ControlRef control);
void SetQDGlobalsRandomSeed(long randomSeed);
RgnHandle GetPortVisibleRegion(CGrafPtr port, RgnHandle visRgn);
