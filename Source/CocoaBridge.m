//
//  CocoaBridge.m
//  PixelToy
//
//  Created by Leon McNeill on 2/28/08.
//  Copyright 2008 LairWare Ltd. All rights reserved.
//

#import "CocoaBridge.h"
#include <Cocoa/Cocoa.h>

@interface PixelToyOpenPanelDelegate : NSObject <NSOpenSavePanelDelegate> {
    OSType mAllowedType;
    NSSet *mAllowedExtensions;
    NSMutableDictionary *mCachedHFSTypeByPath;
}

+ (PixelToyOpenPanelDelegate *)sharedInstance;

- (void)setAllowedType:(OSType)theType;
- (void)setAllowedExtensions:(NSArray *)extensions;
@end

@implementation PixelToyOpenPanelDelegate

+ (PixelToyOpenPanelDelegate *)sharedInstance {
    static PixelToyOpenPanelDelegate *sInstance = nil;
    if (!sInstance) {
        sInstance = [[PixelToyOpenPanelDelegate alloc] init];
        [sInstance setAllowedType:0];
    }
    return sInstance;
}

- (void)setAllowedType:(OSType)theType {
    mAllowedType = theType;
}

- (void)setAllowedExtensions:(NSArray *)extensions {
    NSMutableSet *mut = [NSMutableSet set];
    for (NSString *aString in extensions) {
        [mut addObject:[aString lowercaseString]];
    }

    [mAllowedExtensions release];
    mAllowedExtensions = [mut copy];
}

- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    if (mAllowedType == 0)
        return YES;

    if (![url isFileURL])
        return NO;

    NSString *asPath = [url path];

    // Always allow directories to be entered
    if ([[url absoluteString] hasSuffix:@"/"] && ![[NSWorkspace sharedWorkspace] isFilePackageAtPath:asPath])
        return YES;

    if ([mAllowedExtensions count]) {
        NSString *thisExt = [[asPath pathExtension] lowercaseString];
        if (thisExt && [mAllowedExtensions containsObject:thisExt])
            return YES;
    }

    NSNumber *thisType = [mCachedHFSTypeByPath objectForKey:asPath];
    if (!thisType) {
        if (!mCachedHFSTypeByPath) {
            mCachedHFSTypeByPath = [[NSMutableDictionary alloc] init];
        }
        thisType = [[[NSFileManager defaultManager] attributesOfItemAtPath:asPath error:NULL] objectForKey:NSFileHFSTypeCode];
        if (thisType) {
            [mCachedHFSTypeByPath setObject:thisType forKey:asPath];
        } else {
            [mCachedHFSTypeByPath setObject:[NSNumber numberWithBool:NO] forKey:asPath];
        }
    }

    return ([thisType unsignedLongValue] == mAllowedType);
}

@end

// --------------------------------------------------------------------------------------------------------
#pragma mark -

void CocoaAppLaunchInits(void) {
    NSString *helpPath = [[NSBundle mainBundle] pathForResource:@"PixelToy Help" ofType:@""];
    if (helpPath) {
        FSRef helpRef = {0};
        if (noErr == FSPathMakeRef((const UInt8 *)[helpPath UTF8String], &helpRef, NULL)) {
            AHRegisterHelpBook(&helpRef);
        }
    }
}

void OpenResourceFile(void) {
    CFURLRef resourcesURL = (CFURLRef)ResourcesDirectoryURL();
    NSArray *langs = [[[NSUserDefaults standardUserDefaults] objectForKey:@"AppleLanguages"] arrayByAddingObject:@"en"];

    Boolean success = false;
    int i = 0;
    while (!success && i < [langs count]) {
        NSString *aLang = [langs objectAtIndex:i++];
        if ([aLang isEqual:@"en"]) {
            aLang = @"English";
        } else if ([aLang isEqual:@"fr"]) {
            aLang = @"French";
        }
        NSString *checkName = [NSString stringWithFormat:@"%@.lproj/PixelToy.rsrc", aLang];
        CFURLRef ourResURL = CFURLCreateCopyAppendingPathComponent(nil, resourcesURL, (CFStringRef)checkName, false);
        FSRef fsr;
        if (CFURLGetFSRef(ourResURL, &fsr)) {
            ResFileRefNum resRefNum;
            OSErr err = FSOpenResourceFile(&fsr, 0, NULL, fsRdPerm, &resRefNum);
            success = (err == noErr);
        }
        CFRelease(ourResURL);
    }
}

