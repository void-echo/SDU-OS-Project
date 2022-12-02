// filehdr.cc
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector,
//
//      Unlike in a real system, we do not keep track of file permissions,
//	ownership, last modification date, etc., in the file header.
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#include "filehdr.h"

#include <time.h>

#include "copyright.h"
#include "system.h"

//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool FileHeader::Allocate(BitMap *freeMap, int fileSize) {
    numBytes = fileSize;
    // getSecNum() = divRoundUp(fileSize, SectorSize);
    if (freeMap->NumClear() < getSecNum()) return FALSE;  // not enough space

    for (int i = 0; i < getSecNum(); i++) dataSectors[i] = freeMap->Find();
    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap) {
    for (int i = 0; i < getSecNum(); i++) {
        ASSERT(freeMap->Test((int)dataSectors[i]));  // ought to be marked!
        freeMap->Clear((int)dataSectors[i]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk.
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void FileHeader::FetchFrom(int sector) {
    synchDisk->ReadSector(sector, (char *)this);
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk.
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void FileHeader::WriteBack(int sector) {
    synchDisk->WriteSector(sector,
                           (char *)this);  // TODO HERE: HOW DID THIS WORK ?
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int FileHeader::ByteToSector(int offset) {
    return (dataSectors[offset / SectorSize]);
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength() { return numBytes; }

bool FileHeader::Append(BitMap *freeMap, int fileSize) {
    updateTime();
    int lastSectorSpace = SectorSize - (numBytes % SectorSize);
    if (fileSize > lastSectorSpace) {
        fileSize -=
            lastSectorSpace;  // Put in the last sector, as much as possible
        int newNumSectors =
            divRoundUp(fileSize, SectorSize);  // update necessary new sectors

        if (freeMap->NumClear() < newNumSectors)
            return FALSE;  // not enough space
        int oldSectorNum =
            divRoundUp(numBytes, SectorSize);  // old sector number
        numBytes += (fileSize + lastSectorSpace);
        for (int i = oldSectorNum; i < oldSectorNum + newNumSectors; i++) {
            dataSectors[i] = freeMap->Find();
        }
    } else {
        numBytes += fileSize;
    }
    return TRUE;
}

void FileHeader::updateTime() {
    // time is sec num from UTC 1970.1.1 00:00:00
    time_t now = time(NULL);
    int nowSec = (int)now;
    lastUpdatedTime = nowSec;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void FileHeader::Print() {  // TODO HERE: NEED TO PRINT FULL FILE CONTENTS
    int i, j, k;
    char *data = new char[SectorSize];

    // printf("FileHeader contents.  File size: %d, Last Updated Time: %d,  File
    // blocks:\n", numBytes, lastUpdatedTime);
    if (lastUpdatedTime != 0) {
        printf("FileHeader contents.  File size: %d,  Last Updated Time: ",
               numBytes);
    } else {
        printf("FileHeader contents.  File size: %d", numBytes);
    }
    if (lastUpdatedTime != 0) {
        time_t ___time___ = lastUpdatedTime;
        struct tm *ptm = localtime(&___time___);
        // lastUpdatedTime is sec num from UTC 1970.1.1 00:00:00
        // print time in readable format
        printf("%d-%d-%d %d:%d:%d", ptm->tm_year + 1900, ptm->tm_mon + 1,
               ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
    }
    printf(",  File blocks:\n");
    for (i = 0; i < getSecNum(); i++) printf("%d ", dataSectors[i]);
    printf("\nFile contents:\n");
    for (i = k = 0; i < getSecNum(); i++) {
        synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
            if ('\040' <= data[j] && data[j] <= '\176')  // isprint(data[j])
                printf("%c", data[j]);
            else
                printf("\\%x", (unsigned char)data[j]);
        }
        printf("\n");
    }
    delete[] data;
}
