/*
//
// Copyright (C) 2009, 2010, 2011, 2012 Jean-François DEL NERO
//
// This file is part of the HxCFloppyEmulator file selector.
//
// HxCFloppyEmulator file selector may be used and distributed without restriction
// provided that this copyright statement is not removed from the file and that any
// derivative work contains the original copyright notice and the associated
// disclaimer.
//
// HxCFloppyEmulator file selector is free software; you can redistribute it
// and/or modify  it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// HxCFloppyEmulator file selector is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
//   See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with HxCFloppyEmulator file selector; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
*/

/* #include <stdio.h> */
/* #include <stdlib.h> */
#include <string.h>

/* #include <time.h> */

#include "keysfunc_defs.h"

#include "gui_utils.h"
#include "cfg_file.h"
#include "hxcfeda.h"
#include "fat_access.h"
#include "dir.h"
#include "filelist.h"


#include "atari_hw.h"

/* #include "atari_regs.h" */

/* #include "fat_opts.h" */
/* #include "fat_misc.h" */
/* #include "fat_defs.h" */
/* #include "fat_filelib.h" */
#include "conf.h"


static unsigned long indexptr;
static unsigned long last_setlbabase;
static unsigned char sector[512];
static unsigned char cfgfile_header[512];

static unsigned char currentPath[4*256] = {"\\"};

static unsigned char sdfecfg_file[2048];
static char filter[17];


static disk_in_drive disks_slot_a[NUMBER_OF_SLOT];
static disk_in_drive disks_slot_b[NUMBER_OF_SLOT];
static UWORD _filelistCurrentPage_tab[120];

static struct fs_dir_list_status file_list_status;
static struct fat_dir_entry sfEntry;
static struct fs_dir_ent dir_entry;
extern  struct fatfs _fs;

extern unsigned short SCREEN_XRESOL;
extern unsigned short SCREEN_YRESOL;




//
// Externs
//
extern unsigned char NUMBER_OF_FILE_ON_DISPLAY;

#define MAXFILESPERPAGE 120


//
// Static variables
//
static UWORD _currentPage = 0xffff;
static UWORD _selectorPos;
static signed short _invertedLine;
static UWORD _filelistCurrentPage_tab[MAXFILESPERPAGE];
static UBYTE _isLastPage;
static UWORD _nbPages;

DirectoryEntry   _dirEntLSB;
DirectoryEntry * gfl_dirEntLSB_ptr = &_dirEntLSB;





void _clear_filelistCurrentPage(void)
{
	memset(&_filelistCurrentPage_tab[0], 0xff, 2*MAXFILESPERPAGE);
}

#if(0)
void gfl_jumpToFile(UWORD askedFile)
{
	// regarde déjà si le fichier est déjà affiché
	// si non, fait la recherche, en modifiant filelistCurrentPage
	// dès qu'il est trouvé, faire gfl_showFilesForPage, avec forceRedraw

	UWORD nbPages, runPage, runPos;
	nbPages = dir_getNbPages();
	struct fs_dir_ent dir_entry;

	for (runPage=0; runPage<nbPages; runPage++)
	{
		curFile = dir_getFirstFileForPage(runPage);

		for (runPos=0; runPos<NUMBER_OF_FILE_ON_DISPLAY; runPos++)
		{
			if ((curFile == askedFile) || (0xffff == curFile)) {
				break;
			}

			do {
				curFile++;
				fli_getDirEntryMSB(curFile, &dir_entry);
			} while (!dir_filter(&dir_entry));
		} // for (runPos=0;

		if (curFile == askedFile) {
			gfl_showFilesForPage(runPage);
		}

	} // for (runPage=0;
}
#endif




