Boolean InitializeAEHandlers(void);
static pascal OSErr AEOpenApp(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEOpenDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEPrintDocs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEQuit(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);
static pascal OSErr AEShowPrefs(const AppleEvent *event, AppleEvent *replyEvent, long handlerRefCon);