#include "file.h"
#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "debug.h"
#include <string.h>
BootEntry *bootEntry;
void getBootEntry(int fp) {
  // return start of data area
  if (bootEntry != NULL) {
    free(bootEntry);
  }
  bootEntry = malloc(sizeof(BootEntry));
  pread(fp, bootEntry, sizeof(BootEntry), 0);
}
long getAddrByCluster(long cluster) {
  return (bootEntry->BPB_RsvdSecCnt + bootEntry->BPB_NumFATs*bootEntry->BPB_FATSz32)*bootEntry->BPB_BytsPerSec + (cluster - bootEntry->BPB_RootClus) * bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;
}

int getSubDirEntry(int fp, char* lFile, long start) {
  char *tok;
  DirEntry *dirEntry;
  int offset = 0;
  int i;
  dirEntry = malloc(sizeof(DirEntry));
  tok=strtok(lFile,"/");
  while(tok) {
    DEBUG(printf("%s\n",tok););
    offset = 0;
    while(pread(fp, dirEntry, sizeof(DirEntry), start+offset) >= 0) {
      if(dirEntry->DIR_Name[0] == 0) {
        //end of directory : something wrong
        return -1;
      }
      if(dirEntry->DIR_Attr == 0x0F) {
        //LFN
        offset += sizeof(DirEntry);
        continue;
      }
      if ((dirEntry->DIR_Attr & 0b10000) != 0) {
        // is a directory
        for(i=0;i<8;i++) {
          if (dirEntry->DIR_Name[i] == ' ')
            dirEntry->DIR_Name[i] = 0;
        }
        dirEntry->DIR_Name[i] = 0;
        if (strcmp(dirEntry->DIR_Name, tok) == 0) {
          //target dir
            DEBUG(printf("%s\n",dirEntry->DIR_Name););
          start = getAddrByCluster(dirEntry->DIR_FstClusHI * 65535 + dirEntry->DIR_FstClusLO);
          DEBUG(printf("start:%ld\n",start););
          break;
        }
      }
      offset += sizeof(DirEntry);
    }
    tok=strtok(NULL,"/");
  }
  return start;
}
void list(char *device, char* lFile) {
  int fp;
  long start;
  int index = 0;
  int num = 1;
  int i;
  long position;
  DirEntry *dirEntry;
  fp = open(device, O_RDONLY);
  if (fp < 0) {
    return;
  }
  getBootEntry(fp);
  start = getAddrByCluster(bootEntry->BPB_RootClus);
  DEBUG(printf("%ld\n",start););
  if (strcmp(lFile,"/") != 0) {
    //not root, go into the deepest dir recursively
    start = getSubDirEntry(fp, lFile,start);
  }
  dirEntry = malloc(sizeof(DirEntry));
  while(pread(fp, dirEntry, sizeof(DirEntry), start+index*sizeof(DirEntry)) >= 0) {
    if(dirEntry->DIR_Name[0] == 0) {
      //end of directory
      break;
    }
    if(dirEntry->DIR_Attr == 0x0F) {
      //LFN
      index++;
      continue;
    }
    printf("%d, ",num);
    //print file name
    for(i=0;i<11;i++) {
      if (i==8 && dirEntry->DIR_Name[i] != ' ')
        putchar('.');
      if (dirEntry->DIR_Name[i] == 0xe5)
        putchar('?');
      else if(dirEntry->DIR_Name[i] == ' ')
        continue;
      else
        putchar(dirEntry->DIR_Name[i]);
    }
    if ((dirEntry->DIR_Attr & 0b10000) != 0) {
      // is a directory
      putchar('/');
    }
    position = dirEntry->DIR_FstClusHI * 65535 + dirEntry->DIR_FstClusLO;
    printf(", %d, %ld\n",dirEntry->DIR_FileSize,position);
    index++;
    num++;
  }
  free(dirEntry);
  close(fp);
}
void recover(char *rFile, char *oFile) {
  printf("r: %s o: %s",rFile,oFile);
}
