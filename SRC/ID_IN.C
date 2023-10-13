//
//	ID Engine
//	ID_IN.c - Input Manager
//	v1.0d1
//	By Jason Blochowiak
//

//
//	This module handles dealing with the various input devices
//
//	Depends on: Memory Mgr (for demo recording), Sound Mgr (for timing stuff),
//				User Mgr (for command line parms)
//
//	Globals:
//		LastScan - The keyboard scan code of the last key pressed
//		LastASCII - The ASCII value of the last key pressed
//	DEBUG - there are more globals
//

#include "wl_def.h"

#include <dos.h>

#include <snd.h>
#include <mos.h>
#include <his.h>

char	mwork[MosWorkSize];

#define Stacksize 1024
char stackArea[ Stacksize ];

#define KEYintNumber 1
#define KEYRegister  0x600

//
// joystick constants
//
#define	JoyScaleMax		32768
#define	JoyScaleShift	8
#define	MaxJoyValue		5000

/*
=============================================================================

					GLOBAL VARIABLES

=============================================================================
*/

//
// configuration variables
//
boolean			MousePresent;
boolean			JoysPresent[MaxJoys];

union REGS regs;


// 	Global variables
		boolean			Keyboard[NumCodes];
		boolean			Paused;
		char			LastASCII;
		volatile ScanCode	LastScan;

		KeyboardDef		KbdDefs = {0x56,0x35,0x6e,0x4d,0x70,0x4f,0x51,0x71,0x50,0x48};
		JoystickDef		JoyDefs[MaxJoys];
		ControlType		Controls[MaxPlayers];

		Demo			DemoMode = demo_Off;
		byte 			*DemoBuffer;	// _seg
		word			DemoOffset,DemoSize;

/*
=============================================================================

					LOCAL VARIABLES

=============================================================================
*/
static	byte        ASCIINames[] =		// Unshifted ASCII for scan codes
					{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 0 ,27 ,'1','2','3','4','5','6','7','8','9','0','-','^', 0,  8 ,	// 0
	 9 ,'q','w','e','r','t','y','u','i','o','p','@', 0 , 13,'a','s',	// 1
	'd','f','g','h','j','k','l',';',':', 0, 'z','x','c','v','b','n',	// 2
	'm',',','.','/', 0 ,' ','*','/','+','-','7','8','9','=','4','5',	// 3
	'6', 0 ,'1','2','3', 13,'0', 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 4
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 5
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 6
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0		// 7
					};
static byte ShiftNames[] =		// Shifted ASCII for scan codes
					{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	 0 ,27 ,'!', 34,'#','$','%','&', 39,'(',')', 0 ,'=','~','|', 8 ,	// 0
	 9 ,'Q','W','E','R','T','Y','U','I','O','P','`','{', 13,'A','S',	// 1
	'D','F','G','H','J','K','L','+','*','}','Z','X','C','V','B','N',	// 2
	'M','<','>','?','_',' ','*','/','+','-','7','8','9','=','4','5',	// 3
	 0,  0 ,'1','2','3', 13,'0', 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 4
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 5
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,	// 6
	 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0		// 7
					};
static byte SpecialNames[] =	// ASCII for 0xe0 prefixed codes
					{
//	 0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 0
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,13 ,0  ,0  ,0  ,	// 1
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 2
	0  ,0  ,0  ,0  ,0  ,'/',0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 3
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 4
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 5
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,	// 6
	0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0  ,0   	// 7
					};

static	boolean		IN_Started;
static	boolean		CapsLock;
static	ScanCode	CurCode,LastCode;

static	Direction	DirTable[] =		// Quick lookup for total direction
					{
						dir_NorthWest,	dir_North,	dir_NorthEast,
						dir_West,		dir_None,	dir_East,
						dir_SouthWest,	dir_South,	dir_SouthEast
					};

static	char			*ParmStrings_id_in[] = {"nojoys","nomouse",0};

boolean NumLockPanic=false;

byte scanbuffer[16];
int curscanoffs=0;


//	Internal routines

