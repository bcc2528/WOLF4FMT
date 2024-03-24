
#include "wl_def.h"

// defines

/*FILE *grfile=NULL;
FILE *mapfile=NULL;*/

int ChunksInFile;
int PMSpriteStart;
int PMSoundStart;
long *PageOffsets;
word *PageLengths;
byte *Pages;
int *Pagetables;

/*float radtoint;

short minheightdiv;

word wallspot[SCRWIDTH+1];*/

void PM_Startup(boolean (*update)(unsigned current,unsigned total))
{
		byte *offset;
		byte buf[4096];
		int Pageoffset;
		int i;
		unsigned	current,total;
		char    fname[13]={"VSWAP."};
		strcat(fname,extension);
		
		FILE *file=fopen(fname,"rb");
		if(!file)
				CA_CannotOpen(fname);
				
		ChunksInFile=0;
		fread(&ChunksInFile,sizeof(word),1,file);
		PMSpriteStart=0;
		fread(&PMSpriteStart,sizeof(word),1,file);
		PMSoundStart=0;
		fread(&PMSoundStart,sizeof(word),1,file);
		
		PageOffsets=(long *) malloc(ChunksInFile*sizeof(long));
		fread(PageOffsets,sizeof(long),ChunksInFile,file);
		
		PageLengths=(word *) malloc(ChunksInFile*sizeof(word));
		fread(PageLengths,sizeof(word),ChunksInFile,file);

		Pagetables=(int *) malloc(ChunksInFile*sizeof(int));

		Pageoffset = (PMSpriteStart - 1) * 1024;
		for(i = PMSpriteStart;i < ChunksInFile;i++)
		{
			Pageoffset += PageLengths[i];
		}
		
		// TODO: Doesn't support variable page lengths as used by the sounds (page length always <=4096 there)

		total = ChunksInFile;
		current = 0;

		Pages=(byte *) malloc(Pageoffset);
		if(Pages == NULL)
		{
			exit(0);
		}

		Pageoffset = 0;
		for(i=0;i<ChunksInFile;i++)
		{
			if(i >= PMSpriteStart)
			{
				fseek(file,PageOffsets[i],SEEK_SET);
				offset = Pages+Pageoffset;
				fread(offset,1,PageLengths[i],file);
			}
			else
			{
				fseek(file,PageOffsets[i],SEEK_SET);
				fread(buf,1,4096,file);
				offset = Pages+Pageoffset;
				PageLengths[i] = 1024;
				for(int k = 0;k < 32;k++)
				{
					for(int j = 0;j < 32;j++)
					{
						*offset++ = buf[(j * 2) + (k * 128)];
					}
				}
			}
			Pagetables[i] = Pageoffset;
			Pageoffset += PageLengths[i];
			current++;
			update(current,total);
		}

		fclose(file);
		update(total,total);
		free(PageOffsets);
}

void PM_Shutdown()
{
	free(PageLengths);
	free(Pagetables);
	free(Pages);
}

