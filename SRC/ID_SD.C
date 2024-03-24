//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundSourcePresent - Sound Source thingie present?
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//

#include "wl_def.h"
#include <SND.h>

#include <dos.h>

#ifdef  nil
#undef  nil
#endif
#define nil     0

char	swork[16384];
#define PCM_CH		71
#define FM_CH_NUM	6

//#define BUFFERDMA
#define NOSEGMENTATION
//#define SHOWSDDEBUG

#define SDL_SoundFinished()     {SoundNumber = (soundnames) 0; SoundPriority = 0;}

// Macros for SoundBlaster stuff
#define sbOut(n,b)      outp((n) + sbLocation,b)
#define sbIn(n)         inp((n) + sbLocation)
#define sbWriteDelay()  while (sbIn(sbWriteStat) & 0x80);
#define sbReadDelay()   while (sbIn(sbDataAvail) & 0x80);

// Macros for AdLib stuff
#define selreg(n)       outp(alFMAddr,n)
#define writereg(n)     outp(alFMData,n)
#define readstat()      inp(alFMStatus)

//      Global variables
		boolean         AdLibPresent,
								PCMPresent,StereoPCMPresent,
								NeedsDigitized,NeedsMusic,
								SoundPositioned;
		SDMode          SoundMode;
		SMMode          MusicMode;
		SDSMode         DigiMode;
		volatile longword TimeCount;
		byte **SoundTable;
		int                     DigiMap[LASTSOUND];

//      Internal variables
static  boolean                 SD_Started;
				boolean                 nextsoundpos;
static  char                    *ParmStrings_id_sd[] =
												{
														"noal",
														"nopcm",
														"noste",
														nil
												};
//static  void                    (*SoundUserHook)(void);
				soundnames              SoundNumber,DigiNumber;
				word                    SoundPriority,DigiPriority;
				int                             LeftPosition,RightPosition;
				long                    LocalTime;
				word                    TimerRate;

				word                    NumDigi,DigiLeft,DigiPage;
				word                    *DigiList;
				boolean                 DigiPlaying;
static  boolean                 DigiMissed,DigiLastSegment;

//      RF5C68 variables
static  boolean                                 PCMNoCheck,StereoPCMNoCheck;
static  volatile boolean                PCMSamplePlaying;

//      PC Sound variables
				volatile byte   pcLastSample;
				volatile byte *pcSound;
				longword                pcLengthLeft;

//      AdLib variables
				boolean                 alNoCheck;
				byte                    *alSound;
				byte                    alBlock;
				longword                alLengthLeft;
				longword                alTimeCount;
				Instrument      alZeroInst;
				boolean         alNoIRQ;

byte multi[2][6], tl[2][6], ar[2][6], fnum[6], fnum_hi[6];

static byte uint8_to_int8_for_pcmwave[256] =
{
0x7f,0x7f,0x7e,0x7d,0x7c,0x7b,0x7a,0x79,0x78,0x77,0x76,0x75,0x74,0x73,0x72,0x71,
0x70,0x6f,0x6e,0x6d,0x6c,0x6b,0x6a,0x69,0x68,0x67,0x66,0x65,0x64,0x63,0x62,0x61,
0x60,0x5f,0x5e,0x5d,0x5c,0x5b,0x5a,0x59,0x58,0x57,0x56,0x55,0x54,0x53,0x52,0x51,
0x50,0x4f,0x4e,0x4d,0x4c,0x4b,0x4a,0x49,0x48,0x47,0x46,0x45,0x44,0x43,0x42,0x41,
0x40,0x3f,0x3e,0x3d,0x3c,0x3b,0x3a,0x39,0x38,0x37,0x36,0x35,0x34,0x33,0x32,0x31,
0x30,0x2f,0x2e,0x2d,0x2c,0x2b,0x2a,0x29,0x28,0x27,0x26,0x25,0x24,0x23,0x22,0x21,
0x20,0x1f,0x1e,0x1d,0x1c,0x1b,0x1a,0x19,0x18,0x17,0x16,0x15,0x14,0x13,0x12,0x11,
0x10,0xf ,0xe ,0xd ,0xc ,0xb ,0xa ,0x9 ,0x8 ,0x7 ,0x6 ,0x5 ,0x4 ,0x3 ,0x2 ,0x1 ,
0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,
0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,
0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,
0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,
0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,
0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,
0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,
0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xfe
};
  


// This table maps channel numbers to carrier and modulator op cells
//static  byte                    carriers[9] =  { 3, 4, 5,11,12,13,19,20,21},
//                                                modifiers[9] = { 0, 1, 2, 8, 9,10,16,17,18};

//      Sequencer variables
				boolean                 sqActive;
				word                    *sqHack,*sqHackPtr;
				word                    sqHackLen,sqHackSeqLen;
				long                    sqHackTime;



int count_time=0;
int count_fx=0;
int count_pcm=0;
int extreme=0;

//volatile boolean deactivateSoundHandler=false;

int lastsoundstarted=-1;
int lastdigiwhich=-1;
int lastdigistart=-1;
int lastdigisegstart=-1;

int freq = 0;