void gfl_showFilesForPage(UBYTE fRepaginate, UBYTE fForceRedrawAll)
{
	static UWORD _oldPage = 0xffff;
	UWORD i;
	UWORD y_pos;
	UBYTE fForceRedraw=0;

	if (fRepaginate) {
		dir_paginate();
		_nbPages = dir_getNbPages();
		_selectorPos = 0;
		_currentPage = 0;
		fForceRedraw = 1;
	}

	if ( (_oldPage != _currentPage) || fForceRedraw || fForceRedrawAll )
	{
		UWORD curFile;
		struct fs_dir_ent dir_entry;

		_oldPage = _currentPage;

		// start at the first file of the directory
//			dir_getFilesForPage(_currentPage, _filelistCurrentPage_tab);
		if (fForceRedrawAll) {
			clear_list(5);
		} else {
			clear_list(0);
		}

		_invertedLine = -1;	// the screen has been clear: no need to de-invert the line

		_clear_filelistCurrentPage();

		// page number
		hxc_printf(0,PAGE_X_POS, PAGE_Y_POS, "Page %d of %d      ", _currentPage+1, _nbPages);

		// filter
		hxc_printf(0, FILTER_X_POS, FILTER_Y_POS,"Filter (F1): [%s]", dir_getFilter());

		y_pos=FILELIST_Y_POS;

		curFile = dir_getFirstFileForPage(_currentPage);
		fli_getDirEntryMSB(curFile, &dir_entry);

		for (i=0; i<NUMBER_OF_FILE_ON_DISPLAY; i++)
		{
			hxc_printf(0,0,y_pos," %c%s", (dir_entry.is_dir)?(10):(' '), dir_entry.filename);
			y_pos=y_pos+8;

			_filelistCurrentPage_tab[i] = curFile;
			do {
				curFile++;
				if (!fli_getDirEntryMSB(curFile, &dir_entry)) {
					i = NUMBER_OF_FILE_ON_DISPLAY;
					break;
				}
				;
			} while (!dir_filter(&dir_entry));

		}

		_invertedLine = -1;
		_isLastPage = ((_currentPage+1) == _nbPages);
	} // if ( (newPage != _currentPage) || (0xffff == _currentPage) )




	if (_invertedLine != _selectorPos) {
		// reset the inverted line
		if (_invertedLine>=0) {
			invert_line(_invertedLine);
			hxc_printf(0, 0, FILELIST_Y_POS+(_invertedLine*8), " ");
		}

		// set the inverted line (only if it has changed)
		_invertedLine = _selectorPos;
		hxc_printf(0, 0, FILELIST_Y_POS+(_selectorPos*8), ">");
		invert_line(_selectorPos);
	}

	fli_getDirEntryLSB(_filelistCurrentPage_tab[_selectorPos], gfl_dirEntLSB_ptr);
}




long gfl_mainloop()
{
	long key, key_ret;
	key_ret = 0;
	key = wait_function_key();

	switch((UWORD) (key>>16))
	{
		case 0x48: /* UP */
			if (_selectorPos > 0) {
				_selectorPos--;
				break;
			}

			// top line
			if (0==_currentPage) {
				// first page: stuck
				break;
			}

			// top line not on the first page
			_selectorPos=NUMBER_OF_FILE_ON_DISPLAY-1;
			_currentPage--;
			break;

		case 0x50: /* Down */
			if ( (_selectorPos+1)==NUMBER_OF_FILE_ON_DISPLAY ) {
				// last line of display
				if (!_isLastPage) {
					_currentPage++;
					_selectorPos = 0;
				}
				break;
			} else if (0xffff != _filelistCurrentPage_tab[_selectorPos+1]) {
				// next file exist: allow down
				_selectorPos++;
			}
			break;

		case 0x0150: /* Shift Down : go next page */
		case 0x0250:
			/* TODO: better handling */
			if ( (_nbPages > 1) && (!_isLastPage) ) {
				_currentPage++;
				_selectorPos=0;
			}
			break;

		case 0x148: /* Shift Up: go previous page */
		case 0x248:
			if (_currentPage) {
				_currentPage--;
			}
			_selectorPos=0;
			break;

		case 0x448:	/* Ctrl Up : go to first page, first file */
			_currentPage=0;
			_selectorPos=0;
			break;

		case 0x450: /* Ctrl Down: go to last file */
			/* TODO : currently it only go to the first file of the last page ! */
			_currentPage = _nbPages - 1;
			_selectorPos=0;
			break;


		default:
			//key not processed: return it (otherwise, return 0)
			key_ret = key;
	}

	return key_ret;
}