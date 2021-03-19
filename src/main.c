#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>

#include "xmod.h"


struct option {
    bool verbose;
    bool cVerbose;
    bool recursive;
};


// void myfilerecursive(char *basePath)
// {
//     char path[1000];
//     struct dirent *dp;
//     DIR *dir = opendir(basePath);

   
//     if (!dir)
//         return;

//     while ((dp = readdir(dir)) != NULL)
//     {
//         if (strcmp(dp->d_name, ".") != 0 && strcmp(dp->d_name, "..") != 0)
//         {
//             printf("%s\n", dp->d_name);
//             strcpy(path, basePath);
//             strcat(path, "/");
//             strcat(path, dp->d_name);

//             myfilerecursive(path);
//         }
//     }

//     closedir(dir);
// } 






int main(int argc, char** argv) {
  
  /* [OPTIONS] */
  char opt;
  int arg = 1;
  struct option options;
  while ((opt = getopt (argc, argv, "vcR")) != -1) { 
      switch (opt) {
      case 'v':
          options.verbose = true;
          arg++;
          break;
      case 'c':
          options.cVerbose = true;
          arg++;
          break;
      case 'R':
          options.recursive = true;
          arg++;
          break;
      default:
          fprintf(stderr, "Argument in wrong format.\n");
          return 1;
      }
  }
    
    /* MODE */
    char *mode_string = NULL;
    mode_string = argv[arg];
    arg++;
    checkValidMode(mode_string);

    /* FILE/DIR */
    while (argv[arg]) {
      struct stat stat_buffer;
      char *path = argv[arg];
      if (stat (path, &stat_buffer)) {
        fprintf(stderr,"Check path name.\n");
        return 1;
      }
      mode_t file_mode = stat_buffer.st_mode & ~S_IFMT;
      //xmod();
      if (options.recursive && isdir(path, stat_buffer)) {

      }
      else {
        xmod(path, mode_string);
        fprintf(stdout, "Single file.\n");
      }
      arg++;
    }
    
    return 0;
}
