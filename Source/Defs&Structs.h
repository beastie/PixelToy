// PixelToy.c headers
#ifndef __DEFSANDSTRUCTS__
#define __DEFSANDSTRUCTS__

#if TARGET_CPU_X86 || TARGET_CPU_X86_64 || TARGET_OS_IPHONE
	#define USE_GCD			1
#else
	#define USE_GCD			0
#endif

// Menu defines
#define APPLEMENU		128
#define  ABOUTID		1
#define FILEMENU		129
#define  OPENSETSID		1
#define  OPENID			2
#define  RELOADID		3
#define	 SAVEID			4
#define  SAVESETSID		5
#define	 EXPORTQTID		6
#define	 CLEARID		7
#define	 PAUSEID		8
#define  CLOSEWINDOWID	9
#define  QUITID			11
#define OPTIONSMENU		130
#define  AUTOPILOTID	1
#define  LOWQUALITYID	2
#define  HIGHQUALITYID	3
#define  FULLSCREENID	4
#define  DITHERID		5
#define  CONFIGPILOTID	6
#define  PREFSID		7
#define  AFTEREFFECTID	9
#define	 EMBOSSID		10
#define  WAVEOPTSID		11
#define  TEXTOPTSID		12
#define  PARTICOPTID	13
#define  BITMAPOPTID	14
#define  VARID			15
#define ACTIONSMENU		131
#define  BOUNCINGID		1
#define  RANDOMWALKID	2
#define  SWARMID		3
#define  RAINID			4
#define  SOUNDWAVEID	5
#define  DOODLEID		6
#define  TEXTID			7
#define  PARTICLEID		8
#define  IMAGEID		9
#define FILTERSMENU		132
#define  CLEARFILTERS	1
#define  EDITCUSTOM		2
#define  FIRSTFILTER	4
#define  HORIZMIRROR	30
#define  VERTMIRROR		31
#define  CONSTRAINMIRROR	32
#define COLORSMENU			133
#define	 RANDPALETTEID		1
#define  ADDPALETTEID		2
#define	 DELPALETTEID		3
#define  RENAMEPALETTEID	4
#define  PREVPALETTEID		5
#define  NEXTPALETTEID		6
#define  NOAUDIOCOLOR		8
#define  AUDIOBLACK			9
#define  AUDIOWHITE			10
#define  AUDIOINVERT		11
#define  AUDIOROTATE		12
#define  FIRSTPALETTEID		14
#define SETSMENU		134
#define  ADDSETID		1
#define  DELSETID		2
#define  RENAMESETID	3
#define  EDITCOMMENTSID	4
#define  ZEROSETTINGSID	5
#define  TIMESETID		6
#define  PREVSETID		7
#define  NEXTSETID		8
#define WINDOWMENU		135
#define  SHOWHIDEALLID	1
#define  MOVESIZEGROUP	2
#define  DEFAULTPOSNS	3
#define  ACTIONWINID	5
#define  FILTERWINID	6
#define  COLORSWINID	7
#define  COLOREDWINID	8
#define  SETSWINID		9
#define  OPTIONSWINID	10

//Misc defines
#define BASERES			400
#define PIXELTOYVERSIONBIGENDIAN	3
#define FILTERVERSION	42
#define ACC				16
#define FRAMESIZE		0
#define MAXGENERATORS	16
#define MAXTEXTS		16
#define MAXIMAGES		16
#define MAXWAVES		16
#define ABOUTWIDTH		472
#define ABOUTHEIGHT		300
#define MAXMON 100

// Destroy/Setup bitmap worlds
#define SB_BOTH				0
#define SB_IMAGE			1
#define SB_MASK				2

// Autopilot defines
#define AUTOPILOTVERSION	3

#define APN_VERSION				0
#define APN_NUMSECONDS			1
#define APN_ACTIONSCHANGE		2
#define APN_OPTIONSCHANGE		3
#define APB_ACTIONS				0
#define APB_OPTIONS				1
#define APB_ACTSOUND			2
#define APB_ALWAYSSOUND			3
#define APB_FILTERS				4
#define APB_RANDCOLORS			5
#define APB_SAVEDCOLORS			6
#define APB_MIRRORS				7

