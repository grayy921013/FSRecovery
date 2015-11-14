#include "file.h"
#include "struct.h"
#include <stdio.h>
#include <stdlib.h>
BootEntry *bootEntry;
void list(char *device) {
  bootEntry = malloc(sizeof(BootEntry));
  FILE *fp;
  fp = fopen(device,"rb");
  if (fp == NULL) {
    return;
  }
  pread(bootEntry,1, sizeof(BootEntry),fp);
  int startOfData = (bootEntry->BPB_RsvdSecCnt + bootEntry->BPB_NumFATs*bootEntry->BPB_FATSz32)*bootEntry->BPB_BytsPerSec;
  printf("%d\n",startOfData);
}
void recover(char *rFile, char *oFile) {
  printf("r: %s o: %s",rFile,oFile);
}
