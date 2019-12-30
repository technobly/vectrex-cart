#include <stdlib.h>
#include <string.h>

#include "menu.h"

#include "xprintf.h"
#include "fatfs/ff.h"

//Get a listing of the roms in the 'roms/' directory and
//poke them into the menu cartridge space
void loadListing(char *fdir, dir_listing *listing, char *romData, const int fn_ptrs, const int str_ptrs, const int num_files_ptr) {
	char buff[30];
	char *name;

	int ptrpos = fn_ptrs;  // fixed location in multicart.bin for 512 filename pointers (from &ptrpos ~ &strpos)
	int strpos = str_ptrs; // filename data starts here

	int i;
	int is_dir;
	int idx;
	file_entry *f_entry;

	xprintf("Reading dir: %s...\n", fdir);
	sortDirectory(fdir, listing);
	for (idx = 0; idx < listing->num_files; idx++) {
		f_entry=&(listing->f_entry[idx]);

		strncpy(buff, f_entry->fname, sizeof(buff));
		name=buff;

		is_dir=f_entry->is_dir;

		//xprintf("Found file %s (%d), file %d\n", name, is_dir, idx);
		romData[ptrpos++]=strpos>>8;
		romData[ptrpos++]=strpos&0xff;

		// remove extensions
		removeExtension(name, ".bin");
		removeExtension(name, ".BIN");
		removeExtension(name, ".vec");
		removeExtension(name, ".VEC");

		i = is_dir ? (MENU_TEXT_LEN-2) : MENU_TEXT_LEN;
		if (is_dir) romData[strpos++]='<';
		while (*name!=0 && i>0) {
			if (*name<32) {
				romData[strpos++]=' ';
			} else if (*name>=32 && *name<95) {
				romData[strpos++]=*name;
			} else if (*name>='a' && *name<='z') {
				romData[strpos++]=(*name-'a')+'A'; //convert to caps
			} else {
				romData[strpos++]='_';
			}
			name++;
			i--;
		}
		if (is_dir) romData[strpos++]='>';
		romData[strpos++]=0x80; //end of string
	}
	//finish with zero ptr
	romData[ptrpos++]=0;
	romData[ptrpos++]=0;

	romData[num_files_ptr]=listing->num_files-1;

	xprintf("Done.\n");
}

int removeExtension(char* filename, char* extension) {
	char* ptr = strstr(filename,extension);
	if (ptr) {
		*ptr = 0;
		return 1;
	}
	return 0;
}

static int f_entry_compare(const void* a, const void* b)
{
	// setting up rules for comparison
	file_entry *f_entryA = (file_entry *)a;
	file_entry *f_entryB = (file_entry *)b;

	int dirA = f_entryA->is_dir;
	int dirB = f_entryB->is_dir;

	if (dirA == dirB) {
		return (strcmp(f_entryA->fname, f_entryB->fname));
	} else {
		 return dirB - dirA;
	}
}


void sortDirectory(char *fdir, dir_listing *listing) {
	DIR d;
	FILINFO fi;
	char lfn[_MAX_LFN + 1];
	fi.lfname=lfn;
	fi.lfsize=sizeof(lfn);

	int idx = 0;
	file_entry *f_entry;
	char *name;
	int is_dir;

	// initial unsorted directory read
	f_opendir(&d, fdir);
	while (f_readdir(&d, &fi)==FR_OK) {
		// xprintf("Found file %s (%s)\n", fi.lfname, fi.fname);
		f_entry=&(listing->f_entry[idx]);

		if (fi.fname[0]==0) break;
		if (fi.fname[0]=='.') {
			if (fi.fname[1] != '.')
				continue;
			if (strcmp(fdir, "/roms") == 0)
				continue;
		}

		is_dir = (fi.fattrib & AM_DIR) ? 1 : 0;
		if (fi.lfname[0]=='.') continue; // ignore MacOS dotfiles
		name=fi.lfname;
		if (name==NULL || name[0]==0) name=fi.fname; //use short name if no long name available

		f_entry->is_dir = is_dir;
		strcpy(f_entry->fname, name);
		idx++;
	}
	f_closedir(&d);

	listing->num_files = idx;

	xprintf("Found %d files in %s\n", listing->num_files, fdir);

	// xprintf("About to sort\n");
	qsort(listing->f_entry, listing->num_files, sizeof(file_entry), f_entry_compare);
	// xprintf("Done sorting\n");
}
