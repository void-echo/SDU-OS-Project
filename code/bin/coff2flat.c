/*
 Copyright (c) 1992 The Regents of the University of California.
 All rights reserved.  See copyright.h for copyright notice and limitation 
 of liability and disclaimer of warranty provisions.
 */

/* This program reads in a COFF format file, and outputs a flat file --
 * the flat file can then be copied directly to virtual memory and executed.
 * In other words, the various pieces of the object code are loaded at 
 * the appropriate offset in the flat file.
 *
 * Assumes coff file compiled with -N -T 0 to make sure it's not shared text.
 */

#define MAIN
#include "copyright.h"
#undef MAIN
/*
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include <reloc.h>
#include <syms.h>
*/



#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>		// equivalent to unistd.h.
#else
#include <unistd.h>
#endif
#include "coff.h"
#include "noff.h"

/* NOTE -- once you have implemented large files, it's ok to make this bigger! */
#define StackSize      		1024      /* in bytes */
#define ReadStruct(f,s) 	Read(f,(char *)&s,sizeof(s))

// extern char *malloc();

unsigned int
WordToHost(unsigned int word) {
#ifdef HOST_IS_BIG_ENDIAN
	 register unsigned long result;
	 result = (word >> 24) & 0x000000ff;
	 result |= (word >> 8) & 0x0000ff00;
	 result |= (word << 8) & 0x00ff0000;
	 result |= (word << 24) & 0xff000000;
	 return result;
#else 
	 return word;
#endif /* HOST_IS_BIG_ENDIAN */
}

unsigned short
ShortToHost(unsigned short shortword) {
#if HOST_IS_BIG_ENDIAN
	 register unsigned short result;
	 result = (shortword << 8) & 0xff00;
	 result |= (shortword >> 8) & 0x00ff;
	 return result;
#else 
	 return shortword;
#endif /* HOST_IS_BIG_ENDIAN */
}


/* read and check for error */
void Read(int fd, char *buf, int nBytes)
{
    if (read(fd, buf, nBytes) != nBytes) {
	fprintf(stderr, "File is too short\n");
	exit(1);
    }
}

/* write and check for error */
void Write(int fd, char *buf, int nBytes)
{
    if (write(fd, buf, nBytes) != nBytes) {
	fprintf(stderr, "Unable to write file\n");
	exit(1);
    }
}

/* do the real work */
int main (int argc, char **argv)
{
    int fdIn, fdOut, numsections, i, top, tmp;
    struct filehdr fileh;
    struct aouthdr systemh;
    struct scnhdr *sections;
    char *buffer;

    if (argc < 2) {
	fprintf(stderr, "Usage: %s <coffFileName> <flatFileName>\n", argv[0]);
	exit(1);
    }
    
/* open the COFF file (input) */
    fdIn = open(argv[1], O_RDONLY, 0);
    if (fdIn == -1) {
	perror(argv[1]);
	exit(1);
    }

/* open the NOFF file (output) */
    fdOut = open(argv[2], O_WRONLY|O_CREAT|O_TRUNC , 0666);
    if (fdIn == -1) {
	perror(argv[2]);
	exit(1);
    }
    
/* Read in the file header and check the magic number. */
    ReadStruct(fdIn,fileh);
    fileh.f_magic = ShortToHost(fileh.f_magic);
    fileh.f_nscns = ShortToHost(fileh.f_nscns); 
    if (fileh.f_magic != MIPSELMAGIC) {
	fprintf(stderr, "File is not a MIPSEL COFF file\n");
	exit(1);
    }
    
/* Read in the system header and check the magic number */
    ReadStruct(fdIn,systemh);
    systemh.magic = ShortToHost(systemh.magic);
    if (systemh.magic != OMAGIC) {
	fprintf(stderr, "File is not a OMAGIC file\n");
	exit(1);
    }
    
/* Read in the section headers. */
    numsections = fileh.f_nscns;
    sections = (struct scnhdr *)malloc(fileh.f_nscns * sizeof(struct scnhdr));
    Read(fdIn, (char *) sections, fileh.f_nscns * sizeof(struct scnhdr));

 /* Copy the segments in */
    printf("Loading %d sections:\n", fileh.f_nscns);
    for (top = 0, i = 0; i < fileh.f_nscns; i++) {
	printf("\t\"%s\", filepos 0x%lx, mempos 0x%lx, size 0x%lx\n",
	      sections[i].s_name, sections[i].s_scnptr,
	      sections[i].s_paddr, sections[i].s_size);
	if ((sections[i].s_paddr + sections[i].s_size) > top)
	    top = sections[i].s_paddr + sections[i].s_size;
	if (strcmp(sections[i].s_name, ".bss") && /* no need to copy if .bss */
	    	strcmp(sections[i].s_name, ".sbss")) {
	    lseek(fdIn, sections[i].s_scnptr, 0);
	    buffer = malloc(sections[i].s_size);
	    Read(fdIn, buffer, sections[i].s_size);
	    Write(fdOut, buffer, sections[i].s_size);
	    free(buffer);
	}
    }
/* put a blank word at the end, so we know where the end is! */
    printf("Adding stack of size: %d\n", StackSize);
    lseek(fdOut, top + StackSize - 4, 0);
    tmp = 0;
    Write(fdOut, (char *)&tmp, 4);

    close(fdIn);
    close(fdOut);
}