CFURLRef ResourcesDirectoryURL(void) {
    static CFURLRef sResult = nil;
    if (!sResult) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        sResult = (CFURLRef)[[NSURL alloc] initFileURLWithPath:[[NSBundle mainBundle] resourcePath]];
        [pool release];
    }
    return sResult;
}

OSErr GetAppSupportSpecForFilename(CFStringRef filename, FSSpec *theSpec, Boolean create) {
    if (!theSpec || !filename) {
        return paramErr;
    }
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    FSVolumeRefNum appSuppVRefNum = 0;
    SInt32 appSuppDirID = 0;
    OSErr theErr = FindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &appSuppVRefNum, &appSuppDirID);
    if (noErr == theErr) {
        FSSpec appSuppSpec;
        theErr = FSMakeFSSpec(appSuppVRefNum, appSuppDirID, "\pPixelToy", &appSuppSpec);
        if (fnfErr == theErr) {
            SInt32 newDirID;
            theErr = FSpDirCreate(&appSuppSpec, smSystemScript, &newDirID);
        }
        if (noErr == theErr) {
            NSString *appSuppPath = [(NSString *)CreatePOSIXPathFromFSSpec(&appSuppSpec) autorelease];
            appSuppPath = [appSuppPath stringByAppendingPathComponent:(NSString *)filename];
            BOOL tempCreate = (![[NSFileManager defaultManager] fileExistsAtPath:appSuppPath]);
            if (tempCreate) {
                [[NSFileManager defaultManager] createFileAtPath:appSuppPath contents:[NSData data] attributes:nil];
            }
            CFURLRef asURL = CFURLCreateWithFileSystemPath(NULL, (CFStringRef)appSuppPath, kCFURLPOSIXPathStyle, false);
            if (asURL) {
                FSRef fsRef;
                if (CFURLGetFSRef(asURL, &fsRef)) {
                    theErr = FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, theSpec, NULL);
                }
                CFRelease(asURL);
            }
            if (tempCreate) {
                if (noErr == theErr) {
                    theErr = fnfErr;
                }
                if (!create) {
                    [[NSFileManager defaultManager] removeItemAtPath:appSuppPath error:nil];
                }
            }
        }
    }
    [pool release];
    return theErr;
}

void GetDisplayModesForDisplay(CGDirectDisplayID dispID, CFArrayRef *displayModeStrings, CFArrayRef *displayModes) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSArray *modes = (NSArray *)CGDisplayAvailableModes(dispID);
    NSMutableDictionary *uniqueModes = [NSMutableDictionary dictionary];
    NSMutableDictionary *uniqueModeStrings = [NSMutableDictionary dictionary];
    int i;
    for (i = [modes count]-1; i >= 0; i--) {
        NSDictionary *aMode = [modes objectAtIndex:i];
        int bpp = [[aMode objectForKey:@"BitsPerPixel"] intValue];
        if (bpp >= 24) {
            NSNumber *width = [aMode objectForKey:@"Width"];
            NSNumber *height = [aMode objectForKey:@"Height"];
            NSNumber *rate = [aMode objectForKey:@"RefreshRate"];
            int sortNumber = [width intValue] * 32768 + [height intValue] * 1000 + [rate intValue];
            NSString *key = [NSString stringWithFormat:@"%012d", sortNumber];
            NSString *string = [NSString stringWithFormat:@"%@ x %@", width, height];
            if ([rate intValue] > 0)
                string = [string stringByAppendingFormat:@" (%@ Hz)", rate];
            [uniqueModes setObject:aMode forKey:key];
            [uniqueModeStrings setObject:string forKey:key];
        }
    }
    // compare:options: NSNumericSearch
    NSArray *sortedKeys = [[uniqueModes allKeys] sortedArrayUsingSelector:@selector(compare:)];
    NSMutableArray *sortedStrings = [NSMutableArray array];
    NSMutableArray *sortedModes = [NSMutableArray array];
    for (i = 0; i < [sortedKeys count]; i++) {
        NSString *aKey = [sortedKeys objectAtIndex:i];
        [sortedModes addObject:[uniqueModes objectForKey:aKey]];
        [sortedStrings addObject:[uniqueModeStrings objectForKey:aKey]];
    }
    if (displayModeStrings) {
        *displayModeStrings = (CFArrayRef)[sortedStrings retain];
    }
    if (displayModes) {
        *displayModes = (CFArrayRef)[sortedModes copy];
    }
    [pool release];
}

