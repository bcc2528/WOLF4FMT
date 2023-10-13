//
//	ID Engine
//	ID_IN.h - Header file for Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

#ifndef	__ID_IN__
#define	__ID_IN__

#ifdef	__DEBUG__
#define	__DEBUG_InputMgr__
#endif

#define	MaxPlayers	4
#define	MaxKbds		2
#define	MaxJoys		2
#define	NumCodes	128

typedef	byte		ScanCode;
#define	sc_None			0
#define	sc_Bad			0xff
#define	sc_Return		0x1d
#define	sc_Enter		sc_Return
#define	sc_Escape		0x7c
#define	sc_Space		0x35
#define	sc_BackSpace	0x0f
#define	sc_Tab			0x10
#define	sc_Alt			0x5c
#define	sc_Control		0x52
#define	sc_CapsLock		0x55
#define	sc_LShift		0x53
#define	sc_RShift		0x53
#define	sc_UpArrow		0x4d
#define	sc_DownArrow	0x50
#define	sc_LeftArrow	0x4f
#define	sc_RightArrow	0x51
#define	sc_Insert		0x48
#define	sc_Delete		0x4b
#define	sc_Home			0x4e
#define	sc_End			0x71
#define	sc_PgUp			0x70
#define	sc_PgDn			0x6e
#define	sc_F1			0x5d
#define	sc_F2			0x5e
#define	sc_F3			0x5f
#define	sc_F4			0x60
#define	sc_F5			0x61
#define	sc_F6			0x62
#define	sc_F7			0x63
#define	sc_F8			0x64
#define	sc_F9			0x65
#define	sc_F10			0x66
#define	sc_F11			0x69
#define	sc_F12			0x5b

#define	sc_1			0x02
#define	sc_2			0x03
#define	sc_3			0x04
#define	sc_4			0x05
#define	sc_5			0x06
#define	sc_6			0x07
#define	sc_7			0x08
#define	sc_8			0x09
#define	sc_9			0x0a
#define	sc_0			0x0b

#define	sc_A			0x1e
#define	sc_B			0x2e
#define	sc_C			0x2c
#define	sc_D			0x20
#define	sc_E			0x13
#define	sc_F			0x21
#define	sc_G			0x22
#define	sc_H			0x23
#define	sc_I			0x18
#define	sc_J			0x24
#define	sc_K			0x25
#define	sc_L			0x26
#define	sc_M			0x30
#define	sc_N			0x2f
#define	sc_O			0x19
#define	sc_P			0x1a
#define	sc_Q			0x11
#define	sc_R			0x13
#define	sc_S			0x1f
#define	sc_T			0x15
#define	sc_U			0x17
#define	sc_V			0x2d
#define	sc_W			0x12
#define	sc_X			0x2b
#define	sc_Y			0x16
#define	sc_Z			0x2a

#define	key_None		0
#define	key_Return		0x1d
#define	key_Enter		key_Return
#define	key_Escape		0x7c
#define	key_Space		0x35
#define	key_BackSpace		0x0f
#define	key_Tab			0x10
#define	key_Delete		0x4b

// 	Stuff for the mouse
#define	MReset		0
#define	MButtons	3
#define	MDelta		11

#define	MouseInt	0x33
//#define	Mouse(x)	_AX = x,geninterrupt(MouseInt)

typedef	enum		{
						demo_Off,demo_Record,demo_Playback,demo_PlayDone
					} Demo;
typedef	enum		{
						ctrl_Keyboard,
							ctrl_Keyboard1 = ctrl_Keyboard,ctrl_Keyboard2,
						ctrl_Joystick,
							ctrl_Joystick1 = ctrl_Joystick,ctrl_Joystick2,
						ctrl_Mouse
					} ControlType;
typedef	enum		{
						motion_Left = -1,motion_Up = -1,
						motion_None = 0,
						motion_Right = 1,motion_Down = 1
					} Motion;
typedef	enum		{
						dir_North,dir_NorthEast,
						dir_East,dir_SouthEast,
						dir_South,dir_SouthWest,
						dir_West,dir_NorthWest,
						dir_None
					} Direction;
typedef	struct		{
						boolean		button0,button1,button2,button3;
						short			x,y;
						Motion		xaxis,yaxis;
						Direction	dir;
					} CursorInfo;
typedef	CursorInfo	ControlInfo;
typedef	struct		{
						ScanCode	button0,button1,
									upleft,		up,		upright,
									left,				right,
									downleft,	down,	downright;
					} KeyboardDef;
typedef	struct		{
						word		joyMinX,joyMinY,
									threshMinX,threshMinY,
									threshMaxX,threshMaxY,
									joyMaxX,joyMaxY,
									joyMultXL,joyMultYL,
									joyMultXH,joyMultYH;
					} JoystickDef;
// Global variables
extern	boolean			Keyboard[],
				JoysPresent[];
extern	boolean			MousePresent;
extern	boolean			Paused;
extern	char			LastASCII;
extern	volatile ScanCode	LastScan;
extern	KeyboardDef		KbdDefs;
extern	JoystickDef		JoyDefs[];
extern	ControlType		Controls[MaxPlayers];

extern	Demo			DemoMode;
extern	byte 			*DemoBuffer; // _seg
extern	word			DemoOffset,DemoSize;

extern union REGS		regs;


// Function prototypes
#define	IN_KeyDown(code)	(Keyboard[(code)])
#define	IN_ClearKey(code)	{Keyboard[code] = false;\
							if (code == LastScan) LastScan = sc_None;}

// DEBUG - put names in prototypes
extern	void		IN_Startup(void),IN_Shutdown(void);
extern	void		IN_SetKeyHook(void (*)());
extern	void		IN_ClearKeysDown(void);
extern	void		IN_ReadCursor(CursorInfo *);
extern	void		IN_ReadControl(int,ControlInfo *);
extern	void		IN_GetJoyAbs(word joy,word *xp,word *yp);
extern	void		IN_SetupJoy(word joy,word minx,word maxx,
								word miny,word maxy);
extern	void		IN_StopDemo(void),IN_FreeDemoBuffer(void),
					IN_Ack(void),IN_AckBack(void);
extern	boolean		IN_UserInput(longword delay);
extern	char		*IN_GetScanName(ScanCode);

void	IN_MouseMove (int *dx, int *dy);
void	IN_MouseClear (void);
byte	IN_MouseButtons (void);
byte	IN_JoyButtons (void);

void INL_GetJoyDelta(word joy,int *dx,int *dy);
void IN_StartAck(void);
boolean IN_CheckAck (void);

#endif
