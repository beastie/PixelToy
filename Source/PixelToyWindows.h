// PixelToyWindows.h

void PTInitialWindows(void);
void PTTerminateWindows(void);
void PushWindows(void);
void PopWindows(void);
void HandleWindowChoice(short theItem);
void HandleWindowMouseDown(short winNum);
void PTResetWindows(void);
void PTCreateWindow(short winNum);
void PTDestroyWindow(short winNum);
void PTInvalidateWindow(short winNum);
void PTUpdateWindow(short winNum);
void PTDrawWindow(short winNum);
void HandleWindowDrag(WindowPtr window, Point point);
void MagneticWindows(short oldx, short oldy);
void HandleEditMouseDown(void);
short ColorIndicated(void);
void MyGetColor(RGBColor *color, short index);
void MySetColor(RGBColor *color, short index);
Boolean ChooseRGBColor(RGBColor* orgColor, RGBColor* newColor);
void UpdateEditColors(short firstSel, short lastSel);
void UpdateActionButtons(void);
void HandleActionMouseDown(void);
void UpdateFilterButtons(void);
void HandleFilterMouseDown(void);
void UpdateColorButtons(void);
void HandleColorMouseDown(void);
void UpdateSetButtons(void);
void HandleSetMouseDown(void);
void UpdateOptionButtons(void);
void HandleOptionMouseDown(void);
Boolean ButtonPressed(Rect* rect);
