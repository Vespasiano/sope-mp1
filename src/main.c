#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "xmod.h"




int main(int argc, char** argv) {
  char opt;
  extern int optind;
  char *options_string = malloc(200 * sizeof(char));
  struct option options;
  strcat(options_string, "-");
  while ((opt = getopt (argc, argv, "vcR")) != -1) {
    switch (opt) {
      case 'v':
        options.verbose = true;
        strcat(options_string, "v");
        break;
      case 'c':
        options.cVerbose = true;
        strcat(options_string, "c");
        break;
      case 'R':
        options.recursive = true;
        strcat(options_string, "R");
        break;
      default:
        fprintf(stderr, "Argument in wrong format.\n");
        exit(1);
    }
  }
  
  
  /* MODE */
  char *mode_string = NULL;
  mode_string = argv[optind];
  optind++;


  /* FILE/DIR */
  while (argv[optind]) {
    struct stat stat_buffer;
    char *path = argv[optind];
    if (stat (path, &stat_buffer)) {
      fprintf(stderr,"Check path name.\n");
      return 1;
    }
    xmod(path, mode_string, stat_buffer, options_string, options);
    optind++;
  }
  
  return 0;
}

