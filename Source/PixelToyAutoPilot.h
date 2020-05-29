// PixelToyAutoPilot.h

void BuildFontList(void);
void ConfigureAutopilotDialog(void);
void ShowAutoPilotSettings(DialogPtr theDialog);
void DoAutoPilot(Boolean force);
void SetAutoPilotDefaults(void);
void SetAPAction(char action, Boolean value);
Boolean GetAPAction(char action);
void SetAPOption(char action, Boolean value);
Boolean GetAPOption(char action);
void StopAutoChanges(void);
void ResumeAutoChanges(void);
