// PixelToyPalette.h

void GetDefaultPalettes			(void);
OSErr GetPalettes				(FSSpecPtr fss);
void PutPalettes				(void);
void InitializePalette			(void);
void PaletteTransitionUpdate	(void);
void EnsureSystemPalette		(void);
void PushPal					(void);
void PopPal						(void);
void RandomPalette				(Boolean singleHue);
void EmployPalette				(Boolean instant);
void UseColorPreset				(short theItem, Boolean instantly);

void PopulateColorsMenu			(void);
void MarkCurrentPalette			(void);

void DoAddPaletteDialog			(void);
void DeletePaletteDialog		(void);
void DoRenamePaletteDialog		(short preselected);

void ResChangeBitsPush			(void);
void ResChangeBitsPop			(void);
void ResChangeBitsRelease		(void);
