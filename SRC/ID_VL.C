// ID_VL.C

#include "wl_def.h"

#include <dos.h>
#include <string.h>
#include <conio.h>

//High C Lib for FM TOWNS Graphics
#include <EGB.H>

char work[EgbWorkSize] ;

//
// SC_INDEX is expected to stay at SC_MAPMASK for proper operation
//

unsigned int vbuf = 0;
unsigned int vdisp = 0;

#define VRAMWIDTH 1024

boolean		screenfaded;

byte		palette1[256][3], palette2[256][3];

//===========================================================================


void SetTextCursor(byte x, byte y)
{

}

/*
=======================
=
= VL_Shutdown
=
=======================
*/

void	VL_Shutdown (void)
{
	VL_SetTextMode ();
}


/*
=======================
=
= VL_SetVGAPlaneMode
=
=======================
*/

void	VL_SetVGAPlaneMode (void)
{
	//Screen set Mode12(640*480,256 Colors, 1 Page only)
	EGB_resolution( work, 0, 12 );

	///x2 zoom, 640*480->320*240
	EGB_displayStart( work, 2, 2, 2 );
	EGB_displayStart( work, 0, 0, 0 );
	EGB_displayStart( work, 3, 320, 240);

	EGB_displayPage( work, 0, 3 );
	EGB_writePage( work, 0 );

	EGB_color( work, 0, 0x00 );
	EGB_clearScreen( work );

}

/*
=======================
=
= VL_SetTextMode
=
=======================
*/

void	VL_SetTextMode (void)
{
	EGB_resolution( work, 0, 1 );
	EGB_resolution( work, 1, 1 );

	EGB_displayPage(work, 0, 3 );

	EGB_writePage( work, 1 );

	EGB_color( work, 0, 0x00 );
	EGB_clearScreen( work );

	EGB_writePage( work, 0 );

	EGB_color( work, 0, 0x00 );
	EGB_clearScreen( work );
}

void VL_WriteCRTC(int address, int data)
{
	_outb( 0x0440, address );
	_outw( 0x0442, data );
}

unsigned short VL_ReadCRTC(int address)
{
	_outb( 0x0440, address );
	return _inw( 0x0442 );
}

/*
=======================
=
= VL_WaitVBL
=
=======================
*/

void VL_WaitVBL(int vbls)
{
	while(vbls--)
	{
		//Wait Vsync
		_outb( 0x0440, 30 );
		while(!(_inb(0x443) & 4)){}
	}
}

/*
=============================================================================

						PALETTE OPS

		To avoid snow, do a WaitVBL BEFORE calling these

=============================================================================
*/


/*
=================
=
= VL_FillPalette
=
=================
*/

void VL_FillPalette (int red, int green, int blue)
{
	for (int i=0;i<256;i++)
	{
		_outb( 0xfd90, i );
		_outb( 0xfd94, red );
		_outb( 0xfd96, green );
		_outb( 0xfd92, blue );
	}
}

//===========================================================================

/*
=================
=
= VL_SetPalette
=
= If fast palette setting has been tested for, it is used
= (some cards don't like outsb palette setting)
=
=================
*/

void VL_SetPalette (byte *palette)
{
	for (int i=0;i<256;i++)
	{
		_outb( 0xfd90, i );
		_outb( 0xfd94, *palette++ );
		_outb( 0xfd96, *palette++ );
		_outb( 0xfd92, *palette++ );
	}
}


//===========================================================================

/*
=================
=
= VL_GetPalette
=
= This does not use the port string instructions,
= due to some incompatabilities
=
=================
*/

void VL_GetPalette (byte *palette)
{
	for (int i=0;i<256;i++)
	{
		_outb( 0xfd90, i );
		*palette++ = _inb( 0xfd94 );
		*palette++ = _inb( 0xfd96 );
		*palette++ = _inb( 0xfd92 );
	}
}


//===========================================================================

/*
=================
=
= VL_FadeOut
=
= Fades the current palette to the given color in the given number of steps
=
=================
*/

void VL_FadeOut (int start, int end, int red, int green, int blue, int steps)
{
	int		i,j,orig;
	byte	*origptr, *newptr;

	VL_WaitVBL(1);
	VL_GetPalette (&palette1[0][0]);
	memcpy (palette2,palette1,768);

//
// fade through intermediate frames
//
	for (i=0;i<steps;i++)
	{
		origptr = &palette1[start][0];
		newptr = &palette2[start][0];
		for (j=start;j<=end;j++)
		{
			orig = *origptr++;
			*newptr++ = orig + (red-orig) * i / steps;
			orig = *origptr++;
			*newptr++ = orig + (green-orig) * i / steps;
			orig = *origptr++;
			*newptr++ = orig + (blue-orig) * i / steps;
		}

		VL_WaitVBL(1);
		VL_SetPalette (&palette2[0][0]);
	}

//
// final color
//
	VL_FillPalette (red,green,blue);

	screenfaded = true;
}