// Dialog defines
#define DLG_AUTOPILOT			BASERES+3
#define DLG_NOPALTRANS			BASERES+4
#define DLG_QUICKTIMEOPTS		BASERES+6
#define DLG_RETURNFULLSCREEN	BASERES+7
#define DLG_VMWARNING			BASERES+8
#define DLG_NAMESET				BASERES+9
#define DLG_DELSET				BASERES+10
#define DLG_REPLACE				BASERES+11
#define DLG_GETINTERVAL			BASERES+12
#define DLG_PARTICLEDITOR		BASERES+13
#define DLG_IMAGEEDITOR			BASERES+14
#define DLG_REGISTRATION		BASERES+15
#define DLG_WAVEEDITOR			BASERES+16
#define DLG_RENAMEPALETTE		BASERES+19
#define DLG_TEXTEDITOR			BASERES+22
#define DLG_RENAMESET			BASERES+24
#define DLG_EDITSETCOMM			BASERES+25
#define DLG_PREFERENCES			BASERES+30
#define DLG_NAMEPALETTE			BASERES+31
#define DLG_SELCOLORSET			BASERES+32
#define DLG_MISCOPTIONS			BASERES+33
#define DLG_CUSTOMFILTER		BASERES+34
#define DLG_CUBEWARNING			BASERES+35
#define DLG_OSXSOUNDINPUT		BASERES+36
#define DLG_WARN256COLORS		BASERES+37
#define DLG_SETNEEDSSOUNDINPUT	BASERES+38

// Sound defines
#define ABOUTNOISE	BASERES
#define BALALAIKA	BASERES+1

struct PTPalette
{
	Str63		palname; // palname[63] = 1 means that the palentrys are little endian.
	RGBColor	palentry[256];
};

struct PTParticle	{ short x, y, dx, dy; char gen; };
struct PTLine		{ short x, y, dx, dy, weight, gravity; };
struct PTWalk		{ short x, y, dx, dy, size; };
struct PTBee		{ short x, y, dx, dy; };

struct PTTextGen
{
	short		fontID;
	Str63		fontName;
	Str63		string;
	short		size;
	Boolean		sizeReactSound;
	short		xpos;
	short		ypos;
	short		brightness;
	Boolean		brightReactSound;
	Boolean		brightPulse;
	short		behave;
	short		reserved1;
	short		reserved2;
};

#define IMAGEACTIVE	42
#define MOVIEACTIVE 43

struct PTImageGen
{
	short		active;
	FSSpec		imagefss;
	short		orgwidth;
	short		orgheight;
	short		action;
	short		actionVar[8];
	Rect		place;
	short		depth;
	Boolean		invert;
	Boolean		transparent;
	Boolean		drawAfterActions;
	// 1 bit options
	short		britemode; // 0=fixed 1=pulsing 2=sound
	short		brightness; // if fixed
	// 8 bit options
	FSSpec		maskfss;
	Boolean		invertMask;

	short		reserved1;
	short		reserved2;
};

struct PTWaveGen
{
	char		active;
	Str63		name;
	char		type;
	char		vertical;
	char		solid;
	char		horizflip;
	char		vertflip;
	short		thickness;
	short		spacing;
	short		brightness;
	short		action;
	short		actionVar[8];
	Rect		place;
	short		reserved1;
	short		reserved2;
};

#define MAXWINDOW		20
#define WINACTIONS		0
#define WINFILTERS		1
#define WINCOLORS		2
#define WINCOLORED		3
#define WINSETS			4
#define WINOPTIONS		5
// above palettes - below dialogs
#define WINMAINSTORED	7
#define WINMAIN			8
#define WINPREFS		9
#define WINMISCOPTS		10
#define WINTEXTOPTS		11
#define WINPARTICLEOPTS	12
#define WINIMAGEOPTS	13
#define WINWAVEOPTS		14
#define WINCUSTFILTER	15
#define WINAUTOPILOT	16
#define WINTIMEDSET		17

#define MAXACTION		32
#define ACTLINES		0
#define ACTBALLS		1
#define ACTSWARM		2
#define ACTDROPS		3
#define ACTSWAVE		4
#define ACTDOODLE		5
#define ACTTEXT			6
#define ACTPARTICLES	7
#define ACTIMAGES		8

#define FTV_SIZEDOWN	0
#define FTV_MOVE		1

