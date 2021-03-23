#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <dirent.h>
#include <unistd.h>

#include "utilities.h"
#include "xmod.h"



int main(int argc, char** argv) {

  //Creates environment variable with time since epoch to calculate instant
  struct timeval initial_time_struct;
  gettimeofday(&initial_time_struct, NULL);
  unsigned long initial_time = initial_time_struct.tv_sec * SECONDS_TO_MILLISECONDS + initial_time_struct.tv_usec * MICROSECONDS_TO_MILLISECONDS;
  char *initial_time_string_ms = malloc(sizeof(initial_time));
  sprintf(initial_time_string_ms, "%lu", initial_time);
  setenv("PROGRAM_TIME_SINCE_EPOCH", initial_time_string_ms, 0);

  //Sets environment variable "PROGRAM_PID" to original process's id
  pid_t program_pid = getpid();
  char program_pid_string[100];
  sprintf(program_pid_string, "%i", (int) program_pid);
  setenv("PROGRAM_PID", program_pid_string, 0);

  //Opens 
  if (program_pid == atoi(getenv("PROGRAM_PID"))) {
    char* log_filename = getenv("LOG_FILENAME");
    FILE *fptr = fopen(log_filename, "w");
    fclose(fptr);
  }
  
  /* [OPTIONS] */
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
