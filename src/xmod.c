#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>


#include "utilities.h"
#include "xmod.h"


int isdir(const char *path) {
   struct stat statbuffer;
   if (stat(path, &statbuffer) != 0)
       return 0;
   return S_ISDIR(statbuffer.st_mode);
}

int processMode(const char *mode_string, mode_t *final_mode, char *path, struct stat stat_buffer) {
  
  bool rwx[3] = {0, 0, 0};
  bool ugo[3] = {0, 0, 0};
  int set_mode = -1;
  mode_t new_mode;


  /* OCTAL MODE */
  if (mode_string[0] == '0') {
      unsigned int temp_mode;
      if (sscanf(mode_string, "%o", &temp_mode) != 1) {
          exit(1);
      }
      *final_mode = (mode_t) temp_mode;
      return 0;
  }

  /* <ugoa><-+=><rwx> MODE */
  //Needs at least 1 char in <ugoa>, 1 char in <-+=> and 1 char in <rwx>
  if (strlen(mode_string) < MIN_MODE_SIZE) {
      fprintf(stderr, "Mode string is too small.\n");
      exit(1);
  }

  //Has at most 1 char in <ugoa>, 1 char in <-+=> and 3 chars in <rwx>
  if (strlen(mode_string) > MAX_MODE_SIZE) {
      fprintf(stderr, "Mode string is too big.\n");
      exit(1);
  }

  switch (mode_string[0]){
  case 'u':
      ugo[0] = true;
      break;
  case 'g':
      ugo[1] = true;
      break;
  case 'o':
      ugo[2] = true;
      break;
  case 'a':
      ugo[0] = true;
      ugo[1] = true;
      ugo[2] = true;
      break;
    default:
      fprintf(stderr, "Character 1 in mode isn't valid.\n");
      exit(1);
  }
  
  switch (mode_string[1]){
  case '-':
    set_mode = 0;
    break;
  case '+':
    set_mode = 1;
    break;
  case '=':
    set_mode = 2;
    break;
  default:
    fprintf(stderr, "Character 2 in mode isn't valid.\n");
    exit(1);  
  }

  for (int i = 2; i < strlen(mode_string); i++) {
    switch (mode_string[i]){
    case 'r':
        rwx[0] = true;
        break;
    case 'w':
        rwx[1] = true;
        break;
    case 'x':
        rwx[2] = true;
        break;
    default:
      fprintf(stderr, "Character %i in mode isn't valid.\n", i);
      exit(1);
    }
  }

  if (rwx[0]) {
    if (ugo[0])
      new_mode |= S_IRUSR;
    if (ugo[1])
      new_mode |= S_IRGRP;
    if (ugo[2])
      new_mode |= S_IROTH;
  }
  if (rwx[1]) {
    if (ugo[0])
      new_mode |= S_IWUSR;
    if (ugo[1])
      new_mode |= S_IWGRP;
    if (ugo[2])
      new_mode |= S_IWOTH;
  }
  if (rwx[2]) {
    if (ugo[0])
      new_mode |= S_IXUSR;
    if (ugo[1])
      new_mode |= S_IXGRP;
    if (ugo[2])
      new_mode |= S_IXOTH;
  }

  
  
  mode_t file_mode;
  file_mode = stat_buffer.st_mode & ~S_IFMT;

  switch (set_mode) {

  // - case
  case 0:
    *final_mode = (file_mode & ~new_mode);
    break;

  // + case
  case 1:
    *final_mode = (file_mode | new_mode);
    break;
    
  // = case
  case 2:
    if ((ugo[0]))
      *final_mode = (file_mode & ~(S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR)) | (new_mode & (S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR));
    if ((ugo[1]))
      *final_mode = (file_mode & ~(S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP)) | (new_mode & (S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP));
    if ((ugo[2]))
      *final_mode = (file_mode & ~(S_IROTH | S_IWOTH | S_IXOTH)) | (new_mode & (S_IROTH | S_IWOTH | S_IXOTH));
    break;
  }
  return 0;
}

int xmod(char *path, char *mode_string, struct stat stat_buffer, char *options_string, struct option options) {
  clock_t initial_clock = clock();




  if(isdir(path) && options.recursive){
    struct dirent *directory_entry;
    DIR *dr = opendir(path);
    if (dr == NULL) {
      fprintf(stderr, "Could not open directory in path %s", path);
    }
    while ((directory_entry = readdir(dr)) != NULL) {

    //Own directory or parent directory
      char *filename = malloc(sizeof(directory_entry->d_name) + sizeof(NULL));
      sprintf(filename, "%s", directory_entry->d_name);
      if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0)) { continue; }


      char *new_path = malloc(sizeof(path) + sizeof('/') +sizeof(directory_entry->d_name));
      sprintf(new_path, "%s%c%s", path, '/' , directory_entry->d_name);


      if(isdir(new_path)) {
        pid_t childPid;
        if ((childPid = fork()) < 0) {
          fprintf(stderr, "Error when forking process.\n");
          exit(1);
        }

        //Child process
        else if (childPid == 0) {
          execl("./xmod", "./xmod", options_string, mode_string, new_path, (char *) NULL);
          perror("execl");
        }
        
        //Father process
        else {
          int returnStatus;  
          while (wait(&returnStatus) != -1) {
            if (returnStatus == 0) {  // Child process terminated without error.  
            }
            if (returnStatus == 1) {
              fprintf(stderr, "The child process terminated with an error!.");
            }
          }
        }
      }




      
    }
       

  }
  //Path isn't a directory or recursive mode isn't set
  else {
  }




  printf("Processed file %s\n\n", path);
  // mode_t final_mode = 0;
  // processMode(mode_string, &final_mode, path, stat_buffer);
  // chmod(path, final_mode);
  return 0;
}
