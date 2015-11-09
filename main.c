#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void printUsage(char* path) {
  printf("Usage: ");
  printf("%s",path);
  printf(" -d [device filename] [other arguments]\n-l target            List the target directory\n-r target -o dest    Recover the target pathname\n");
}
void list() {
  printf("list\n");
}
void recover(char *rFile, char *oFile) {
  printf("r: %s o: %s",rFile,oFile);
}
int main(int argc, char** argv) {
  char *device = NULL;
  int lflag = 0;
  char *rFile = NULL;
  char *oFile = NULL;
  int c;
  opterr = 0;
  while ((c = getopt (argc, argv, "d:lr:o:")) != -1) {
    switch (c)
    {
      case 'd':
      device = optarg;
      break;
      case 'l':
      if (device != NULL) {
        lflag = 1;
      }
      break;
      case 'r':
      if (device != NULL) {
        rFile = optarg;
      }
      break;
      case 'o':
      if (device != NULL) {
        oFile = optarg;
      }
      case '?':
      abort();
      default:
      abort();
    }
  }
  if (device) {
    if (lflag) {
      list();
      return 0;
    } else if (rFile && oFile) {
      recover(rFile,oFile);
      return 0;
    }
  }
  printUsage(argv[0]);
  return 0;
}
