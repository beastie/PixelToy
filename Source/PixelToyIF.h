// PixelToyIF headers

void OpenPrefs(void);
void GetPrefs(void);
void PutPrefs(void);
void ForceOnScreen(WindowPtr win);
void GetGlobalWindowRect(WindowPtr win, Rect *rect);
short GetWindowDevice(WindowPtr win);
void GetWindowDeviceRect(WindowPtr win, Rect *rect);
void PreferencesDialog(void);
void PrefsTabDisplay(DialogPtr theDialog, ControlHandle tabControl);
void PrefsEnableDisableControls(DialogPtr theDialog);
OSErr SwitchInResolution(void);
OSErr SwitchOutResolution(void);
void SetLivePrefsSliderValue(short item, short value);
Boolean QuickTimeOptionsDialog(void);
void MiscOptionsDialog(void);
void SetLiveMiscOptionSliderValue(short item, short value);
void ShowMiscOptionsValues(DialogPtr theDialog, Boolean clear);
void SetMiscOptionsDefaults(void);
void MySetWindowTitle(void);
void SanitizeString(Str255 str);
Boolean ControlIsPressed(void);
Boolean ShiftIsPressed(void);
Boolean OptionIsPressed(void);
Boolean CommandIsPressed(void);
Boolean CheckSystemRequirements(void);
void SetUpMacStuff(void);
static pascal Boolean VerifyResDialogFilter(DialogPtr theDlg, EventRecord *event, DialogItemIndex *itemHit);
void ConfigureFilter(short setOK, short setCancel);
void CreateDialogFilter(void);
pascal Boolean DialogFilter(DialogPtr theDlg, EventRecord *event, short *itemHit);
void UpdateSliderDisplay(DialogPtr theDialog);
void SetSliderActions(DialogPtr theDialog, short dialogID);
pascal void MySliderProc(ControlHandle ctrl, ControlPartCode partCode);
void SetLiveSliderValue(short dialogID, short item, short value);
void MyHideMenuBar(void);
void MyShowMenuBar(void);
void NotifyNoPaletteTransition(void);
Boolean NotifyReturningFullScreen(void);
void NotifyVirtualMemorySucks(void);
void WarnAboutCubeSound(void);
void WarnAboutOSXSoundInput(void);
Boolean WarnAbout256Colors(void);
Boolean ReplaceDialog(void);
SInt16 DoStandardAlert(AlertType type, short id);
Boolean EqualFSS(FSSpec *fss1, FSSpec *fss2);
Boolean IsMacOSX(void);
void MyDrawText(Str255 str, short x, short y, short brite);
Boolean MyIsMenuItemEnabled(MenuRef menu, MenuItemIndex item);
void MyDisableMenuItem(MenuRef theMenu, short item);
void MyEnableMenuItem(MenuRef theMenu, short item);
OSStatus MyValidWindowRect(WindowRef window, const Rect* bounds);
OSStatus MyInvalWindowRect(WindowRef window, const Rect* bounds);
Boolean GoodHandle(Handle h);
void DebugString(Str255 str);

