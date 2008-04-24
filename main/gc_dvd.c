#include <stdio.h>
#include <stdlib.h>
#include <ogc/dvd.h>
#include <malloc.h>
#include <string.h>
#include <gccore.h>
#include "gc_dvd.h"

int last_current_dir = -1;
volatile unsigned long* dvd = (volatile unsigned long*)0xCC006000;
struct
{
	char name[128];
	int flags;
	int sector, size;
} file[MAXIMUM_ENTRIES_PER_DIR] __attribute__((aligned(32))); //150 files per dir, MAXIMUM.
unsigned char sector_buffer[2048] __attribute__((aligned(32)));

int dvd_read_id()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xA8000040;
	dvd[3] = 0;
	dvd[4] = 0x20;
	dvd[5] = 0x80000000;
	dvd[6] = 0x20;
	dvd[7] = 3; // enable reading!
	while (dvd[7] & 1);
	if (dvd[0] & 0x4)
		return 1;
	return 0;
}

unsigned int dvd_get_error(void)
{
	dvd[2] = 0xE0000000;
	dvd[8] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
	return dvd[8];
}


void dvd_motor_off()
{
	dvd[0] = 0x2E;
	dvd[1] = 0;
	dvd[2] = 0xe3000000;
	dvd[3] = 0;
	dvd[4] = 0;
	dvd[5] = 0;
	dvd[6] = 0;
	dvd[7] = 1; // IMM
	while (dvd[7] & 1);
}

int dvd_read(void* dst, unsigned int len, unsigned int offset)
{
	if ((((int)dst) & 0xC0000000) == 0x80000000) // cached?
		dvd[0] = 0x2E;
	dvd[1] = 0;

	dvd[2] = 0xA8000000;
	dvd[3] = offset >> 2;
	dvd[4] = len;
	dvd[5] = (unsigned long)dst;
	dvd[6] = len;
	dvd[7] = 3; // enable reading!
	DCInvalidateRange(dst, len);
	while (dvd[7] & 1);

	if (dvd[0] & 0x4)
		return 1;
		
	return 0;
}

int read_sector(void* buffer, int sector)
{
	return dvd_read(buffer, 2048, sector * 2048);
}

int read_safe(void* dst, int offset, int len)
{
	int ol = len;
	
	while (len)
	{
		int sector = offset / 2048;
		read_sector(sector_buffer, sector);
		int off = offset & 2047;

		int rl = 2048 - off;
		if (rl > len)
			rl = len;
		memcpy(dst, sector_buffer + off, rl);	

		offset += rl;
		len -= rl;
		dst += rl;
	}
	return ol;
}

int read_direntry(unsigned char* direntry)
{
       int nrb = *direntry++;
       ++direntry;

       int sector;

       direntry += 4;
       sector = (*direntry++) << 24;
       sector |= (*direntry++) << 16;
       sector |= (*direntry++) << 8;
       sector |= (*direntry++);        

       int size;

       direntry += 4;

       size = (*direntry++) << 24;
       size |= (*direntry++) << 16;
       size |= (*direntry++) << 8;
       size |= (*direntry++);

       direntry += 7; // skip date

       int flags = *direntry++;
       ++direntry; ++direntry; direntry += 4;

       int nl = *direntry++;

       char* name = file[files].name;

       file[files].sector = sector;
       file[files].size = size;
       file[files].flags = flags;

       if ((nl == 1) && (direntry[0] == 1)) // ".."
       {
               file[files].name[0] = 0;
               if (last_current_dir != sector)
                       files++;
       }
       else if ((nl == 1) && (direntry[0] == 0))
       {
               last_current_dir = sector;
       }
       else
       {
               if (is_unicode)
               {
                       int i;
                       for (i = 0; i < (nl / 2); ++i)
                               name[i] = direntry[i * 2 + 1];
                       name[i] = 0;
                       nl = i;
               }
               else
               {
                       memcpy(name, direntry, nl);
                       name[nl] = 0;
               }

               if (!(flags & 2))
               {
                       if (name[nl - 2] == ';')
                               name[nl - 2] = 0;

                       int i = nl;
                       while (i >= 0)
                               if (name[i] == '.')
                                       break;
                               else
                                       --i;

                       ++i;

               }
               else
               {
                       name[nl++] = '/';
                       name[nl] = 0;
               }

               files++;
       }

       return nrb;
}



void read_directory(int sector, int len)
{
       read_sector(sector_buffer, sector);

       
       int ptr = 0;
       files = 0;
       memset(file,0,sizeof(file));
       while (len > 0)
       {
               ptr += read_direntry(sector_buffer + ptr);
               if (!sector_buffer[ptr])
               {
                       len -= 2048;
                       read_sector(sector_buffer, ++sector);
                       ptr = 0;
               }
       }
}

int dvd_read_directoryentries(unsigned int offset, int size) {
       int sector = 16;
       static unsigned char bufferDVD[2048] __attribute__((aligned(32)));
       
       struct pvd_s* pvd = 0;
       struct pvd_s* svd = 0;
       while (sector < 32)
       {
               if (read_sector(bufferDVD, sector))
                       return FATAL_ERROR;
               if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\2CD001\1", 8))
               {
                       svd = (void*)bufferDVD;
                       break;
               }
               ++sector;
       }


       if (!svd)
       {
               sector = 16;
               while (sector < 32)
               {
                       if (read_sector(bufferDVD, sector))
                               return FATAL_ERROR;

                       if (!memcmp(((struct pvd_s *)bufferDVD)->id, "\1CD001\1", 8))
                       {
                               pvd = (void*)bufferDVD;
                               break;
                       }
                       ++sector;
               }
       }

       if ((!pvd) && (!svd))
       {
               return NO_ISO9660_DISC;
       }

       files = 0;
       if (svd)
       {
               is_unicode = 1;
               read_direntry(svd->root_direntry);
       }
       else
       {
               is_unicode = 0;
               read_direntry(pvd->root_direntry);
       }
       if((size + offset) == 0) {
               // enter root
               read_directory(file[0].sector, file[0].size);
       }
       else
               read_directory((offset/2048), size);
               

       
       if(files>0)
               return files;
       return NO_FILES;
}




