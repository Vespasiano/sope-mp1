#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "utilities.h"

int processMode(const char *mode_string, bool ugo[3], mode_t *new_mode, int *set_mode) {
  
    bool rwx[3] = {0, 0, 0};

    /* OCTAL MODE */
    if (mode_string[0] == '0') {
        unsigned int temp_mode;
        if (sscanf(mode_string, "%o", &temp_mode) != 1) {
            exit(1);
        }
        *new_mode = (mode_t) temp_mode;
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
        *set_mode = 0;
        break;
    case '+':
        *set_mode = 1;     
        break;
    case '=':
        *set_mode = 2;     
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
            *new_mode |= S_IRUSR;
          if (ugo[1])
            *new_mode |= S_IRGRP;
          if (ugo[2])
            *new_mode |= S_IROTH;
        }
    if (rwx[1]) {
          if (ugo[0])
            *new_mode |= S_IWUSR;
          if (ugo[1])
            *new_mode |= S_IWGRP;
          if (ugo[2])
            *new_mode |= S_IWOTH;
        }
    if (rwx[2]) {
          if (ugo[0])
            *new_mode |= S_IXUSR;
          if (ugo[1])
            *new_mode |= S_IXGRP;
          if (ugo[2])
            *new_mode |= S_IXOTH;
        }

    return 0;
}

int getFinalMode(char *path, bool ugo[3], mode_t new_mode, int set_mode, struct stat stat_buffer, mode_t *final_mode) {
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
  default:
    exit(1);
  }

  return 0;
}



int xmod (char *path, bool ugo[3], mode_t new_mode, int set_mode, struct stat stat_buffer) {

  return 0;
}