void ym3812_to_ym2612(int address, int data)
{
	if((address >= 0x20) && (address <= 0x2d)) //multi
	{
		address -= 0x20;
		if(address >= 11) //ch4-6 op2
		{
			multi[1][address - 8] = data;
			//SND_fm_write_data(1, YM2612Char + 4 + (address - 11), data & 0xf);
			SND_fm_write_data(1, YM2612Char + 8 + (address - 11), data & 0xf);
			//SND_fm_write_data(1, YM2612Char + 12 + (address - 11), data & 0xf);
		}
		else if(address >= 8) //ch4-6 op1
		{
			multi[0][address - 5] = data;
			SND_fm_write_data(1, YM2612Char + (address - 8), data & 0xf);
		}
		else if(address == 6 || address == 7) //none
		{
		}
		else if(address >= 3) //ch1-3 op2
		{
			multi[1][address - 3] = data;
			//SND_fm_write_data(0, YM2612Char + 4 + (address - 3), data & 0xf);
			SND_fm_write_data(0, YM2612Char + 8 + (address - 3), data & 0xf);
			//SND_fm_write_data(0, YM2612Char + 12 + (address - 3), data & 0xf);
		}
		else //ch1-3 op1
		{
			multi[0][address] = data;
			SND_fm_write_data(0, YM2612Char + address, data & 0xf);
		}
	}
	else if((address >= 0x40) && (address <= 0x4d)) //total level
	{
		address -= 0x40;
		if(address >= 11) //ch4-6 op2
		{
			tl[1][address - 8] = data;
			//SND_fm_write_data(1, YM2612Scale + 4 + (address - 11), data & 0x3f);
			SND_fm_write_data(1, YM2612Scale + 8 + (address - 11), data & 0x3f);
			//SND_fm_write_data(1, YM2612Scale + 12 + (address - 11), data & 0x3f);
		}
		else if(address >= 8) //ch4-6 op1
		{
			tl[0][address - 5] = data;
			SND_fm_write_data(1, YM2612Scale + (address - 8), data & 0x3f);
		}
		else if(address == 6 || address == 7) //none
		{
		}
		else if(address >= 3) //ch1-3 op2
		{
			tl[1][address - 3] = data;
			//SND_fm_write_data(0, YM2612Scale + 4 + (address - 3), data & 0x3f);
			SND_fm_write_data(0, YM2612Scale + 8 + (address - 3), data & 0x3f);
			//SND_fm_write_data(0, YM2612Scale + 12 + (address - 3), data & 0x3f);
		}
		else //ch1-3 op1
		{
			tl[0][address] = data;
			SND_fm_write_data(0, YM2612Scale + address, data & 0x3f);
		}
	}
	else if((address >= 0x60) && (address <= 0x6d)) //attack
	{
		address -= 0x60;
		if(address >= 11) //ch4-6 op2
		{
			ar[1][address - 8] =  data;
			//SND_fm_write_data(1, YM2612Attack + 4 + (address - 11), ((data >> 4) & 0xf) | ((multi[1][address - 8] << 2) & 0x40));
			//SND_fm_write_data(1, YM2612Decay + 4 + (address - 11), (multi[1][address - 8] & 0x80) | (data & 0xf));
			//SND_fm_write_data(1, YM2612SR + 4 + (address - 11), (multi[1][address - 8] >> 4) & 0x2);
			SND_fm_write_data(1, YM2612Attack + 8 + (address - 11), ((data >> 4) & 0xf) | ((multi[1][address - 8] << 2) & 0x40));
			SND_fm_write_data(1, YM2612Decay + 8 + (address - 11), (multi[1][address - 8] & 0x80) | (data & 0xf));
			SND_fm_write_data(1, YM2612SR + 8 + (address - 1), (multi[1][address - 8] >> 4) & 0x2);
			//SND_fm_write_data(1, YM2612Attack + 12 + (address - 11), ((data >> 4) & 0xf) | ((multi[1][address - 8] << 2) & 0x40));
			//SND_fm_write_data(1, YM2612Decay + 12 + (address - 11), (multi[1][address - 8] & 0x80) | (data & 0xf));
			//SND_fm_write_data(1, YM2612SR + 12 + (address - 11), (multi[1][address - 8] >> 4) & 0x2);
		}
		else if(address >= 8) //ch4-6 op1
		{
			ar[0][address - 5] =  data;
			SND_fm_write_data(1, YM2612Attack + (address - 8), ((data >> 4) & 0xf) | ((multi[0][address - 5] << 2) & 0x40));
			SND_fm_write_data(1, YM2612Decay + (address - 8), (multi[0][address - 5] & 0x80) | (data & 0xf));
			SND_fm_write_data(1, YM2612SR + (address - 8), (multi[0][address - 5] >> 4) & 0x2);
		}
		else if(address == 6 || address == 7) //none
		{
		}
		else if(address >= 3) //ch1-3 op2
		{
			ar[1][address - 3] =  data;
			//SND_fm_write_data(0, YM2612Attack + 4 + (address - 3), ((data >> 4) & 0xf) | ((multi[1][address - 3] << 2) & 0x40));
			//SND_fm_write_data(0, YM2612Decay + 4 + (address - 3), (multi[1][address - 3] & 0x80) | (data & 0xf));
			//SND_fm_write_data(0, YM2612SR + 4 + (address - 3), (multi[1][address - 3] >> 4) & 0x2);
			SND_fm_write_data(0, YM2612Attack + 8 + (address - 3), ((data >> 4) & 0xf) | ((multi[1][address - 3] << 2) & 0x40));
			SND_fm_write_data(0, YM2612Decay + 8 + (address - 3), (multi[1][address - 3] & 0x80) | (data & 0xf));
			SND_fm_write_data(0, YM2612SR + 8 + (address - 3), (multi[1][address - 3] >> 4) & 0x2);
			//SND_fm_write_data(0, YM2612Attack + 12 + (address - 3), ((data >> 4) & 0xf) | ((multi[1][address - 3] << 2) & 0x40));
			//SND_fm_write_data(0, YM2612Decay + 12 + (address - 3), (multi[1][address - 3] & 0x80) | (data & 0xf));
			//SND_fm_write_data(0, YM2612SR + 12 + (address - 3), (multi[1][address - 3] >> 4) & 0x2);
		}
		else //ch1-3 op1
		{
			ar[0][address] =  data;
			SND_fm_write_data(0, YM2612Attack + address, ((data >> 4) & 0xf) | ((multi[0][address] << 2) & 0x40));
			SND_fm_write_data(0, YM2612Decay + address, (multi[0][address]  & 0x80) | (data & 0xf));
			SND_fm_write_data(0, YM2612SR + address, (multi[0][address] >> 4) & 0x2);
		}
	}
	else if((address >= 0x80) && (address <= 0x8d)) //sustain
	{
		address -= 0x80;
		if(address >= 11) //ch4-6 op2
		{
			//SND_fm_write_data(1, YM2612Sus + 4 + (address - 11), data & 0xff);
			SND_fm_write_data(1, YM2612Sus + 8 + (address - 11), data & 0xff);
			//SND_fm_write_data(1, YM2612Sus + 12 + (address - 11), data & 0xff);
		}
		else if(address >= 8) //ch4-6 op1
		{
			SND_fm_write_data(1, YM2612Sus + (address - 8), data & 0xff);
		}
		else if(address == 6 || address == 7) //none
		{
		}
		else if(address >= 3) //ch1-3 op2
		{
			//SND_fm_write_data(0, YM2612Sus + 4 + (address - 3), data & 0xff);
			SND_fm_write_data(0, YM2612Sus + 8 + (address - 3), data & 0xff);
			//SND_fm_write_data(0, YM2612Sus + 12 + (address - 3), data & 0xff);
		}
		else //ch1-3 op1
		{
			SND_fm_write_data(0, YM2612Sus + address, data & 0xff);
		}
	}
	else if((address >= 0xa0) && (address <= 0xa5)) //f-number
	{
		address -= 0xa0;
		if(address >= 3) //ch4-6
		{
			fnum[address] = data;
			SND_fm_write_data(1, YM2612FreqH + (address - 3), (((fnum_hi[address]  << 1) & 0x3f ) | ((fnum[address] >> 7) & 0x1)) & 0xff);
			SND_fm_write_data(1, YM2612FreqL + (address - 3), (fnum[address] << 1) &0xff);
		}
		else //ch1-3
		{
			fnum[address] = data;
			SND_fm_write_data(0, YM2612FreqH + address, (((fnum_hi[address] << 1) & 0x3f) | ((fnum[address] >> 7) & 0x1)) & 0xff);
			SND_fm_write_data(0, YM2612FreqL + address, (fnum[address] << 1) & 0xff);
		}

	}
	else if((address >= 0xb0) && (address <= 0xb5)) //block
	{
		address -= 0xb0;
		if(address >= 3) //ch4-6
		{
			fnum_hi[address] = data;
			SND_fm_write_data(1, YM2612FreqH + (address - 3), (((fnum_hi[address] << 1) & 0x3f) | ((fnum[address] >> 7) & 0x1)) & 0xff);
			SND_fm_write_data(1, YM2612FreqL + (address - 3), (fnum[address] << 1) & 0xff);
			address++;
		}
		else //ch1-3
		{
			fnum_hi[address] = data;
			SND_fm_write_data(0, YM2612FreqH + address, (((fnum_hi[address] << 1) & 0x3f) | ((fnum[address] >> 7) & 0x1)) & 0xff);
			SND_fm_write_data(0, YM2612FreqL + address, (fnum[address] << 1) & 0xff);
		}

		if(data & 32)
		{
			SND_fm_write_data(0, 0x28, (0x30 + address) & 0x37);
		}
		else
		{
			SND_fm_write_data(0, 0x28, address & 0x7);
		}
	}
	else if((address >= 0xc0) && (address <= 0xc5)) //feedback
	{
		address -= 0xc0;
		if(address >= 3) //ch4-6
		{
			SND_fm_write_data(1, YM2612FeedCon + (address - 3), ((data << 2) & 0x38) | 0x6 + (data & 0x1));
		}
		else //ch1-3
		{
			SND_fm_write_data(0, YM2612FeedCon + address, ((data << 2) & 0x38) | 0x6 + (data & 0x1));
		}
	}
}