CFDictionaryRef MatchingModeForDisplay(CFDictionaryRef desiredMode, CFArrayRef modeArray) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSDictionary *exactMode = nil;
    NSDictionary *closeMode = nil;
    NSNumber *desiredWidth = [(NSDictionary *)desiredMode objectForKey:@"Width"];
    NSNumber *desiredHeight = [(NSDictionary *)desiredMode objectForKey:@"Height"];
    NSNumber *desiredRate = [(NSDictionary *)desiredMode objectForKey:@"RefreshRate"];
    int i = [(NSArray *)modeArray count] - 1;
    while (!exactMode && i >= 0) {
        NSDictionary *aMode = [(NSArray *)modeArray objectAtIndex:i--];
        if ([[aMode objectForKey:@"BitsPerPixel"] intValue] >= 24 && [desiredWidth isEqual:[aMode objectForKey:@"Width"]] &&
            [desiredHeight isEqual:[aMode objectForKey:@"Height"]]) {
            if ([desiredRate isEqual:[aMode objectForKey:@"RefreshRate"]]) {
                exactMode = aMode;
            } else {
                closeMode = aMode;
            }
        }
    }
    [pool release];
    if (exactMode) {
        return (CFDictionaryRef)exactMode;
    }
    return (CFDictionaryRef)closeMode;
}

OSErr CopyResourceFileToSpec(CFStringRef resourceFilename, FSSpec *theSpec) {
    OSErr theErr = paramErr;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *filename = (NSString *)resourceFilename;
    NSString *defaultPath =
        [[NSBundle mainBundle] pathForResource:[filename stringByDeletingPathExtension] ofType:[filename pathExtension]];
    if (theSpec && [defaultPath length] && [[NSFileManager defaultManager] fileExistsAtPath:defaultPath]) {
        NSString *destination = [(NSString *)CreatePOSIXPathFromFSSpec(theSpec) autorelease];
        [[NSFileManager defaultManager] removeItemAtPath:destination error:nil];
        if ([[NSFileManager defaultManager] copyPath:defaultPath toPath:destination handler:nil]) {
            theErr = noErr;
        }
    }
    [pool release];
    return theErr;
}

OSStatus UDrawThemePascalString(ConstStr255Param inPString, ThemeFontID inFontID) {
    OSStatus error = paramErr;

    if (inPString != NULL) {
        CFStringRef cfstring = CFStringCreateWithPascalString(kCFAllocatorDefault, inPString, CFStringGetSystemEncoding());
        // or kCFStringEncodingMacRoman?
        if (cfstring) {
            // measure so we can figure out the bounding box based
            // on the current pen and text metrics

            Point dimensions = {0, 0};    // will have height and width
            SInt16 baseline;              // will be negative
            Point pen;
            Rect bounds;

            GetThemeTextDimensions(cfstring, inFontID, kThemeStateActive, false, &dimensions, &baseline);

            GetPen(&pen);

            bounds.left = pen.h;
            bounds.bottom = pen.v - baseline;    // baseline is negative value
            bounds.right = bounds.left + dimensions.h + 1;
            bounds.top = bounds.bottom - dimensions.v;

            error = DrawThemeTextBox(cfstring, inFontID, kThemeStateActive, false, &bounds, teFlushDefault, NULL);

            CFRelease(cfstring);
        }
    }

    return error;
}

short UThemePascalStringWidth(ConstStr255Param inPString, ThemeFontID inFontID) {
    if (inPString != NULL) {
        CFStringRef cfstring = CFStringCreateWithPascalString(kCFAllocatorDefault, inPString, CFStringGetSystemEncoding());
        // or kCFStringEncodingMacRoman?
        if (cfstring) {
            // measure so we can figure out the bounding box based
            // on the current pen and text metrics

            Point dimensions = {0, 0};    // will have height and width
            SInt16 baseline;              // will be negative
            short result;

            GetThemeTextDimensions(cfstring, inFontID, kThemeStateActive, false, &dimensions, &baseline);

            result = dimensions.h + 1;
            CFRelease(cfstring);
            return result;
        }
    }

    return 0;
}

