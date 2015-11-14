#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "file.h"
#include "debug.h"

void printUsage(char* path) {
  printf("Usage: ");
  printf("%s",path);
  printf(" -d [device filename] [other arguments]\n-l target            List the target directory\n-r target -o dest    Recover the target pathname\n");
}

int main(int argc, char** argv) {
  char *device = NULL;
  char *lFile = NULL;
  char *rFile = NULL;
  char *oFile = NULL;
  int err = 0;
  int c;
  opterr = 0;
  while ((c = getopt (argc, argv, "d:l:r:o:")) != -1) {
    switch (c)
    {
      case 'd':
      device = optarg;
      DEBUG(printf("device: %s\n",device););
      break;
      case 'l':
      if (device != NULL) {
        lFile = optarg;
        DEBUG(printf("l: %s\n",lFile););
      }
      break;
      case 'r':
      if (device != NULL) {
        rFile = optarg;
        DEBUG(printf("r: %s\n",rFile););
      }
      break;
      case 'o':
      if (device != NULL) {
        oFile = optarg;
        DEBUG(printf("o: %s\n",oFile););
      }
      break;
      default:
      err = 1;
    }
  }
  if (!err && device) {
    if (lFile) {
      list(device,lFile);
      return 0;
    } else if (rFile && oFile) {
      recover(device, rFile,oFile);
      return 0;
    }
  }
  printUsage(argv[0]);
  return 0;
}