void ym2612_registers_initialize(int ch)
{
	int bank;

	if (ch >= 0 && ch <= 2)
	{
		bank = 0;
		SND_fm_write_data(0, 0x28, ch & 0x7);
		/*multi[0][ch] = 0;
		tl[0][ch] = 0;
		ar[0][ch] = 0;
		multi[1][ch] = 0;
		tl[1][ch] = 0;
		ar[1][ch] = 0;
		fnum[ch] = 0;
		fnum_hi[ch] = 0;*/
	}
	else if(ch >= 3 && ch <= 5)
	{
		bank = 1;
		SND_fm_write_data(0, 0x28, (ch + 1) & 0x7);
		/*multi[0][ch] = 0;
		tl[0][ch] = 0;
		ar[0][ch] = 0;
		multi[1][ch] = 0;
		tl[1][ch] = 0;
		ar[1][ch] = 0;
		fnum[ch] = 0;
		fnum_hi[ch] = 0;*/
		ch -= 3;
	}

	for(int op = 0; op < 16 ; op+=4)
	{
		SND_fm_write_data(bank, YM2612Char + op + ch, 0);
		SND_fm_write_data(bank, YM2612Scale + op + ch, 127);
		SND_fm_write_data(bank, YM2612Attack + op + ch, 0x1f);
		SND_fm_write_data(bank, YM2612Decay + op + ch, 0);
		SND_fm_write_data(bank, YM2612SR + op + ch, 0);
		SND_fm_write_data(bank, YM2612Sus + op + ch, 0x0);
	}

	SND_fm_write_data(bank, YM2612FreqH + ch, 0);
	SND_fm_write_data(bank, YM2612FreqL + ch, 0);

	SND_fm_write_data(bank, YM2612FeedCon + ch, 0x3e);

}

