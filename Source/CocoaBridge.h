//
//  CocoaBridge.h
//  PixelToy
//
//  Created by Leon McNeill on 2/28/08.
//  Copyright 2008 LairWare Ltd. All rights reserved.
//

void CocoaAppLaunchInits(void);
void OpenResourceFile(void);
CFURLRef ResourcesDirectoryURL(void);
void GetDisplayModesForDisplay(CGDirectDisplayID dispID, CFArrayRef *displayModeStrings, CFArrayRef *displayModes);
CFDictionaryRef MatchingModeForDisplay(CFDictionaryRef desiredMode, CFArrayRef modeArray);
OSErr CopyResourceFileToSpec(CFStringRef resourceFilename, FSSpec *theSpec);
OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID);
short UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID);
OSErr GetAppSupportSpecForFilename(CFStringRef filename, FSSpec *theSpec, Boolean create);
CFStringRef CreatePOSIXPathFromFSSpec(FSSpec *theFSS);
CFStringRef CreateLogFileName(void);
void GetAppNameAndVersion(Str63 *theString);
CFStringRef CopyAppVersionString(void);
void GetAppCopyright(Str63 *theString);
Boolean FileHasSuffixOrType(FSSpec *theFSS, OSType theType);
OSStatus CocoaChooseFile(CFStringRef message, CFStringRef fileTypeOrExtension, FSSpec *theSpec);
OSStatus CocoaSaveFile(CFStringRef title, CFStringRef message, CFStringRef fileExtension, OSType creator, OSType fileType, FSSpec *theSpec);
void SleepForSeconds(float seconds);
