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
    printf("fileSize: %d \n", fileSize);
    // numSectors = divRoundUp(fileSize, SectorSize);
    printf("numSectors: %d \n", getSecNum());

    if (freeMap->NumClear() < getSecNum())
        return FALSE;  // not enough space
    else if (NumDirect + NumDirect2 <= getSecNum())
        return FALSE;  // second index do not have enough space

    int lastIndex = NumDirect - 1;  // the last one of the first index
    if (getSecNum() < lastIndex) {
        for (int i = 0; i < getSecNum(); i++) {
            dataSectors[i] = freeMap->Find();  // Allocate sectors
        }
        dataSectors[lastIndex] = -1;  // the last one of the first inode is -1
        printf("************Now the last is: %d***************** \n",
               dataSectors[lastIndex]);

    } else {
        for (int i = 0; i < lastIndex; i++) {
            dataSectors[i] = freeMap->Find();  // Allocate sectors
        }
        dataSectors[lastIndex] =
            freeMap->Find();  // find a sector for the second inode

        int dataSector2[NumDirect2];
        for (int i = 0; i < getSecNum() - NumDirect; i++) {
            dataSector2[i] = freeMap->Find();  // Allocate sectors
        }
        synchDisk->WriteSector(dataSectors[lastIndex],
                               (char *)dataSector2);  // write back to the disk
    }

    return TRUE;
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void FileHeader::Deallocate(BitMap *freeMap) {
    int lastIndex = NumDirect - 1;
    if (dataSectors[lastIndex] == -1) {  //没用到二级索引的时候的释放
        for (int i = 0; i < getSecNum(); i++) {
            ASSERT(freeMap->Test((int)dataSectors[i]));
            freeMap->Clear((int)dataSectors[i]);
        }
    } else {  //用到了二级索引
        int i;
        for (i = 0; i < lastIndex; i++) {  //先把一级索引释放了
            ASSERT(freeMap->Test((int)dataSectors[i]));
            freeMap->Clear((int)dataSectors[i]);
        }
        int dataSector2[NumDirect2];
        synchDisk->ReadSector(dataSectors[lastIndex],
                              (char *)dataSector2);  //先找到二级索引节点
        freeMap->Clear((int)dataSectors[lastIndex]);
        for (i; i < getSecNum(); i++) {
            freeMap->Clear(
                (int)dataSector2[i - lastIndex]);  //释放二级索引节点内容
        }
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
    // updateTime();
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
    int lastIndex = NumDirect - 1;
    if (offset / SectorSize < lastIndex) {
        return (dataSectors[offset / SectorSize]);
    } else {
        int dataSectors2[NumDirect2];
        synchDisk->ReadSector(dataSectors[lastIndex], (char *)dataSectors2);
        return (dataSectors2[offset / SectorSize - lastIndex]);
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int FileHeader::FileLength() { return numBytes; }

bool FileHeader::Append(BitMap *freeMap, int fileSize) {
    printf("Append:\n");
    int restFileSize =
        SectorSize * getSecNum() -
        numBytes;  // the size of the sector which haven't be used
    if (restFileSize >= fileSize) {
        numBytes = numBytes + fileSize;
        printf(
            "***********************the current sector is "
            "enough*********************** \n");
        return TRUE;
    } else {
        printf(
            "***********************the current sector is not "
            "enough*********************** \n");
        int moreFileSize =
            fileSize - restFileSize;  // the current sector is not enough, need
                                      // to be extend
        printf("in Append: moreFileSize: %d \n", moreFileSize);

        if (freeMap->NumClear() < divRoundUp(moreFileSize, SectorSize)) {
            return FALSE;
        } else if (NumDirect + NumDirect2 <=
                   getSecNum() +
                       divRoundUp(moreFileSize,
                                  SectorSize)) {  // the disk is enough but the
                                                  // sum of the first inode and
                                                  // th second is not enough
            return FALSE;
        }
        int i = getSecNum();
        numBytes = numBytes + fileSize;

        // int AppendSector = divRoundUp(moreFileSize,SectorSize); //caculate
        // the sector numbers need to be extend numSectors = numSectors +
        // AppendSector;

        int lastIndex = NumDirect - 1;

        if (dataSectors[lastIndex] == -1) {
            if (getSecNum() < lastIndex) {
                printf(
                    "***********************now the first inode is "
                    "enough*********************** \n");
                for (; i < getSecNum(); i++) {
                    dataSectors[i] = freeMap->Find();
                }
            } else {  // extend the second inode
                printf(
                    "***********************now the first inode is not "
                    "enough*********************** \n");
                for (; i < lastIndex; i++) {
                    dataSectors[i] =
                        freeMap->Find();  // allocate the first inode
                }
                dataSectors[lastIndex] =
                    freeMap->Find();  // set the sector of the second inode to
                                      // the last of the first inode
                int dataSector2[NumDirect2];
                for (; i < getSecNum(); i++) {
                    dataSector2[i - lastIndex] =
                        freeMap->Find();  // allocate the second inode
                }
                synchDisk->WriteSector(dataSectors[lastIndex],
                                       (char *)dataSector2);  // write back
            }
        } else {  // have use the second inode, but the extend files haven't
                  // exceed the size of second inode
            int dataSector2[NumDirect2];
            synchDisk->ReadSector(
                dataSectors[lastIndex],
                (char *)dataSector2);  // find the second inode
            for (; i < getSecNum(); i++) {
                dataSector2[i - lastIndex] =
                    freeMap->Find();  // allocate the second inode
            }
            synchDisk->WriteSector(dataSectors[lastIndex],
                                   (char *)dataSector2);  // write back
        }
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
    printf("Print:\n");
    int i, j, k;
    int lastIndex = NumDirect - 1;
    char *data = new char[SectorSize];

    if (dataSectors[lastIndex] == -1) {
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
    } else {
        int dataSectors2[NumDirect2];
        synchDisk->ReadSector(dataSectors[lastIndex], (char *)dataSectors2);
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
        for (i = 0; i < lastIndex; i++) printf("%d ", dataSectors[i]);
        for (; i < getSecNum(); i++) printf("%d ", dataSectors2[i - lastIndex]);
        printf("\nFile contents:\n");
        for (i = k = 0; i < lastIndex; i++) {
            synchDisk->ReadSector(dataSectors[i], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }
        for (; i < getSecNum(); i++) {
            synchDisk->ReadSector(dataSectors2[i - lastIndex], data);
            for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                if ('\040' <= data[j] && data[j] <= '\176')
                    printf("%c", data[j]);
                else
                    printf("\\%x", (unsigned char)data[j]);
            }
            printf("\n");
        }

        delete[] data;
    }
}