byte set_wavedata(byte *data,longword len)
{
	_Far byte *waveram;
	_FP_SEG(waveram) = 0x140;
	_FP_OFF(waveram) = 0x0;

	byte control = 0x80;

	_outb( 0x4f7, control );

	while(len)
	{
		//*waveram++ = uint8_to_int8_for_pcmwave[*data++];
		*waveram++ = *data++;

		if(_FP_OFF(waveram) == 4096)
		{
			control++;
			if(control == 0x8b)
			{
				break;
			}
			_outb( 0x4f7, control );
			_FP_OFF(waveram) = 0x0;
		}
		len--;
	}

	*waveram = 0xff;

	return control;
}

void SDL_Cache_Gun_Sounds(void) // for SDL_PCMPlay_Gun
{
	_Far byte *waveram;
	_FP_SEG(waveram) = 0x140;

	int i;

	word    len;
	byte  	*data;

	byte control = 12;

	for (i = 4; i < 7; i++)
	{
		len = DigiList[(i * 2) + 1];
		data = (Pages+Pagetables[PMSoundStart+(DigiList[(i * 2) + 0])]);

		_FP_OFF(waveram) = 0x0;
		_outb(0x4f7, control);

		while (len)
		{
		//*waveram++ = uint8_to_int8_for_pcmwave[*data++];
		*waveram++ = *data++;

		if (_FP_OFF(waveram) == 4096)
		{
			_FP_OFF(waveram) = 0x0;
			control++;
			_outb(0x4f7, control);
		}
		len--;
		}

		*waveram = 0xff;
		control++;
	}

}


void SDL_turnOnPCSpeaker(word timerval)
{
	byte reg;

	reg = _inb(0x0060);
	reg >>= 2;
	reg |= 4;
	_outb(0x0060, reg);

	_outb(0x0046, 0xb6);

	_outb(0x0044, timerval & 0xff);
	_outb(0x0044, (timerval >> 8) & 0xff);

}

void SDL_turnOffPCSpeaker(void)
{
	byte reg;

	reg = _inb(0x0060);
	reg >>= 2;
	reg &= 3;
	_outb(0x0060, reg);
}

void SDL_setPCSpeaker(byte val)
{
	_outb(0x0044, val);
}

