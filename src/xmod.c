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

int isdir(const char *path) {
   struct stat statbuffer;
   if (stat(path, &statbuffer) != 0)
       return 0;
   return S_ISDIR(statbuffer.st_mode);
}

int checkValidMode(const char *mode_string) {
  
  /* Valid Octal Mode*/
  if (mode_string[0] == '0') {
    unsigned int temp_mode;
    if (sscanf(mode_string, "%o", &temp_mode) != 1) {
        exit(1);
    }
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

  if (mode_string[0] != 'u' & mode_string[0] != 'g' & mode_string[0] != 'o' & mode_string[0] != 'a') {
    fprintf(stderr, "Character 1 in mode isn't valid.\n");
    exit(1);
  }

  if (mode_string[1] != '-' & mode_string[1] != '+' & mode_string[1] != '=') {
  fprintf(stderr, "Character 2 in mode isn't valid.\n");
  exit(1);
  }

  for (int i = 2; i < strlen(mode_string); i++) {
    if (mode_string[i] != 'r' & mode_string[i] != 'w' & mode_string[i] != 'x') {
      fprintf(stderr, "Character %i in mode isn't valid.\n", i);
      exit(1);
    }
  }

  return 0;
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

int xmod(char *path, char *mode_string) {
  clock_t initial_clock = clock();
  struct stat stat_buffer;
  if (stat (path, &stat_buffer)) {
    fprintf(stderr,"Check path name.\n");
    exit(1);
  }

  mode_t final_mode = 0;
  processMode(mode_string, &final_mode, path, stat_buffer);
  // chmod(path, final_mode);

  fprintf(stdout, "Am currently in father directory %s\n\n", path);  
  if(isdir(path)){
    struct dirent *directory_entry;
    DIR *dr = opendir(path);
    if (dr == NULL) {
      fprintf(stderr, "Could not open directory in path %s", path);
    }
    while ((directory_entry = readdir(dr)) != NULL) {

    //Own directory or parent directory
      char *filename = malloc(sizeof(directory_entry->d_name) + sizeof(NULL));
      sprintf(filename, "%s", directory_entry->d_name);
      if ((strcmp(filename, ".") == 0) | (strcmp(filename, "..") == 0)) { continue; }


      char *new_path = malloc(sizeof(path) + sizeof('/') +sizeof(directory_entry->d_name));
      sprintf(new_path, "%s%c%s", path, '/' , directory_entry->d_name);
      printf("new_path is %s\n", new_path);
      if(isdir(new_path)) {
        pid_t childPid;
        if ((childPid = fork()) < 0) {
          fprintf(stderr, "Error when forking process.\n");
          exit(1);
        }
        //Child process
        else if (childPid == 0) {
          char *argv[] = {"./xmod", new_path, mode_string, NULL};
          setenv("LOG_FILENAME", "1", true);
          printf("calling xmod for %s\n", new_path);
          execl("./xmod", "./xmod", new_path, mode_string, NULL, NULL);
          printf("process died wtf");
        }
        //Father process
        else {
          printf("path is %s and is parent process of new path %s\n\n", path, new_path);
          int returnStatus;    
          waitpid(childPid, &returnStatus, 0);  // Parent process waits here for child to terminate.
          if (returnStatus == 0)  // Verify child process terminated without error.  
          {
          }

          if (returnStatus == 1)      
          {
            fprintf(stderr, "The child process terminated with an error!.");    
            exit(1);
          }
          // bool LOG_FILENAME = true; //testing
          //   if (LOG_FILENAME) {
          //     clock_t final_clock = clock();
          //     double instant = (double)(final_clock - initial_clock) / CLOCKS_PER_SEC;
          //     fprintf(stdout, "%f ; %d ; action ; info\n", instant, childPid);
          //   }
        }
      }
      else {
        //chmod(new_path, final_mode);
      }
    }
  }
  
  return 0;
}


// int xmod (char *path, bool ugo[3], mode_t new_mode, struct stat stat_buffer) {
//   mode_t final_mode = 0;
//   getFinalMode(path, ugo, new_mode, set_mode, stat_buffer, &final_mode);
//   // chmod(path, final_mode);
//   return 0;
// }


// int processMode(const char *mode_string, char *new_mode, char *set_mode, char* ugoa) {
  
//   bool rwx[3] = {0, 0, 0};
//   bool ugo[3] = {0, 0, 0};
//   /* OCTAL MODE */
//   if (mode_string[0] == '0') {
//       unsigned int temp_mode;
//       if (sscanf(mode_string, "%o", &temp_mode) != 1) {
//           exit(1);
//       }
//       *new_mode = temp_mode;
//       return 0;
//   }

//   /* <ugoa><-+=><rwx> MODE */
//   //Needs at least 1 char in <ugoa>, 1 char in <-+=> and 1 char in <rwx>
//   if (strlen(mode_string) < MIN_MODE_SIZE) {
//       fprintf(stderr, "Mode string is too small.\n");
//       exit(1);
//   }

//   //Has at most 1 char in <ugoa>, 1 char in <-+=> and 3 chars in <rwx>
//   if (strlen(mode_string) > MAX_MODE_SIZE) {
//       fprintf(stderr, "Mode string is too big.\n");
//       exit(1);
//   }

//   switch (mode_string[0]){
//   case 'u':
//       ugo[0] = true;
//       *ugoa = "u";
//       break;
//   case 'g':
//       ugo[1] = true;
//       *ugoa = "g";
//       break;
//   case 'o':
//       ugo[2] = true;
//       *ugoa = "o";
//       break;
//   case 'a':
//       ugo[0] = true;
//       ugo[1] = true;
//       ugo[2] = true;
//       *ugoa = "a";
//       break;
//   default:
//       fprintf(stderr, "Character 1 in mode isn't valid.\n");
//       exit(1);
//   }
 
//   if((mode_string[1] == '-') | (mode_string[1] == '+') | (mode_string[1] == '=')) {
//     *set_mode = mode_string[1];
//   }
//   else {
//     fprintf(stderr, "Character 2 in mode isn't valid.\n");
//     exit(1);
//   }

//   for (int i = 2; i < strlen(mode_string); i++) {
//     switch (mode_string[i]){
//     case 'r':
//         rwx[0] = true;
//         break;
//     case 'w':
//         rwx[1] = true;
//         break;
//     case 'x':
//         rwx[2] = true;
//         break;
//     default:
//         fprintf(stderr, "Character %i in mode isn't valid.\n", i);
//         exit(1);
//     }
//   }

//   unsigned int temp_mode = 0;
//   if (rwx[0]) {
//     if (ugo[0])
//       temp_mode |= S_IRUSR;
//     if (ugo[1])
//       temp_mode |= S_IRGRP;
//     if (ugo[2])
//       temp_mode |= S_IROTH;
//   }
//   if (rwx[1]) {
//     if (ugo[0])
//       temp_mode |= S_IWUSR;
//     if (ugo[1])
//       temp_mode |= S_IWGRP;
//     if (ugo[2])
//       temp_mode |= S_IWOTH;
//   }
//   if (rwx[2]) {
//     if (ugo[0])
//       temp_mode |= S_IXUSR;
//     if (ugo[1])
//       temp_mode |= S_IXGRP;
//     if (ugo[2])
//       temp_mode |= S_IXOTH;
//   }

//   *new_mode = temp_mode;

//   return 0;
// }

// int getFinalMode(char *path, char  *new_mode, char *set_mode, struct stat stat_buffer, mode_t *final_mode) {
//   mode_t file_mode, current_mode;
//   file_mode = stat_buffer.st_mode & ~S_IFMT;
//   current_mode = new_mode;

//   bool ugo[3] = {0, 0, 0};


//   switch (set_mode[0]) {

//   // - case
//   case '-':
//     *final_mode = (file_mode & ~current_mode);
//     break;

//   // + case
//   case '+':
//     *final_mode = (file_mode | current_mode);
//     break;
    
//   // = case
//   case '=':
//     if ((ugo[0]))
//       *final_mode = (file_mode & ~(S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR)) | (current_mode & (S_ISUID | S_IRUSR | S_IWUSR | S_IXUSR));
//     if ((ugo[1]))
//       *final_mode = (file_mode & ~(S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP)) | (current_mode & (S_ISGID | S_IRGRP | S_IWGRP | S_IXGRP));
//     if ((ugo[2]))
//       *final_mode = (file_mode & ~(S_IROTH | S_IWOTH | S_IXOTH)) | (current_mode & (S_IROTH | S_IWOTH | S_IXOTH));
//     break;
//   default:
//     exit(1);
//   }

//   return 0;
// }