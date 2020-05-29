// PixelToyFilters.c headers

void ToggleFilter(short filter);
Boolean FilterActive(short filter);
void ClearAllFilters(void);
void ToggleMirror(short what);
void FadeEdges(void);
void DoFilter(short filterNum);
void CompileCustomFilter(void);
void DoCustomFilter(void);
void EditCustomFilter(void);
void SetDefaultCustomFilter(void);
Boolean DoLoadFilterDialog(void);
void LoadFilter(FSSpec fss);
Boolean DoSaveFilterDialog(void);
void SaveFilter(FSSpec fss);
long ShowFilterTotal(DialogPtr theDialog, short item);
void BlurToOffBlit(void);
void DoMirroring(void);