// YM2612 Timer A for 700Hz interrupts
void SDL_DoFast(void)
{
	count_fx++;
	if(count_fx>=5)
	{
		count_fx=0;

		if (PCMSamplePlaying)
		{
			count_pcm++;
			if(count_pcm >= 100)
			{
				count_pcm = 0;
				PCMSamplePlaying = false;
				DigiNumber = (soundnames) 0;
				DigiPriority = 0;
				SoundPositioned = false;
			}
		}

		if(pcSound)
		{
			if(*pcSound!=pcLastSample)
			{
				pcLastSample=*pcSound;

				if(pcLastSample)
					SDL_turnOnPCSpeaker(pcLastSample*20);
				else
					SDL_turnOffPCSpeaker();
			}
			pcSound++;
			pcLengthLeft--;
			if(!pcLengthLeft)
			{
				pcSound=0;
				SoundNumber=(soundnames)0;
				SoundPriority=0;
				SDL_turnOffPCSpeaker();
			}                                     
		}

		if(alSound && !alNoIRQ)
		{
			if(*alSound)
			{
				SND_fm_write_data(0, YM2612FreqH, alBlock | *alSound >> 5);
				SND_fm_write_data(0, YM2612FreqL, (*alSound << 3) & 0xff);
			}
			else
			{
				SND_fm_write_data(0, YM2612FreqH, 0);
				SND_fm_write_data(0, YM2612FreqL, 0);
			}
			alSound++;
			alLengthLeft--;
			if(!alLengthLeft)
			{
				alSound=0;
				SoundNumber=(soundnames)0;
				SoundPriority=0;
				SND_fm_write_data(0, YM2612FreqH, 0);
				SND_fm_write_data(0, YM2612FreqL, 0);
			}
		}

		count_time = 1 - count_time;
		if(count_time)
		{
			TimeCount++;
		}
	}

	if(sqActive && !alNoIRQ)
	{
		if(sqHackLen)
		{
			do
			{
				if(sqHackTime>alTimeCount) break;
				sqHackTime=alTimeCount+*(sqHackPtr+1);

				ym3812_to_ym2612(*(byte *)sqHackPtr, *(((byte *)sqHackPtr)+1));

				sqHackPtr+=2;
				sqHackLen-=4;
			}
			while(sqHackLen);
		}
		alTimeCount++;
		if(!sqHackLen)
		{
			sqHackPtr=sqHack;
			sqHackLen=sqHackSeqLen;
			alTimeCount=0;
			sqHackTime=0;
		}
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetTimer0() - Sets system timer 0 to the specified speed
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetTimer0(int speed)
{
	SND_fm_timer_a_set(255, speed);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetIntsPerSec() - Uses SDL_SetTimer0() to set the number of
//              interrupts generated by system timer 0 per second
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetIntsPerSec(word ints)
{
		TimerRate = ints;
		SDL_SetTimer0(1023 - (55580 / ints));
}

static void
SDL_SetTimerSpeed(void)
{
		word    rate;

		rate = TickBase * 10;

		if (rate != TimerRate)
		{
			SND_int_timer_a_set(SDL_DoFast);
			SDL_SetIntsPerSec(rate);
			SND_fm_timer_a_start();
			TimerRate = rate;
		}
}

//
//      PCM code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCMStopSample() - Stops any active sampled sound from the RF5C68 to cease
//
///////////////////////////////////////////////////////////////////////////
static void SDL_PCMStopSample(void)
{
	if (PCMSamplePlaying)
	{
		PCMSamplePlaying = false;

		_outb( 0x4f7, 0xc0);
		_outb( 0x4f6, 0xf0);
	}
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCMPlaySample() - Plays a sampled sound on the RF5C68. 
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCMPlaySample(byte *data,longword len,boolean inIRQ)
{

	//SDL_PCMStopSample();

	_outb(0x04f8, 0xfd);

	set_wavedata(data, len);

	_outb(0x04f7, 0xc0);
	_outb(0x04f6, 0);
	_outb(0x04f8, 0xfc);
	_outb(0x04f7, 0x80);

	PCMSamplePlaying = true;

#ifdef SHOWSDDEBUG
	static int numplayed=0;
	numplayed++;
	VL_Plot(numplayed,1,14);
#endif
}

static void SDL_PCMPlay_Gun(soundnames sound)
{
	_Far byte *waveram;
	_FP_SEG(waveram) = 0x140;
	_FP_OFF(waveram) = 0x0;

	_outb(0x04f8, 0xfe);

	//Need wait(384 source clocks) for RF5C68 registers? i don't know.
	_outb(0x4f7, 0x8f);
	*waveram++ = 0x0;
	for(int i = 0;i < 383;i++)
	{
		*waveram++ = 0xff;
	}

	_outb(0x04f7, 0xc1);

	switch (sound)
	{
		case ATKMACHINEGUNSND:
			_outb(0x04f6, 0xc0);
			break;

		case ATKPISTOLSND:
			_outb(0x04f6, 0xd0);
			break;
		case ATKGATLINGSND:
			_outb(0x04f6, 0xe0);
			break;
	}

	_outb(0x04f8, 0xfc);
	_outb(0x04f7, 0x80);
}


///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartPCM() - Turns on the PCM
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartPCM(void)
{
	_Far char *waveram;
	_FP_SEG(waveram) = 0x140;
	_FP_OFF(waveram) = 0x0;
	int i;

	_FP_OFF(waveram) = 0x0;
	_outb( 0x4f7, 0x0 );

	*waveram++ = 0x0;

	for(i = 0;i < 4095;i++)
	{
		*waveram++ = 0xff;
	}

	_FP_OFF(waveram) = 0x0;
	_outb( 0x4f7, 0xf );

	*waveram++ = 0x0;

	for(i = 0;i < 4095;i++)
	{
		*waveram++ = 0xff;
	}

	_outb( 0x4f7, 0x40 );

	_outb( 0x4f0, 0xff);
	_outb( 0x4f1, 0xff);
	_outb( 0x4f2, 0xae );
	_outb( 0x4f3, 0x2 );
	_outb( 0x4f4, 0x0 );
	_outb( 0x4f5, 0xf0);
	_outb( 0x4f6, 0x0);

	_outb(0x4f7, 0x41);

	_outb(0x4f0, 0xff);
	_outb(0x4f1, 0xff);
	_outb(0x4f2, 0xae);
	_outb(0x4f3, 0x2);
	_outb(0x4f4, 0x0);
	_outb(0x4f5, 0xf0);
	_outb(0x4f6, 0xf0);

	_outb(0x04f8, 0xfc);

	_outb(0x4f7, 0x80);

	DigiNumber = (soundnames) 0;
	DigiPriority = 0;
	SoundPositioned = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPCM() - Turns off the RF5C68
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutPCM(void)
{
		SDL_PCMStopSample();

		if (StereoPCMPresent)
		{
				// Restore FM output levels
				SND_pan_set(0 , 64);
				SND_pan_set(1 , 64);
				SND_pan_set(2 , 64);
				SND_pan_set(3 , 64);
				SND_pan_set(4 , 64);
				SND_pan_set(5 , 64);

				// Restore Voice output levels
				SND_pan_set(PCM_CH , 64);
		}

}


//
//      PC Sound code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySound() - Plays the specified sound on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySound(PCSound *sound)
{
	_disable();

	pcLastSample = -1;
	pcLengthLeft = sound->common.length;
	pcSound = sound->data;

	_enable();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSound() - Stops the current sound playing on the PC Speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSound(void)
{
	_disable();

	pcSound = 0;
	SDL_turnOffPCSpeaker(); // Turn the speaker off

	_enable();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPC() - Turns off the pc speaker
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutPC(void)
{
	_disable();

	pcSound = 0;
	SDL_turnOffPCSpeaker(); // Turn the speaker off

	_enable();
}

void
SDL_PlayDigiSegment(memptr addr,word len,boolean inIRQ)
{
		lastdigisegstart=(int)addr;
		if(DigiMode != sds_Off)
		{
			SDL_PCMPlaySample((byte *) addr,len,inIRQ);
		}
}

void
SD_StopDigitized(void)
{
		DigiLeft = 0;
		DigiMissed = false;
		DigiPlaying = false;
		DigiNumber = (soundnames) 0;
		DigiPriority = 0;
		SoundPositioned = false;

		SDL_PCMStopSample();
}

void
SD_Poll(void)
{

}


///////////////////////////////////////////////////////////////////////////
//
//      SD_SetPosition() - Sets the attenuation levels for the left and right
//              channels by using the mixer.
//
///////////////////////////////////////////////////////////////////////////
void
SD_SetPosition(int leftpos,int rightpos)
{
		if      ((leftpos < 0) ||       (leftpos > 15)  ||      (rightpos < 0)  ||      (rightpos > 15)
						||      ((leftpos == 15) && (rightpos == 15)))
				Quit("SD_SetPosition: Illegal position");

		switch (DigiMode)
		{
		case sds_MonoPCM:
			int temp = _max(leftpos, rightpos);
			temp = 15 - temp;
			_outb(0x4f7, 0xc0);
			_outb(0x4f1, ((temp << 4) & 0xf0) | (temp & 0xf));
			break;
		case sds_StereoPCM:
			rightpos = 15 - rightpos;
			leftpos = 15 - leftpos;
			_outb(0x4f7, 0xc0);
			_outb(0x4f1, ((rightpos << 4) & 0xf0) | (leftpos & 0xf));
			break;
		}
}

void
SD_PlayDigitized(word which,int leftpos,int rightpos)
{
		word    len;
		memptr  addr;

		if (!DigiMode)
				return;

		SD_StopDigitized();
		if (which >= NumDigi)
				Quit("SD_PlayDigitized: bad sound number");

		SD_SetPosition(leftpos,rightpos);

		lastdigiwhich=which;

		DigiPage = DigiList[(which * 2) + 0];
		DigiLeft = DigiList[(which * 2) + 1];

		//lastdigistart=DigiPage;

		len = DigiLeft;
		addr = (Pages+Pagetables[PMSoundStart+DigiPage]);
		DigiPage++;

		DigiPlaying = true;
		DigiLastSegment = false;

		SDL_PlayDigiSegment(addr,len,false);
		DigiLeft -= len;
		if (!DigiLeft)
				DigiLastSegment = true;

		SD_Poll();
}


void
SD_SetDigiDevice(SDSMode mode)
{
		if (mode == DigiMode)
				return;

		SD_StopDigitized();

		switch (mode)
		{
			case sds_MonoPCM:
				if (!PCMPresent)
				{
					DigiMode = sds_Off;
				}
				else
				{
					DigiMode = sds_MonoPCM;
				}
				break;
			case sds_StereoPCM:
				if (!StereoPCMPresent)
				{
					if (!PCMPresent)
					{
						DigiMode = sds_Off;
					}
					else
					{
						DigiMode = sds_MonoPCM;
					}
				}
				else
				{
					DigiMode = sds_StereoPCM;
				}
				break;
			default:
				DigiMode = mode;
				break;

		}
}

void
SDL_SetupDigi(void)
{
		memptr  list;
		word    *p;
		byte	*pcmdata;
		int             i;

		DigiList=malloc(PMPageSize);
		p=(word *)(Pages+Pagetables[ChunksInFile-1]);
		memcpy(DigiList,p,PMPageSize);

		pcmdata = (Pages+Pagetables[PMSoundStart]);
		while(pcmdata < (Pages+Pagetables[ChunksInFile-1]))
		{
			*pcmdata++ = uint8_to_int8_for_pcmwave[*pcmdata];
		}

		NumDigi = 128; // Sound data num. need fix

		for (i = 0;i < LASTSOUND;i++)
				DigiMap[i] = -1;
}


///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_ALStopSound(void)
{
//      _asm    cli

		alNoIRQ = true;
		
		alSound = 0;
		SND_fm_write_data(0, YM2612FreqH, 0);
		SND_fm_write_data(0, YM2612FreqL, 0);
		SND_key_off(0);

		alNoIRQ = false;

//      _asm    sti
}

static void
SDL_AlSetFXInst(Instrument *inst)
{
	//SND_fm_write_data(0, 0x28, 0x30);
	SND_fm_write_data(0, YM2612Char, inst->mChar & 0xf);
	//SND_fm_write_data(0, YM2612Char+4, inst->cChar & 0xf);
	SND_fm_write_data(0, YM2612Char+8, inst->cChar & 0xf);
	//SND_fm_write_data(0, YM2612Char+12, inst->cChar & 0xf);
	SND_fm_write_data(0, YM2612Scale, (inst->mScale) & 0x3f);
	//SND_fm_write_data(0, YM2612Scale+4, (inst->cScale) & 0x3f);
	SND_fm_write_data(0, YM2612Scale+8, (inst->cScale) & 0x3f);
	//SND_fm_write_data(0, YM2612Scale+12, (inst->cScale) & 0x3f);
	SND_fm_write_data(0, YM2612Attack, ((inst->mAttack >> 3) & 0x1e) | (inst->mScale & 0xc0));
	//SND_fm_write_data(0, YM2612Attack+4, ((inst->cAttack >> 3) & 0x1e) | (inst->cScale & 0xc0));
	SND_fm_write_data(0, YM2612Attack+8, ((inst->cAttack >> 3) & 0x1e) | (inst->cScale & 0xc0));
	//SND_fm_write_data(0, YM2612Attack+12, ((inst->cAttack >> 3) & 0x1e) | (inst->cScale & 0xc0));
	SND_fm_write_data(0, YM2612Decay, (inst->mChar & 0x80) | ((inst->mAttack << 1) & 0x1e));
	//SND_fm_write_data(0, YM2612Decay+4, (inst->cChar & 0x80) | ((inst->cAttack << 1) & 0x1e));
	SND_fm_write_data(0, YM2612Decay+8, (inst->cChar & 0x80) | ((inst->cAttack << 1) & 0x1e));
	//SND_fm_write_data(0, YM2612Decay+12, (inst->cChar & 0x80) | ((inst->cAttack << 1) & 0x1e));
	SND_fm_write_data(0, YM2612SR, (inst->mChar >> 5) & 0x1);
	//SND_fm_write_data(0, YM2612SR+4, (inst->cChar >> 5) & 0x1);
	SND_fm_write_data(0, YM2612SR+8, (inst->cChar >> 5) & 0x1);
	//SND_fm_write_data(0, YM2612SR+12, (inst->cChar >> 5) & 0x1);
	SND_fm_write_data(0, YM2612Sus, inst->mSus);
	//SND_fm_write_data(0, YM2612Sus+4, inst->cSus);
	SND_fm_write_data(0, YM2612Sus+8, inst->cSus);
	//SND_fm_write_data(0, YM2612Sus+12, inst->cSus);

	SND_fm_write_data(0, YM2612FeedCon, 6);
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the YM2612
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_ALPlaySound(AdLibSound *sound)
{
		Instrument      *inst;
		byte            *data;

		SDL_ALStopSound();

//      _asm    cli
		alNoIRQ = true;

		alLengthLeft = sound->common.length;
		data = sound->data;
		alSound = (byte *)data;
		alBlock = ((sound->block & 7) << 3);
		inst = &sound->inst;

		if (!(inst->mSus | inst->cSus))
		{
//              _asm    sti
				Quit("SDL_ALPlaySound() - Bad instrument");
		}

//      SDL_AlSetFXInst(&alZeroInst);   // DEBUG
		SND_key_on(0, 64, 127);
		SDL_AlSetFXInst(inst);

		alNoIRQ = false;

//      _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutFM() - Shuts down the YM2612 for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutFM(void)
{
//      _asm    cli
		alNoIRQ = true;

		//alOutInIRQ(alEffects,0);
		//SND_fm_write_data(0, YM2612FreqH, 0);
		SND_key_off(0);
		SDL_AlSetFXInst(&alZeroInst);
		alSound = 0;

		alNoIRQ = false;
//      _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanFM() - Totally shuts down the YM2612
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanFM(void)
{
	int     i;

	alNoIRQ = true;

	//alOutInIRQ(alEffects,0);
	for (i = 0;i < FM_CH_NUM;i++)
	{
		ym2612_registers_initialize(i);
		SND_key_abort(i);
	}

	alNoIRQ = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
		alNoIRQ = true;
		//alOutInIRQ(alEffects,alFXReg);
		SDL_AlSetFXInst(&alZeroInst);
		alNoIRQ = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_DetectAdLib() - Determines if there's an AdLib (or SoundBlaster
//              emulating an AdLib) present
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_DetectAdLib(void)
{
	for(int i = 0;i < FM_CH_NUM;i++)
	{
		ym2612_registers_initialize(i);
	}

	return(true);
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
		switch (SoundMode)
		{
		case sdm_PC:
				SDL_ShutPC();
				break;
		case sdm_AdLib:
				SDL_ShutFM();
				break;
		}
		SoundMode = sdm_Off;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanDevice() - totally shuts down all sound devices
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanDevice(void)
{
		if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
				SDL_CleanFM();
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
		switch (SoundMode)
		{
		case sdm_AdLib:
				SDL_StartAL();
				break;
		}
		SoundNumber = (soundnames) 0;
		SoundPriority = 0;
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetSoundMode(SDMode mode)
{
		boolean result = false;
		word    tableoffset;

		SD_StopSound();

#ifndef _MUSE_
		if ((mode == sdm_AdLib) && !AdLibPresent)
				mode = sdm_PC;

		switch (mode)
		{
		case sdm_Off:
				NeedsDigitized = false;
				result = true;
				break;
		case sdm_PC:
				tableoffset = STARTPCSOUNDS;
				NeedsDigitized = false;
				result = true;
				break;
		case sdm_AdLib:
				if (AdLibPresent)
				{
						tableoffset = STARTADLIBSOUNDS;
						NeedsDigitized = false;
						result = true;
				}
				break;
		}
#else
		result = true;
#endif

		if (result && (mode != SoundMode))
		{
				SDL_ShutDevice();
				SoundMode = mode;
#ifndef _MUSE_
				SoundTable = &audiosegs[tableoffset];
#endif
				SDL_StartDevice();
		}

		SDL_SetTimerSpeed();

		return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetMusicMode(SMMode mode)
{
		boolean result = false;

		SD_FadeOutMusic();
		while (SD_MusicPlaying())
				;

		switch (mode)
		{
		case smm_Off:
				NeedsMusic = false;
				result = true;
				break;
		case smm_AdLib:
				if (AdLibPresent)
				{
						NeedsMusic = true;
						result = true;
				}
				break;
		}

		if (result)
				MusicMode = mode;

		SDL_SetTimerSpeed();

		return(result);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
		int     i;

		if (SD_Started)
				return;

		SND_init(swork);
		SND_elevol_mute(0x33);

		alNoCheck = false;
		PCMNoCheck = false;
		StereoPCMNoCheck = false;
#ifndef _MUSE_
		for (i = 1;i < myargc;i++)
		{
				switch (US_CheckParm(myargv[i],ParmStrings_id_sd))
				{
				case 0:                                         // No AdLib detection
						alNoCheck = true;
						break;
				case 1:                                         // RF5C68 detection
						PCMNoCheck = true;
						break;
				case 2:                                         // RF5C68 2ch detection
						StereoPCMNoCheck = true;
						break;
				}
		}
#endif

//        SoundUserHook = 0;

		LocalTime = TimeCount = alTimeCount = 0;

		SD_SetSoundMode(sdm_Off);
		SD_SetMusicMode(smm_Off);

		if (!alNoCheck)
		{
				AdLibPresent = SDL_DetectAdLib();
		}

	if(!PCMNoCheck)
	{
		PCMPresent = true;

		if (!StereoPCMNoCheck)
		{
			StereoPCMPresent = true;
		}
	}

		if (PCMPresent)
	{
			SDL_StartPCM();
	}

		SDL_SetupDigi();

		SDL_Cache_Gun_Sounds();

		SD_Started = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Default() - Sets up the default behaviour for the Sound Mgr whether
//              the config file was present or not.
//
///////////////////////////////////////////////////////////////////////////
/* void
SD_Default(boolean gotit,SDMode sd,SMMode sm)
{
		boolean gotsd,gotsm;

		gotsd = gotsm = gotit;

		if (gotsd)      // Make sure requested sound hardware is available
		{
				switch (sd)
				{
				case sdm_AdLib:
						gotsd = AdLibPresent;
						break;
				}
		}
		if (!gotsd)
		{
				if (AdLibPresent)
						sd = sdm_AdLib;
				else
						sd = sdm_PC;
		}
		if (sd != SoundMode)
				SD_SetSoundMode(sd);


		if (gotsm)      // Make sure requested music hardware is available
		{
				switch (sm)
				{
				case sdm_AdLib:
						gotsm = AdLibPresent;
						break;
				}
		}
		if (!gotsm)
		{
				if (AdLibPresent)
						sm = smm_AdLib;
		}
		if (sm != MusicMode)
				SD_SetMusicMode(sm);
} */

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
		if (!SD_Started)
				return;

		SD_MusicOff();
		SD_StopSound();
		SDL_ShutDevice();
		SDL_CleanDevice();

		if (PCMPresent)
	{
				SDL_ShutPCM();
	}

	SND_fm_timer_a_set(0,0);
	SND_end();

		SD_Started = false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
		LeftPosition = leftvol;
		RightPosition = rightvol;
		nextsoundpos = true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_PlaySound(soundnames sound)
{
		boolean         ispos;
		SoundCommon     *s;
		int     lp,rp;


		lp = LeftPosition;
		rp = RightPosition;
		LeftPosition = 0;
		RightPosition = 0;

		ispos = nextsoundpos;
		nextsoundpos = false;

		if (sound == -1)
				return(false);

		s = (SoundCommon *) SoundTable[sound];
		
		if ((SoundMode != sdm_Off) && !s)
				Quit("SD_PlaySound() - Uncached sound");

		if ((DigiMode != sds_Off) && (DigiMap[sound] != -1))
		{
			if (sound == ATKPISTOLSND || sound == ATKMACHINEGUNSND || sound == ATKGATLINGSND)
			{
				SDL_PCMPlay_Gun(sound);
			}
			else
			{
				if (s->priority < DigiPriority)
					return(false);

				lastsoundstarted = sound;

				SD_PlayDigitized(DigiMap[sound], lp, rp);
				SoundPositioned = ispos;
				DigiNumber = sound;
				DigiPriority = s->priority;
			}

			return(true);
		}

		if (SoundMode == sdm_Off)
				return(false);

		if (!s->length)
				Quit("SD_PlaySound() - Zero length sound");
		if (s->priority < SoundPriority)
				return(false);

		switch (SoundMode)
		{
		case sdm_PC:
				SDL_PCPlaySound((PCSound *)s);
				break;
		case sdm_AdLib:
				SDL_ALPlaySound((AdLibSound *)s);
				break;
		}

		SoundNumber = sound;
		SoundPriority = s->priority;

		return(false);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
word
SD_SoundPlaying(void)
{
		boolean result = false;

		switch (SoundMode)
		{
		case sdm_PC:
				result = pcSound? true : false;
				break;
		case sdm_AdLib:
				result = alSound? true : false;
				break;
		}

		if (result)
				return(SoundNumber);
		else
				return(false);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
		if (DigiPlaying)
				SD_StopDigitized();

		switch (SoundMode)
		{
		case sdm_PC:
				SDL_PCStopSound();
				break;
		case sdm_AdLib:
				SDL_ALStopSound();
				break;
		}

		SoundPositioned = false;

		SDL_SoundFinished();
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
		while (SD_SoundPlaying())
				;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
	sqActive = true;

	for(int i = 1; i < FM_CH_NUM;i++)
	{
		SND_key_on(i, 60, 127);
		ym2612_registers_initialize(i);
	}

	SND_fm_write_data(0, YM2612FeedCon + 1, 6);
	SND_fm_write_data(0, YM2612FeedCon + 1, 6);
	SND_fm_write_data(1, YM2612FeedCon, 6);
	SND_fm_write_data(1, YM2612FeedCon + 1, 6);
	SND_fm_write_data(1, YM2612FeedCon + 2, 6);
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
	alNoIRQ = true;

	sqActive = false;
	switch (MusicMode)
	{
		case smm_AdLib:
			for(int i = 1; i < FM_CH_NUM;i++)
			{
				ym2612_registers_initialize(i);
			}
			break;
	}

	alNoIRQ = false;

	return sqHackPtr-sqHack;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(MusicGroup *music)
{
		SD_MusicOff();

		if (MusicMode == smm_AdLib)
		{
				sqHackPtr = sqHack = music->values;
				sqHackLen = sqHackSeqLen = music->length;
				sqHackTime = 0;
				alTimeCount = 0;
				SD_MusicOn();
		}
}

void
SD_ContinueMusic(MusicGroup *music,int startoffs)
{
		SD_MusicOff();

		if (MusicMode == smm_AdLib)
		{
				sqHackPtr = sqHack = music->values;
				sqHackLen = sqHackSeqLen = music->length;

				if(startoffs >= sqHackLen)
				{
						Quit("SD_StartMusic: Illegal startoffs provided!");
				}

				// fast forward to correct position
				// (needed to reconstruct the instruments)
				
				for(int i=0;i<startoffs;i+=2)
				{
						byte reg=*(byte *)sqHackPtr;
						byte val=*(((byte *)sqHackPtr)+1);
						//if(reg>=0xb1 && reg<=0xb8) val&=0xdf;           // disable play note flag
						//else if(reg==0xbd) val&=0xe0;                                   // disable drum flags
						
						//sqHackTime=alTimeCount+*(sqHackPtr+1);

						ym3812_to_ym2612(reg,val);

						sqHackPtr+=2;
						sqHackLen-=4;
				}
				sqHackTime = 0;
				alTimeCount = 0;
				
				SD_MusicOn();
		}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
		switch (MusicMode)
		{
		case smm_AdLib:
				// DEBUG - quick hack to turn the music off
				SD_MusicOff();
				break;
		}
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_MusicPlaying(void)
{
		boolean result;

		switch (MusicMode)
		{
		case smm_AdLib:
				result = false;
				// DEBUG - not written
				break;
		default:
				result = false;
		}

		return(result);
}

