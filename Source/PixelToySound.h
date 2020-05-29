// Headers for PixelToySound.c
void OpenSoundIn(short devNum);
void CloseSoundIn(void);
OSErr SetSoundInputGain(int soundInRefNum, short intGain);
Ptr GetSoundInput16BitSamples(UInt32 desiredNumberOfSamples);
OSErr MaintainSoundInput(void);
void WaitUntilRecordingComplete(void);
short RecentVolume(void);
void PlaySound(short what,Boolean async);
void OpenChannel(void);
void CloseChannel(void);
void PrepareSoundInputDeviceMenu(ControlHandle theControl);
void PrepareSoundInputSourceMenu(ControlHandle theControl);