void GetAppNameAndVersion(Str63 *theString) {
    static Str63 sResult = {0};
    if (sResult[0] == 0) {
        NSString *resultStr =
            [NSString stringWithFormat:@"%@ %@ (%@)", [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleExecutable"],
                                       [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleShortVersionString"],
                                       [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"]];
        [resultStr getCString:(char *)sResult + 1 maxLength:60 encoding:NSMacOSRomanStringEncoding];
        sResult[0] = strlen((char *)sResult + 1);
    }
    if (sResult[0])
        memcpy(theString, sResult, sResult[0] + 1);
}

CFStringRef CreateLogFileName(void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *result = [[NSString alloc]
        initWithFormat:@"Log-%@-(%@).txt", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"],
                       [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"], nil];
    [pool release];
    return (CFStringRef)result;
}

CFStringRef CopyAppVersionString(void) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSString *result = [[NSString alloc]
        initWithFormat:@"%@ (%@)", [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleShortVersionString"],
                       [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"], nil];
    [pool release];
    return (CFStringRef)result;
}

void GetAppCopyright(Str63 *theString) {
    static Str63 sResult = {0};
    if (sResult[0] == 0) {
        NSString *resultStr = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleGetInfoString"];
        [resultStr getCString:(char *)sResult + 1 maxLength:60 encoding:NSMacOSRomanStringEncoding];
        sResult[0] = strlen((char *)sResult + 1);
    }
    if (sResult[0])
        memcpy(theString, sResult, sResult[0] + 1);
}

CFStringRef CreatePOSIXPathFromFSSpec(FSSpec *theFSS) {
    CFStringRef result = NULL;
    FSRef fsRef = {0};
    if (noErr == FSpMakeFSRef(theFSS, &fsRef)) {
        CFURLRef urlRef = CFURLCreateFromFSRef(NULL, &fsRef);
        if (urlRef) {
            result = CFURLCopyFileSystemPath(urlRef, kCFURLPOSIXPathStyle);
            CFRelease(urlRef);
        }
    }
    return result;
}

OSErr GetFSSpecFromPOSIXPath(CFStringRef thePath, FSSpec *theFSS) {
    OSErr result = paramErr;
    if (thePath && theFSS) {
        CFURLRef urlRef = CFURLCreateWithFileSystemPath(NULL, thePath, kCFURLPOSIXPathStyle, false);
        if (urlRef) {
            FSRef fsRef = {0};
            Boolean found = CFURLGetFSRef(urlRef, &fsRef);
            CFRelease(urlRef);
            if (found)
                result = FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, NULL, theFSS, NULL);
        }
    }

    return result;
}

CFStringRef CreateFilenameFromFSSpec(FSSpec *theFSS) {
    FSRef fsRef = {0};
    OSErr theErr = FSpMakeFSRef(theFSS, &fsRef);
    if (noErr == theErr) {
        HFSUniStr255 unicodeStr = {0, {0}};
        theErr = FSGetCatalogInfo(&fsRef, kFSCatInfoNone, NULL, &unicodeStr, NULL, NULL);
        if (noErr == theErr)
            return CFStringCreateWithCharacters(nil, unicodeStr.unicode, unicodeStr.length);
    }

    return nil;
}

CFStringRef CreateStringFromOSType(OSType theType) {
    OSType endianCorrectType = EndianU32_NtoB(theType);
    CFStringRef result =
        CFStringCreateWithBytes(nil, (UInt8 *)&endianCorrectType, sizeof(OSType), kCFStringEncodingMacRoman, false);
    return result;
}

OSType GetOSTypeFromString(CFStringRef theStringRef) {
    if (!theStringRef || CFStringGetLength(theStringRef) < 4)
        return 0;
    unsigned char c1 = CFStringGetCharacterAtIndex(theStringRef, 0);
    unsigned char c2 = CFStringGetCharacterAtIndex(theStringRef, 1);
    unsigned char c3 = CFStringGetCharacterAtIndex(theStringRef, 2);
    unsigned char c4 = CFStringGetCharacterAtIndex(theStringRef, 3);
    return (c1 << 24 | c2 << 16 | c3 << 8 | c4);
}

Boolean FileHasSuffixOrType(FSSpec *theFSS, OSType theType) {
    Boolean result = false;

    FInfo finfo;
    OSErr getTypeErr = FSpGetFInfo(theFSS, &finfo);
    if (noErr == getTypeErr && finfo.fdType == theType)
        result = true;

    if (!result) {
        NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
        NSString *filename = [(NSString *)CreateFilenameFromFSSpec(theFSS) autorelease];
        NSString *typeString = [(NSString *)CreateStringFromOSType(theType) autorelease];
        if ([[filename lowercaseString] hasSuffix:[NSString stringWithFormat:@".%@", [typeString lowercaseString]]])
            result = true;
        [pool release];
    }

    return result;
}

OSStatus CocoaChooseFile(CFStringRef message, CFStringRef fileTypeOrExtension, FSSpec *theSpec) {
    OSStatus theErr = paramErr;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSOpenPanel *thePanel = [NSOpenPanel openPanel];
    [thePanel setCanChooseFiles:YES];
    [thePanel setCanChooseDirectories:NO];
    if (fileTypeOrExtension) {
        [[PixelToyOpenPanelDelegate sharedInstance] setAllowedType:GetOSTypeFromString(fileTypeOrExtension)];
        [[PixelToyOpenPanelDelegate sharedInstance] setAllowedExtensions:[NSArray arrayWithObject:(NSString *)fileTypeOrExtension]];
    } else
        [[PixelToyOpenPanelDelegate sharedInstance] setAllowedType:0];
    [thePanel setDelegate:[PixelToyOpenPanelDelegate sharedInstance]];
    [thePanel setAllowsMultipleSelection:NO];
    [thePanel setMessage:NSLocalizedString((NSString *)message, nil)];
    int returnCode = [thePanel runModal];
    if (returnCode == NSCancelButton)
        theErr = userCanceledErr;
    else if (returnCode == NSOKButton) {
        NSString *newFile = [[thePanel filenames] lastObject];
        if (![newFile length])
            NSBeep();
        else
            theErr = GetFSSpecFromPOSIXPath((CFStringRef)newFile, theSpec);
    }
    [pool release];
    return theErr;
}

OSStatus CocoaSaveFile(CFStringRef title, CFStringRef message, CFStringRef fileExtension, OSType creator, OSType fileType,
                       FSSpec *theSpec) {
    OSStatus theErr = paramErr;
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    NSSavePanel *thePanel = [NSSavePanel savePanel];
    if (fileExtension)
        [thePanel setNameFieldStringValue:[NSString stringWithFormat:@"%@.%@", NSLocalizedString(@"Untitled", nil), fileExtension]];
    [thePanel setCanSelectHiddenExtension:YES];
    [thePanel setExtensionHidden:YES];
    if (title)
        [thePanel setTitle:NSLocalizedString((NSString *)title, nil)];
    if (message)
        [thePanel setMessage:NSLocalizedString((NSString *)message, nil)];
    if (fileExtension)
        [thePanel setAllowedFileTypes:[NSArray arrayWithObject:(NSString *)fileExtension]];
    int returnCode = [thePanel runModal];
    if (returnCode == NSCancelButton)
        theErr = userCanceledErr;
    else if (returnCode == NSOKButton) {
        NSURL *theURL = [thePanel URL];
        if (theURL && [theURL isFileURL]) {
            NSString *asPath = [theURL path];
            NSMutableDictionary *attribsDict = [NSMutableDictionary dictionary];
            if ([thePanel isExtensionHidden])
                [attribsDict setObject:[NSNumber numberWithBool:YES] forKey:NSFileExtensionHidden];
            if (creator)
                [attribsDict setObject:[NSNumber numberWithUnsignedLong:creator] forKey:NSFileHFSCreatorCode];
            if (fileType)
                [attribsDict setObject:[NSNumber numberWithUnsignedLong:fileType] forKey:NSFileHFSTypeCode];
            if ([[NSFileManager defaultManager] createFileAtPath:asPath contents:[NSData data] attributes:attribsDict])
                theErr = GetFSSpecFromPOSIXPath((CFStringRef)asPath, theSpec);
            else
                theErr = fLckdErr;
        }
    }
    [pool release];
    return theErr;
}

void SleepForSeconds(float seconds) {
    [NSThread sleepForTimeInterval:seconds];
}