#define PSMODE_NONE		0
#define PSMODE_BLACK	1
#define PSMODE_WHITE	2
#define PSMODE_INVERT	3
#define PSMODE_ROTATE	4

struct pixeltoyfilter
{
	unsigned char	fVal[256];
	long			target;
};

struct monitorcontextinfo
{
	short			h, v, depth;
	Fixed			refresh;
	Rect			bounds;
};

struct setinformation
{
	// global parameters
	Str63		windowTitle;
	Str255		setComment;
	Boolean		filter[256];
	Boolean		postFilter;
	Boolean		emboss;
	Boolean		sound;
	RGBColor	palentry[256];

	// custom filter data
	struct pixeltoyfilter	customfilter;
		
	// actions
	Boolean		action[MAXACTION];
	Boolean		actReactSnd[MAXACTION];
	short		actCount[MAXACTION];
	short		actSiz[MAXACTION];
	short		actSizeVar[MAXACTION];
	short		actMisc1[MAXACTION];
	short		actMisc2[MAXACTION];
	
	// used to be unusual action-specific parameters
	Boolean		reservedBool1;
	Boolean		reservedBool2;
	
	struct PTTextGen	textGen[MAXTEXTS];
	
	// Particle generator stuff that really should have been a struct array
	Str63		pg_name[MAXGENERATORS];
	short		pg_pbehavior[MAXGENERATORS];
	short		pg_genaction[MAXGENERATORS];
	short		pg_solid[MAXGENERATORS];
	short		pg_rate[MAXGENERATORS];
	short		pg_gravity[MAXGENERATORS];
	Boolean		pg_walls[MAXGENERATORS];
	short		pg_xloc[MAXGENERATORS];
	short		pg_xlocv[MAXGENERATORS];
	short		pg_yloc[MAXGENERATORS];
	short		pg_ylocv[MAXGENERATORS];
	short		pg_dx[MAXGENERATORS];
	short		pg_dxv[MAXGENERATORS];
	short		pg_dy[MAXGENERATORS];
	short		pg_dyv[MAXGENERATORS];
	short		pg_size[MAXGENERATORS];
	short		pg_brightness[MAXGENERATORS];
	short		pg_reserved2[MAXGENERATORS];
	
	struct PTImageGen image[MAXIMAGES];
	short		horizontalMirror;
	short		verticalMirror;
	short		constrainMirror;
	short		paletteSoundMode;
	struct PTWaveGen wave[MAXWAVES];
};

struct preferences
{
	// windowing information
	short		winxsize;
	short		winysize;
	short		storedxsize;
	short		storedysize;
	Point		windowPosition[MAXWINDOW];
	short		windowOpen[MAXWINDOW];
	short		visiblePrefsTab;
	Boolean		mainWindowZoomed;
    unsigned char unused1[33];
	Boolean		fullScreen;
	Boolean		notFirstFullScreen;
	Boolean		dither;
	Boolean		lowQualityMode;
	Boolean		highQualityMode;
	Boolean		speedLimited;
	short		FPSlimit;
	Boolean		showFPS;
	Boolean		autoSetChange;
	short		autoSetInterval;
	Boolean		notifyFontMissing;
	Boolean		noSplashScreen;
	Boolean		hideMouse;
	Boolean		noImageResize;
	Boolean		roughPalette;
	Boolean		useFirstSet;
	Boolean		showComments;
	short		palTransFrames;
	Boolean		autoPilotActive;
	short		autoPilotValue[MAXACTION];
	Boolean		autoPilotOn[MAXACTION];
	short		QTMode;
	short		QTIncludeAudio;
	char		unused2[8];
	Boolean		dragSizeAllWindows;
	Boolean		imagesNotProportional;
	Boolean		changeResolution;
	struct monitorcontextinfo	mon;
	Boolean		exchange0and255;
	short		soundInputDevice;
	short		soundInputSource;
	Boolean		soundInputPlayThru;
	Boolean		soundInputAutoGain;
	short		soundInputSoftwareGain;
	short		soundInputHardwareGain; // -50 - +50 = 0.5-1.5 Fixed
	Boolean		dontcomplainpaltrans;
	Boolean		dontcomplainfsstart;
	Boolean		dontcomplainaboutvm;
	Boolean		dontcomplainaboutcube;
	Boolean		dontcomplainaboutosxsi;
	Boolean		dontcomplainabout256;
};
#endif
