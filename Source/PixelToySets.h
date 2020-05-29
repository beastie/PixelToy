// PixelToy Set function headers

Boolean GetAutoInterval(void);
void TimedSetChange(void);
void RenameOldSet(short preselected);
void EditSetComments(void);
void ZeroSettings(void);
void GetAndUsePixelToySets(void);
void LoadPixelToySet(void *si, short set);
void UsePixelToySet(short);
void InsertNewSetDialog(void);
void DoInsertNewSet(void *newSet);
void DeleteOldSetDialog(void);
void DoDeleteOldSet(short set);
void LoadDefaultSets(void);
void GetPixelToySets(FSSpec	spec);
void PutPixelToySets(void);
void SaveCurrentPixelToySets(void);
void CreateSetsMenu(void);
short ChooseSet(void);
void CurSetTouched(void);
void SaveSetsAs(void);
Boolean SetUsesSoundInput(void *theSet);
void UpdateCurrentSetNeedsSoundInputMessage(void);
