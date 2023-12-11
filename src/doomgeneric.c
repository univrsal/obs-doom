#include <stdio.h>

#include "m_argv.h"

#include "doomgeneric.h"
#include <obs-module.h>
uint32_t *DG_ScreenBuffer = 0;

void M_FindResponseFile(void);
void D_DoomMain(void);

void doomgeneric_Create(void *context)
{
	UNUSED_PARAMETER(context);
	// save arguments
	myargc = 3;
	myargv = bzalloc(sizeof(char *) * 3);

	myargv[0] = "/technically/path/to/exe";
	myargv[1] = "-iwad";
	myargv[2] = "/home/usr/docs/git/c/doom/doom1.wad";

	M_FindResponseFile();

	DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

	D_DoomMain();
}