///////////////////////////////////////////////////////////////////////////
//
//	INL_KeyService() - Handles a keyboard interrupt (key up/down)
//
///////////////////////////////////////////////////////////////////////////
void INL_KeyService(void)
{
	static byte preCode;
	byte	k;
	char	c;

	k = _inb( KEYRegister );	// Get the scan code

	if(k & 0x80)
	{
		/* １バイト目のキーデータ */
		preCode = k;
		_enable();
		return;
	}

	curscanoffs=(curscanoffs+1)&15;
	scanbuffer[curscanoffs]=k;

	if((!(preCode & 0x10)) || ((preCode & 0xf0) == 0xf0))	// Make code
	{
		k &= 0x7f;
		CurCode = LastScan = k;
		Keyboard[k] = true;

		if (k == sc_CapsLock)
		{
			CapsLock ^= true;
		}

		if (Keyboard[sc_LShift])	// If shifted
		{
			c = ShiftNames[k];
			if ((c >= 'A') && (c <= 'Z') && CapsLock)
			c += 'a' - 'A';
		}
		else
		{
			c = ASCIINames[k];
			if ((c >= 'a') && (c <= 'z') && CapsLock)
				c -= 'a' - 'A';
		}

			if (c) LastASCII = c;
	}else			// Break code
	{
		Keyboard[k & 0x7f] = false;
	}
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_GetJoyAbs() - Reads the absolute position of the specified joystick
//
///////////////////////////////////////////////////////////////////////////
void
IN_GetJoyAbs(word joy,word *xp,word *yp)
{
	*xp = 0;
	*yp = 0;
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetJoyDelta() - Returns the relative movement of the specified
//		joystick (from +/-127)
//
///////////////////////////////////////////////////////////////////////////
void INL_GetJoyDelta(word joy,int *dx,int *dy)
{
	int x, y;
	static	longword	lasttime;
	int status;

	SND_joy_in_2(joy, &status);

	x = 0;
	y = 0;

	if(((~status) & 0x1) && !((~(status) >> 1) & 0x1))
	{
		y = -127;
	}
	if(((~(status) >> 1) & 0x1) && !((~status) & 0x1))
	{
		y = 127;
	}
	if(((~(status) >> 2) & 0x1) && !((~(status) >> 3) & 0x1))
	{
		x = -127;
	}
	if(((~(status) >> 3) & 0x1) && !((~(status) >> 2) & 0x1))
	{
		x = 127;
	}

	*dx = x;
	*dy = y;

	lasttime = TimeCount;
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_GetJoyButtons() - Returns the button status of the specified
//		joystick
//
///////////////////////////////////////////////////////////////////////////
static word
INL_GetJoyButtons(word joy)
{
	int status;

	SND_joy_in_2(joy, &status);

	return(~(status >> 4));
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartKbd() - Sets up my keyboard stuff for use
//
///////////////////////////////////////////////////////////////////////////
static void
INL_StartKbd(void)
{
	IN_ClearKeysDown();

	HIS_stackArea( stackArea , Stacksize );
	HIS_setHandler( KEYintNumber , INL_KeyService );
	HIS_enableInterrupt( KEYintNumber );

}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutKbd() - Restores keyboard control to the BIOS
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutKbd(void)
{
	HIS_detachHandler( KEYintNumber );
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartMouse() - Detects and sets up the mouse
//
///////////////////////////////////////////////////////////////////////////
static boolean
INL_StartMouse(void)
{
	MOS_start( mwork, MosWorkSize);
	return false;
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutMouse() - Cleans up after the mouse
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutMouse(void)
{
	MOS_end();
}

//
//	INL_SetJoyScale() - Sets up scaling values for the specified joystick
//
static void
INL_SetJoyScale(word joy)
{
	JoystickDef	*def;

	def = &JoyDefs[joy];
	def->joyMultXL = JoyScaleMax / (def->threshMinX - def->joyMinX);
	def->joyMultXH = JoyScaleMax / (def->joyMaxX - def->threshMaxX);
	def->joyMultYL = JoyScaleMax / (def->threshMinY - def->joyMinY);
	def->joyMultYH = JoyScaleMax / (def->joyMaxY - def->threshMaxY);
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_SetupJoy() - Sets up thresholding values and calls INL_SetJoyScale()
//		to set up scaling values
//
///////////////////////////////////////////////////////////////////////////
void
IN_SetupJoy(word joy,word minx,word maxx,word miny,word maxy)
{
	word		d,r;
	JoystickDef	*def;

	def = &JoyDefs[joy];

	def->joyMinX = minx;
	def->joyMaxX = maxx;
	r = maxx - minx;
	d = r / 3;
	def->threshMinX = ((r / 2) - d) + minx;
	def->threshMaxX = ((r / 2) + d) + minx;

	def->joyMinY = miny;
	def->joyMaxY = maxy;
	r = maxy - miny;
	d = r / 3;
	def->threshMinY = ((r / 2) - d) + miny;
	def->threshMaxY = ((r / 2) + d) + miny;


	INL_SetJoyScale(joy);
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_StartJoy() - Detects & auto-configures the specified joystick
//					The auto-config assumes the joystick is centered
//
///////////////////////////////////////////////////////////////////////////
static boolean
INL_StartJoy(word joy)
{
	//word	x,y;

	//IN_GetJoyAbs(joy, &x, &y);
	//IN_SetupJoy(joy, 0, x * 2, 0, y * 2);

	return(true);
}

///////////////////////////////////////////////////////////////////////////
//
//	INL_ShutJoy() - Cleans up the joystick stuff
//
///////////////////////////////////////////////////////////////////////////
static void
INL_ShutJoy(word joy)
{
	JoysPresent[joy] = false;
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_Startup() - Starts up the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Startup(void)
{
	boolean	checkjoys,checkmouse;
	word	i;

	if (IN_Started)
		return;

	checkjoys = true;
	checkmouse = true;
	for (i = 1;i < myargc;i++)
	{
		switch (US_CheckParm(myargv[i],ParmStrings_id_in))
		{
		case 0:
			checkjoys = false;
			break;
		case 1:
			checkmouse = false;
			break;
		}
	}

	INL_StartKbd();
	MousePresent = checkmouse? INL_StartMouse() : false;

	for (i = 0;i < MaxJoys;i++)
		JoysPresent[i] = checkjoys? INL_StartJoy(i) : false;

	IN_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Shutdown() - Shuts down the Input Mgr
//
///////////////////////////////////////////////////////////////////////////
void
IN_Shutdown(void)
{
	word	i;

	if (!IN_Started)
		return;

	INL_ShutMouse();
	for (i = 0;i < MaxJoys;i++)
		INL_ShutJoy(i);
	INL_ShutKbd();

	IN_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_ClearKeysDown() - Clears the keyboard array
//
///////////////////////////////////////////////////////////////////////////
void
IN_ClearKeysDown(void)
{
	LastScan = sc_None;
	LastASCII = key_None;
	memset (Keyboard,0,sizeof(Keyboard));
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_ReadControl() - Reads the device associated with the specified
//		player and fills in the control info struct
//
///////////////////////////////////////////////////////////////////////////
void
IN_ReadControl(int player,ControlInfo *info)
{
	boolean		realdelta;
	byte		dbyte;
	word		buttons;
	int			dx,dy;
	int		bt, xt, yt;
	Motion		mx,my;
	ControlType	type;
	register KeyboardDef	*def;

	dx = dy = 0;
	mx = my = motion_None;
	buttons = 0;

	if (DemoMode == demo_Playback)
	{
		dbyte = DemoBuffer[DemoOffset + 1];
		my = (Motion) ((dbyte & 3) - 1);
		mx = (Motion) (((dbyte >> 2) & 3) - 1);
		buttons = (dbyte >> 4) & 3;

		if (!(--DemoBuffer[DemoOffset]))
		{
			DemoOffset += 2;
			if (DemoOffset >= DemoSize)
				DemoMode = demo_PlayDone;
		}

		realdelta = false;
	}
	else if (DemoMode == demo_PlayDone)
		Quit("Demo playback exceeded");
	else
	{
		switch (type = Controls[player])
		{
		case ctrl_Keyboard:
			def = &KbdDefs;

			if (Keyboard[def->upleft])
				mx = motion_Left,my = motion_Up;
			else if (Keyboard[def->upright])
				mx = motion_Right,my = motion_Up;
			else if (Keyboard[def->downleft])
				mx = motion_Left,my = motion_Down;
			else if (Keyboard[def->downright])
				mx = motion_Right,my = motion_Down;

			if (Keyboard[def->up])
				my = motion_Up;
			else if (Keyboard[def->down])
				my = motion_Down;

			if (Keyboard[def->left])
				mx = motion_Left;
			else if (Keyboard[def->right])
				mx = motion_Right;

			if (Keyboard[def->button0])
				buttons += 1 << 0;
			if (Keyboard[def->button1])
				buttons += 1 << 1;
			realdelta = false;
			break;
		case ctrl_Joystick1:
		case ctrl_Joystick2:
			INL_GetJoyDelta(type - ctrl_Joystick,&dx,&dy);
			buttons = INL_GetJoyButtons(type - ctrl_Joystick);
			realdelta = true;
			break;
		case ctrl_Mouse:
			MOS_motion(&dx,&dy);
			MOS_rdpos(&bt, &xt, &yt);
			buttons = bt;
			realdelta = true;
			break;
		}
	}

	if (realdelta)
	{
		mx = (dx < 0)? motion_Left : ((dx > 0)? motion_Right : motion_None);
		my = (dy < 0)? motion_Up : ((dy > 0)? motion_Down : motion_None);
	}
	else
	{
		dx = mx * 127;
		dy = my * 127;
	}

	info->x = dx;
	info->xaxis = mx;
	info->y = dy;
	info->yaxis = my;
	info->button0 = buttons & (1 << 0);
	info->button1 = buttons & (1 << 1);
	info->button2 = buttons & (1 << 2);
	info->button3 = buttons & (1 << 3);
	info->dir = DirTable[((my + 1) * 3) + (mx + 1)];

	if (DemoMode == demo_Record)
	{
		// Pack the control info into a byte
		dbyte = (buttons << 4) | ((mx + 1) << 2) | (my + 1);

		if	((DemoBuffer[DemoOffset + 1] == dbyte)	&&	(DemoBuffer[DemoOffset] < 255))
			(DemoBuffer[DemoOffset])++;
		else
		{
			if (DemoOffset || DemoBuffer[DemoOffset])
				DemoOffset += 2;

			if (DemoOffset >= DemoSize)
				Quit("Demo buffer overflow");

			DemoBuffer[DemoOffset] = 1;
			DemoBuffer[DemoOffset + 1] = dbyte;
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
//	IN_Ack() - waits for a button or key press.  If a button is down, upon
// calling, it must be released for it to be recognized
//
///////////////////////////////////////////////////////////////////////////

boolean	btnstate[8];

void IN_StartAck(void)
{
	unsigned i,buttons;

//
// get initial state of everything
//
	IN_ClearKeysDown();
	memset (btnstate,0,sizeof(btnstate));

	buttons = IN_JoyButtons();
	if (MousePresent)
		buttons |= IN_MouseButtons ();

	for (i=0;i<8;i++,buttons>>=1)
		if (buttons&1)
			btnstate[i] = true;
}


boolean IN_CheckAck (void)
{
	unsigned i,buttons;

//
// see if something has been pressed
//
	if (LastScan)
		return true;

	buttons = IN_JoyButtons();
	if (MousePresent)
		buttons |= IN_MouseButtons ();

	for (i=0;i<8;i++,buttons>>=1)
		if ( buttons&1 )
		{
			if (!btnstate[i])
				return true;
		}
		else
			btnstate[i]=false;

	return false;
}


void IN_Ack (void)
{
	if(!IN_Started) { getch(); return; }
	
	
	IN_StartAck ();

	while (!IN_CheckAck ())
	;
}


///////////////////////////////////////////////////////////////////////////
//
//	IN_UserInput() - Waits for the specified delay time (in ticks) or the
//		user pressing a key or a mouse button. If the clear flag is set, it
//		then either clears the key or waits for the user to let the mouse
//		button up.
//
///////////////////////////////////////////////////////////////////////////
boolean IN_UserInput(longword delay)
{
	longword	lasttime;

	lasttime = TimeCount;
	IN_StartAck ();
	do
	{
		if (IN_CheckAck())
			return true;
	} while (TimeCount - lasttime < delay);
	return(false);
}

//===========================================================================

void	IN_MouseMove (int *dx, int *dy)
{
	int x,y;
	MOS_motion(&x,&y);

	*dx = -x;
	*dy = -y;
}

//===========================================================================

void	IN_MouseClear(void)
{
	int x,y;
	MOS_motion(&x,&y);
}

/*
===================
=
= IN_MouseButtons
=
===================
*/

byte	IN_MouseButtons (void)
{
	if (MousePresent)
	{
		int bt, x, y;
		MOS_rdpos(&bt, &x, &y);
		return (byte)bt;
	}
	else
		return 0;
}


/*
===================
=
= IN_JoyButtons
=
===================
*/

byte	IN_JoyButtons (void)
{
	unsigned joybits, temp;

	SND_joy_in_2(0, &temp);
	temp = (~temp) >> 4;
	SND_joy_in_2(1, &joybits);

	joybits = (~joybits) & 0xf0;
	joybits |= temp;

	return (byte)joybits;
}
