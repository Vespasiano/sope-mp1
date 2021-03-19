#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <ctype.h>
#include <unistd.h>

#include "utilities.h"

void inthandler(int signo);
int nftot = 0;
int nfmod = 0;
char* this_path;
char* envVar;
unsigned long initial_time_ms;

void upcase(char *s)
{
    while (*s)
    {
        *s = toupper(*s);
        s++;        
    }
}

void registerSignal(int signo){
  if(envVar){
    FILE *fptr = NULL;
    if ((fptr = fopen(envVar,"a")) == NULL){
      printf("Error opening LOG_FILENAME file!");
      exit(1);
    };

    struct timeval final_time_struct;
    gettimeofday(&final_time_struct, NULL);
    unsigned long final_time_ms = final_time_struct.tv_sec * 1000 + final_time_struct.tv_usec * 0.001;
    unsigned long instant = (final_time_ms - initial_time_ms);
    
    char* event = "SIGNAL_RECV";
    char *info;
    char *str = strdup(sys_signame[signo+1]);
    upcase(str);
    sprintf(info, "SIG%s", str);
    //sprintf(info, "%d", WEXITSTATUS(returnStatus));

    fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, getpid(), event, info);
    fclose(fptr);
  }
}

void inthandler(int signo){
  pid_t pid;
  pid = getpid();


    printf("\n%d ; %s ; %d ; %d\n", pid, this_path, nftot, nfmod);
    printf("\nProcess %d : Terminate(t) or proceed(any key)?\n", pid);
    char strvar;
    //sleep(3);
    strvar = getchar();
    if (strvar == 't'){
      if(envVar){
        FILE *fptr = NULL;
        if ((fptr = fopen(envVar,"a")) == NULL){
          printf("Error opening LOG_FILENAME file!");
          exit(1);
        };

        struct timeval final_time_struct;
        gettimeofday(&final_time_struct, NULL);
        unsigned long final_time_ms = final_time_struct.tv_sec * 1000 + final_time_struct.tv_usec * 0.001;
        unsigned long instant = (final_time_ms - initial_time_ms);
        
        char* event = "SIGNAL_SENT";
        char *info; 
        sprintf(info,"SIGQUIT : %d", getppid());
        //sprintf(info, "%d", WEXITSTATUS(returnStatus));

        fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, pid, event, info);
        fclose(fptr);
      }
      kill(getppid(),SIGQUIT);
      exit(0);
    }
    else{
      puts("\nThe program will continue normally\n");
    }
      
}


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
  mode_t new_mode = 0;


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

  if (file_mode != *final_mode) nfmod++;

  return 0;
}

int xmod(char *path, char *mode_string, struct stat stat_buffer, char *options_string, struct option options) {
  
  envVar = getenv("LOG_FILENAME");
  char *initial_time_string_ms = getenv("PROGRAM_TIME_SINCE_EPOCH");
  initial_time_ms = strtoul(initial_time_string_ms, NULL, 0);


  signal(SIGINT, inthandler);

  //int st = setenv("LOG_FILENAME", argv[1], 1);

    if(envVar)
        printf("Var found: %s", envVar);
    else
        printf("Var not found.");

    

  if (stat (path, &stat_buffer)) {
    fprintf(stderr,"Check path name.\n");
    exit(1);
  }
  else nftot++;
  this_path = path;

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
      if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0)) { continue; }


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

          if(envVar){
            FILE *fptr = NULL;
            if ((fptr = fopen(envVar,"a")) == NULL){
              printf("Error opening LOG_FILENAME file!");
              exit(1);
            };




              struct timeval final_time_struct;
              gettimeofday(&final_time_struct, NULL);
              unsigned long final_time_ms = final_time_struct.tv_sec * 1000 + final_time_struct.tv_usec * 0.001;
              unsigned long instant = (final_time_ms - initial_time_ms);

            
            
            char* event = "PROC_CREAT";
            char *info = malloc(sizeof(mode_string) + sizeof(' ') +sizeof(this_path));
            sprintf(info, "%s%c%s", mode_string, ' ' , this_path);

            fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, getpid(), event, info);
            fclose(fptr);
          }

          char *argv[] = {"./xmod", new_path, mode_string, NULL};
          setenv("LOG_FILENAME", "1", true);
          printf("calling xmod for %s\n", new_path);
          execl("./xmod", "./xmod",mode_string, new_path,NULL);
          perror("execl");
        }
        
        //Father process
        else {

          //Waits for all child processes to finish
          int returnStatus;  
          while (wait(&returnStatus) != -1) {
            if (returnStatus == 0) {  // Child process terminated without error.
            }
            if (returnStatus == 1) {
              fprintf(stderr, "The child process terminated with an error!.");
            }
            if(envVar){
            
              FILE *fptr = NULL;
              if ((fptr = fopen(envVar,"a")) == NULL){
                printf("Error opening LOG_FILENAME file!");
                exit(1);
              };

              struct timeval final_time_struct;
              gettimeofday(&final_time_struct, NULL);
              unsigned long final_time_ms = final_time_struct.tv_sec * 1000 + final_time_struct.tv_usec * 0.001;
              unsigned long instant = (final_time_ms - initial_time_ms);
              
              
              char* event = "PROC_EXIT";
              char *info = malloc(sizeof(mode_string) + sizeof(' ') +sizeof(this_path));
              sprintf(info, "%d", WEXITSTATUS(returnStatus));



              fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, childPid, event, info);
              fclose(fptr);
            }
          }
        }
      }
    }
  }
  
  int returnStatus;  
  while (wait(&returnStatus) != -1) {
    if(envVar){
        FILE *fptr = NULL;
        if ((fptr = fopen(envVar,"w")) == NULL){
          printf("Error opening LOG_FILENAME file!");
          exit(1);
        };

        clock_t final_clock = clock();
        double instant = (double)(final_clock - initial_clock) / CLOCKS_PER_SEC;
        char* event = "PROC_EXIT";
        char *info = malloc(sizeof(mode_string) + sizeof(' ') +sizeof(this_path));
        sprintf(info, "%d", WIFEXITSTATUS(returnStatus));

        fprintf(fptr, "%f ; %d ; %s ; %d\n", instant, getpid(), event, info);
        fclose(fptr);
    }
    if (returnStatus == 0) {  // Child process terminated without error.
    }
    if (returnStatus == 1) {
      fprintf(stderr, "The child process terminated with an error!.");
    }
  }

  mode_t final_mode = 0;
  processMode(mode_string, &final_mode, path, stat_buffer);
  printf("Processed file %s\n\n", path);
  sleep(1);
  chmod(path, final_mode);

  
  return 0;
}


void inthandler(int signo){
    pid_t pid;
    pid = getpid();
    printf("\n%d ; %s ; %d ; %d\n", pid, this_path, nftot, nfmod);
    puts("\n\nTerminate(t) or proceed(any key)?");
    char strvar;
    //sleep(3);
    strvar = getchar();
    if (strvar == 't')
      exit(0);
    else
      puts("\n\nThe program will continue normally");

}
