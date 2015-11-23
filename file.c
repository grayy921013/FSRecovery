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
long getClusterSize() {
  return bootEntry->BPB_BytsPerSec * bootEntry->BPB_SecPerClus;
}
long getNextCluster(long cluster, int fp) {
  uint32_t next;
  pread(fp, &next, 4, bootEntry->BPB_RsvdSecCnt*bootEntry->BPB_BytsPerSec + cluster*4);
  DEBUG(printf("Next cluster: %ld\n",next););
  return next;
}
long getAddrByCluster(long cluster) {
  return (bootEntry->BPB_RsvdSecCnt + bootEntry->BPB_NumFATs*bootEntry->BPB_FATSz32)*bootEntry->BPB_BytsPerSec + (cluster - bootEntry->BPB_RootClus) * getClusterSize();
}
long getSubDirEntry(int fp, char* dir, long cluster) {
  int offset = 0;
  int i;
  DirEntry *dirEntry;
  dirEntry = malloc(sizeof(DirEntry));
  while(cluster > 0 && pread(fp, dirEntry, sizeof(DirEntry), getAddrByCluster(cluster)+offset) >= 0) {
    if(dirEntry->DIR_Name[0] == 0) {
      //end of directory : something wrong
      return -1;
    }
    if(dirEntry->DIR_Attr == 0x0F) {
      //LFN
      offset += sizeof(DirEntry);
      if (offset == getClusterSize()) {
        cluster = getNextCluster(cluster,fp);
        offset =0;
      }
      continue;
    }
    if ((dirEntry->DIR_Attr & 0b10000) != 0) {
      // is a directory
      for(i=0;i<8;i++) {
        if (dirEntry->DIR_Name[i] == ' ')
          dirEntry->DIR_Name[i] = 0;
      }
      dirEntry->DIR_Name[i] = 0;
      if (strcmp(dirEntry->DIR_Name, dir) == 0) {
        cluster = dirEntry->DIR_FstClusHI * 65535 + dirEntry->DIR_FstClusLO;
        DEBUG(printf("start:%ld\n",cluster););
        return cluster;
      }
    }
    offset += sizeof(DirEntry);
    if (offset == getClusterSize()) {
      cluster = getNextCluster(cluster,fp);
      offset =0;
    }
  }
  return -1;
}
long getSubDirEntryByPath(int fp, char* lFile, long cluster) {
  char *tok;
  tok=strtok(lFile,"/");
  while(tok) {
    DEBUG(printf("dir: %s\n",tok););
    cluster = getSubDirEntry(fp, tok, cluster);
    tok=strtok(NULL,"/");
  }
  return cluster;
}
void list(char *device, char* lFile) {
  int fp;
  long cluster;
  int offset = 0;
  int num = 1;
  int i;
  long position;
  DirEntry *dirEntry;
  fp = open(device, O_RDONLY);
  if (fp < 0) {
    return;
  }
  getBootEntry(fp);
  cluster = bootEntry->BPB_RootClus;
  DEBUG(printf("%ld\n",cluster););
  if (strcmp(lFile,"/") != 0) {
    //not root, go into the deepest dir recursively
    cluster = getSubDirEntryByPath(fp, lFile,cluster);
  }
  dirEntry = malloc(sizeof(DirEntry));
  while(cluster < 0x0ffffff7 && pread(fp, dirEntry, sizeof(DirEntry), getAddrByCluster(cluster)+offset) >= 0) {
    if(dirEntry->DIR_Name[0] == 0) {
      //end of directory
      break;
    }
    if(dirEntry->DIR_Attr == 0x0F) {
      //LFN
      offset += sizeof(DirEntry);
      if (offset == getClusterSize()) {
        //arrive at the cluster edge
          cluster = getNextCluster(cluster,fp);
          offset = 0;
      }
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
    offset += sizeof(DirEntry);
    if (offset == getClusterSize()) {
      //arrive at the cluster edge
        cluster = getNextCluster(cluster,fp);
        offset = 0;
    }
    num++;
  }
  free(dirEntry);
  close(fp);
}
DirEntry *getFileEntry(int fp, char* filename, long cluster) {
  int offset = 0;
  int i;
  DirEntry *dirEntry;
  dirEntry = malloc(sizeof(DirEntry));
  char name[11];
  for(i=0;i<11;i++){
    name[i] = ' ';
  }
  DEBUG(printf("finding file:%s in cluster %ld\n",filename,cluster););
  if (strstr(filename,".") != NULL) {
    filename = strtok(filename,".");
    memcpy(name,filename, strlen(filename));
    filename = strtok(NULL,".");
    memcpy(name+8,filename, strlen(filename));
  } else {
    memcpy(name,filename, strlen(filename));
  }
  while(cluster < 0x0ffffff7 && pread(fp, dirEntry, sizeof(DirEntry), getAddrByCluster(cluster)+offset) >= 0) {
    if(dirEntry->DIR_Name[0] == 0) {
      //end of directory : something wrong
      free(dirEntry);
      return NULL;
    }
    if(dirEntry->DIR_Attr == 0x0F) {
      //LFN
      offset += sizeof(DirEntry);
      if (offset == getClusterSize()) {
        cluster = getNextCluster(cluster,fp);
        offset =0;
      }
      continue;
    }
    if (dirEntry->DIR_Name[0] == 0xe5) {
      // is a directory
      for(i=1;i<11;i++) {
        if(name[i] != dirEntry->DIR_Name[i]) {
          break;
        }
      }
      if (i ==11) {
        //find a match
        return dirEntry;
      }
    }
    offset += sizeof(DirEntry);
    if (offset == getClusterSize()) {
      cluster = getNextCluster(cluster,fp);
      offset =0;
    }
  }
  free(dirEntry);
  return NULL;
}
void recover(char* device, char *rFile, char *oFile) {
  FILE *output;
  int depth = 0;
  int i;
  char *tok;
  long cluster;
  int fp;
  char* fileBuffer;
  char filename[1024];
  int fileSize;
  DirEntry *dirEntry;
  strcpy(filename,rFile);
  output = fopen(oFile,"wb");
  fp = open(device, O_RDONLY);
  if (fp < 0) {
    return;
  }
  getBootEntry(fp);
  cluster = bootEntry->BPB_RootClus;
  if (output <= 0) {
    printf("[%s]: failed to open\n", oFile);
    return;
  }
  for(i=0;i<strlen(rFile);i++) {
    if(rFile[i]=='/')
      depth++;
  }
  if (depth == 1) {
    tok = strtok(rFile,"/");
  } else {
    tok = strtok(rFile,"/");
    cluster = getSubDirEntry(fp, tok, cluster);
    depth--;
    while (depth > 1) {
      //tok =
      tok = strtok(NULL,"/");
      cluster = getSubDirEntry(fp, tok, cluster);
      depth --;
    }
    tok = strtok(NULL,"/");
  }
  dirEntry = getFileEntry(fp,tok,cluster);
  if (dirEntry == NULL) {
    printf("[%s]: error - file not found\n", filename);
    return;
  }
  cluster = dirEntry->DIR_FstClusHI * 65535 + dirEntry->DIR_FstClusLO;
  DEBUG(printf("file cluster:%ld\n",cluster););
  fileSize = dirEntry->DIR_FileSize;
  DEBUG(printf("file size:%ld\n",fileSize););
  if (cluster != 0 && getNextCluster(cluster,fp) != 0) {
      printf("[%s]: error - fail to recover\n", filename);
      return;
  }

  fileBuffer = malloc(fileSize);
  DEBUG(printf("file location: %ld\n", getAddrByCluster(cluster)););
  pread(fp, fileBuffer, fileSize, getAddrByCluster(cluster));
  close(fp);
  fwrite(fileBuffer, 1, fileSize, output);
  fclose(output);
  free(fileBuffer);
  free(dirEntry);
}
