
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

/*float radtoint;

short minheightdiv;

word wallspot[SCRWIDTH+1];*/

void PM_Startup(boolean (*update)(unsigned current,unsigned total))
{
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
		
		// TODO: Doesn't support variable page lengths as used by the sounds (page length always <=4096 there)

		total = ChunksInFile;
		current = 0;

		Pages=(byte *) malloc(ChunksInFile*4096);
		for(int i=0;i<ChunksInFile;i++)
		{
			fseek(file,PageOffsets[i],SEEK_SET);
			fread(Pages+i*4096,1,4096,file);
			current++;
			update(current,total);
		}

		fclose(file);
		update(total,total);
}

void PM_Shutdown()
{
	free(PageOffsets);
	free(PageLengths);
	free(Pages);
}