/*
=================
=
= VL_FadeIn
=
=================
*/

void VL_FadeIn (int start, int end, byte *palette, int steps)
{
	int		i,j;
	byte	*orig;

	VL_WaitVBL(1);
	VL_GetPalette (&palette1[0][0]);
	memcpy (&palette2[0][0],&palette1[0][0],sizeof(palette1));

	start *= 3;
	end = end*3+2;

//
// fade through intermediate frames
//
	for (i=0;i<steps;i++)
	{
		for (j=start;j<=end;j++)
		{
			orig = &palette1[0][j];
			palette2[0][j] = *orig + (palette[j]-*orig) * i / steps;
		}

		VL_WaitVBL(1);
		VL_SetPalette (&palette2[0][0]);
	}

//
// final color
//
	VL_SetPalette (palette);
	screenfaded = false;
}

/*
=============================================================================

							PIXEL OPS

=============================================================================
*/

/*
=================
=
= VL_Plot
=
=================
*/

void VL_Plot (int x, int y, int color)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	vram[(x + vbuf) + (y * VRAMWIDTH)] = (byte)color;
}


/*
=================
=
= VL_Hlin
=
=================
*/

void VL_Hlin (unsigned x, unsigned y, unsigned width, int color)
{
	// << 10 = * VRAMWIDTH
	setdata_b(0x10c,(x + vbuf) + (y << 10),color,width);
}


/*
=================
=
= VL_Vlin
=
=================
*/

void VL_Vlin (int x, int y, int height, int color)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	vram += (x + vbuf) + (y * VRAMWIDTH);

	while(height)
	{
		*vram = (byte)color;
		vram += VRAMWIDTH;
		height--;
	}
}


/*
=================
=
= VL_Bar
=
=================
*/

void VL_Bar (int x, int y, int width, int height, int color)
{
	while(height)
	{
		VL_Hlin(x, y++, width, color);
		height--;
	}
}

/*
============================================================================

							MEMORY OPS

============================================================================
*/

/*
=================
=
= VL_MemToLatch
=
=================
*/

void VL_MemToLatch (byte *source, int width, int height, unsigned dest)
{
	_Far byte *sprram;
	_FP_SEG(sprram) = 0x114;
	_FP_OFF(sprram) = 0x0;

	for(int y = 0;y < height;y++)
	{
		for(int x = 0;x < width;x++)
		{
			sprram[dest++] = source[(y * (width >> 2) + (x >> 2)) + (x & 3) *  (width >> 2) * height];
		}
	}
}


//===========================================================================


/*
=================
=
= VL_MemToScreen
=
= Draws a block of data to the screen.
=
=================
*/

void VL_MemToScreen (byte *source, int width, int height, int x, int y)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	int offset = VRAMWIDTH - width;

	vram += x + vbuf + (y * VRAMWIDTH);

	for(y = 0;y < height;y++)
	{
		for(x = 0;x < width;x++)
		{
			*vram++ = *source++;
		}

		vram += offset;
	}
}

void VL_MemToScreen_VGA (byte *source, int width, int height, int x, int y)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = x + vbuf + (y << 10); // << 10 = * VRAMWIDTH

	int width_2 = width >> 2;
	int offset = VRAMWIDTH - width;
	int y2 = 0;
	int width_x_height = width_2 * height;

	for(y = 0;y < height; y++)
	{
		for(x = 0;x < width;x++)
		{
			*vram++ = source[(y2 + (x >> 2)) + (x & 3) * width_x_height];
		}
		vram += offset;
		y2 += width_2;
	}
}

//==========================================================================

/*
=================
=
= VL_LatchToScreen
=
=================
*/

void VL_LatchToScreen (unsigned source, int width, int height, int x, int y)
{
	unsigned int offset = x + vbuf + (y << 10); // << 10 = * VRAMWIDTH

	do{
		_movedata(0x114, source, 0x10c, offset, width);

		offset += VRAMWIDTH;
		source += width;
	}while(--height);
}

//===========================================================================

/*
=================
=
= VL_ScreenToScreen
=
=================
*/

void VL_ScreenToScreen (unsigned int source, unsigned int dest,int width, int height)
{
	do{
		_movedata(0x10c, source, 0x10c, dest, width);

		source += VRAMWIDTH;
		dest += VRAMWIDTH;
		
	}while(--height);
}

void VL_ScreenCopy (unsigned int source, unsigned int dest)
{
	unsigned int height = 200;

	do{
		_movedata(0x10c, source, 0x10c, dest, 320);

		dest += VRAMWIDTH;
		source += VRAMWIDTH;
		
	}while(--height);
}


void VL_VRAM_RAW(unsigned int offset, byte color)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	vram[offset] = color;
}

byte VL_GET_VRAM_RAW(unsigned int offset)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	return vram[offset];
}

byte VL_GET_VRAM_PIXEL(unsigned int x, unsigned int y)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	return vram[vbuf + x + (y * VRAMWIDTH)];
}

