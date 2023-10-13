// ID_VH.C

#include "wl_def.h"

//#define	SCREENWIDTH		1024
#define CHARWIDTH		2
#define TILEWIDTH		4
#define GRPLANES		4
#define BYTEPIXELS		4

#define SCREENXMASK		(~3)
#define SCREENXPLUS		(3)
#define SCREENXDIV		(4)

#define PIXTOBLOCK		4		// 16 pixels to an update block

byte	update[UPDATEHIGH][UPDATEWIDE];

//==========================================================================

pictabletype	*pictable;

int	px,py;
byte	fontcolor,backcolor;
int	fontnumber;
int bufferwidth,bufferheight;

//==========================================================================

void VW_DrawPropString (char *string)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;;

	fontstruct *font;
	int	width,step,step_temp,height;
	byte	*source;
	byte	ch;

	font = (fontstruct *)grsegs[STARTFONT+fontnumber];
	height = bufferheight = font->height;
	_FP_OFF(vram) = vbuf+(py << 10)+px; // << 10 = * SCREENWIDTH

	while ((ch = *string++)!=0)
	{
		width = step_temp = font->width[ch];
		source = ((byte *)font)+font->location[ch];
		do
		{
			step = 0;
			for(int i=0;i<height;i++)
			{
				if(source[step])
				{
					vram[(i<<10)] = fontcolor; // <<10 = * SCREENWIDTH
				}
				step += step_temp;
			}

			source++;
			vram++;
		}while(--width);
	}
	bufferheight = height;
}

/*
=================
=
= VL_MungePic
=
=================
*/

void VL_MungePic (byte *source, unsigned width, unsigned height)
{
	unsigned x,y,size;
	byte *temp, *srcline;

	size = width*height;

	if (width&3)
		Quit ("VL_MungePic: Not divisable by 4!");

//
// copy the pic to a temp buffer
//
	temp=(byte *) malloc(size);
	memcpy (temp,source,size);

//
// munge it back into the original buffer
//
	srcline = temp;
	for (y=0;y<height;y++)
	{
		for (x=0;x<width;x++)
		{
			VL_Plot ( x, y, *(srcline+x));
		}
		srcline+=width;
	}

	free(temp);
}

void VWL_MeasureString (char *string, word *width, word *height, fontstruct *font)
{
	*height = font->height;
	for (*width = 0;*string;string++)
		*width += font->width[*((byte *)string)];	// proportional width
}

void	VW_MeasurePropString (char *string, word *width, word *height)
{
	VWL_MeasureString(string,width,height,(fontstruct *)grsegs[STARTFONT+fontnumber]);
}

/*
=============================================================================

				Double buffer management routines

=============================================================================
*/

void VH_UpdateScreen(void)
{
	VL_ScreenCopy(vbuf, vdisp);
}


void VWB_DrawTile8 (int x, int y, int tile)
{
	LatchDrawChar(x,y,tile);
}

void VWB_DrawPic (int x, int y, int chunknum)
{
	pictabletype *pic = &pictable[chunknum - STARTPICS];

	VL_MemToScreen_VGA (grsegs[chunknum],pic->width,pic->height,x & ~7,y);
}

void VWB_DrawPropString	 (char *string)
{
	VW_DrawPropString (string);
}

void VWB_Bar (int x, int y, int width, int height, int color)
{
	VW_Bar (x,y,width,height,color);
}

void VWB_Plot (int x, int y, int color)
{
	VW_Plot(x,y,color);
}

void VWB_Hlin (int x1, int x2, int y, int color)
{
	VW_Hlin(x1,x2,y,color);
}

void VWB_Vlin (int y1, int y2, int x, int color)
{
	VW_Vlin(y1,y2,x,color);
}


/*
=============================================================================

						WOLFENSTEIN STUFF

=============================================================================
*/

/*
=====================
=
= LatchDrawPic
=
=====================
*/

void LatchDrawPic (unsigned x, unsigned y, unsigned picnum)
{
	pictabletype *pic = &pictable[picnum-STARTPICS];

	VL_LatchToScreen (latchpics[2+picnum-LATCHPICS_LUMP_START],pic->width,pic->height,x*8,y);
}


//==========================================================================

/*
===================
=
= LoadLatchMem
=
===================
*/

void LoadLatchMem (void)
{
	int	i,width,height,start,end;
	byte	*src;
	unsigned destoff;

//
// tile 8s
//
	latchpics[0] = freelatch;
	CA_CacheGrChunk (STARTTILE8);
	src = grsegs[STARTTILE8];
	destoff = freelatch;

	for (i=0;i<NUMTILE8;i++)
	{
		VL_MemToLatch (src,8,8,destoff);
		src += 64;
		destoff += 64;
	}
	UNCACHEGRCHUNK (STARTTILE8);

#if 0	// ran out of latch space!
//
// tile 16s
//
	src = (byte _seg *)grsegs[STARTTILE16];
	latchpics[1] = destoff;

	for (i=0;i<NUMTILE16;i++)
	{
		CA_CacheGrChunk (STARTTILE16+i);
		src = (byte _seg *)grsegs[STARTTILE16+i];
		VL_MemToLatch (src,16,16,destoff);
		destoff+=256;
		if (src)
			UNCACHEGRCHUNK (STARTTILE16+i);
	}
#endif

//
// pics
//
	start = LATCHPICS_LUMP_START;
	end = LATCHPICS_LUMP_END;

	for (i=start;i<=end;i++)
	{
		latchpics[2+i-start] = destoff;
		CA_CacheGrChunk (i);
		width = pictable[i-STARTPICS].width;
		height = pictable[i-STARTPICS].height;
		VL_MemToLatch (grsegs[i],width,height,destoff);
		destoff += width * height;
		UNCACHEGRCHUNK(i);
	}

	//EGAMAPMASK(15);
}

//==========================================================================

/*
===================
=
= FizzleFade
=
= returns true if aborted
=
===================
*/


boolean FizzleFade (unsigned source, unsigned dest,	unsigned width,
		unsigned height, unsigned frames, boolean abortable)
{
	_Far byte *vram;
	_FP_SEG(vram) = 0x10c;
	_FP_OFF(vram) = 0x0;

	int			pixperframe;
	unsigned drawofs,pagedelta;
	unsigned x,y,p,frame;
	long		rndval;

	pagedelta = dest-source;
	rndval = 1;
	x = 0;
	y = 0;
	pixperframe = 64000/frames;

	IN_StartAck ();

	TimeCount=frame=0;
	do
	{
		if (abortable && IN_CheckAck () )
			return true;

		for (p=0;p<pixperframe;p++)
		{
			//
			// seperate random value into x/y pair
			//
			x = (rndval & 0x1ff00) >> 8;// next 9 bits = x xoordinate
			y = rndval & 0xff; // low 8 bits = y xoordinate

			//
			// advance to next random element
			//
			rndval = (rndval >> 1) ^ (rndval & 1 ? 0 : 0x12000);

			if (x>width || y>height)
				continue;
			drawofs = source + (y << 10) + x; // << 10 = * SCREENWIDTH

			//
			// copy one pixel
			//

			vram[drawofs+pagedelta]=vram[drawofs];

			if (rndval == 1)		// entire sequence has been completed
				return false;
		}
		frame++;
		while (TimeCount<frame)		// don't go too fast
		;
	} while (1);
}
