#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <ctype.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include "utilities.h"
#include "xmod.h"


int nftot = 0;
int nfmod = 0;
char* this_path;
char* event;
bool verbose = false;


int isdir(const char *path) {
   struct stat statbuffer;
   if (stat(path, &statbuffer) != 0) { return 0; }
   return S_ISDIR(statbuffer.st_mode);
}

unsigned long calculateInstant() {
  char *initial_time_string_ms = getenv("PROGRAM_TIME_SINCE_EPOCH");
  unsigned long initial_time_ms = strtoul(initial_time_string_ms, NULL, 0);
  struct timeval final_time_struct;
  gettimeofday(&final_time_struct, NULL);
  unsigned long final_time_ms = final_time_struct.tv_sec * SECONDS_TO_MILLISECONDS + final_time_struct.tv_usec * MICROSECONDS_TO_MILLISECONDS;
  unsigned long instant = (final_time_ms - initial_time_ms);
  return instant;
}

void registerReceivedSignal(char* c){
  FILE *fptr = NULL;
  if ((fptr = fopen(getenv("LOG_FILENAME"), "a")) == NULL) {
    printf("Error opening file\n");
    exit(1);
  }
  unsigned int instant = calculateInstant();
  event = "SIGNAL_RECV";
  char *info = malloc(100 * sizeof(char));

  sprintf(info, "SIG%s", c);
  fprintf(fptr, "%u ; %d ; %s ; %s\n", instant, getpid(), event, info);
  fclose(fptr);
}

void registerSentSignal(int signo, pid_t p){
  FILE *fptr = NULL;
  if ((fptr = fopen(getenv("LOG_FILENAME"),"a")) == NULL){
    printf("Error opening file!");
    exit(1);
  };

  unsigned int instant = calculateInstant();
  event = "SIGNAL_SENT";
  char *info = malloc(100 * sizeof(char));

  sprintf(info, "SIG%i : %i (group)", signo, p);
  fprintf(fptr, "%u ; %d ; %s ; %s\n", instant, getpid(), event, info);
  fclose(fptr);
}

void sigINTHandler(int signo){
  char *s = "INT";
  registerReceivedSignal(s);
  pid_t pid = getpid();
  pid_t original_father_pid = (pid_t) atoi(getenv("PROGRAM_PID"));
  printf("\n%d ; %s ; %d ; %d\n", pid, this_path, nftot, nfmod);
  if (pid == original_father_pid) {
    puts("\nTerminate(t) or proceed(any key)?");
    char strvar;
    strvar = getchar();
    if (strvar == 't') { exit(0); }
    else { 
      puts("\nThe program will continue normally\n");
      pid_t group_pid = getpgid(pid);
      registerSentSignal(SIGUSR1, group_pid);
      killpg(group_pid, SIGUSR1);
      }

  }
  else { pause(); }
}

void sigUSR1Handler(int signo) {
  char *s = "USR1";
  registerReceivedSignal(s);
  return;
}

void procExitHandler(int signo) {
  char *s = "CHLD";
  registerReceivedSignal(s);
  int wstat;
  pid_t	child_pid;
  FILE *fptr = NULL;
  if ((fptr = fopen(getenv("LOG_FILENAME"), "a")) == NULL) {
    printf("Error opening file\n");
    exit(1);
  }
  if ((child_pid = wait3(&wstat, WNOHANG, (struct rusage *) NULL)) != -1) {
    if (verbose) {
      printf("Killed child process with id %i\n", (int) child_pid);
    }
    
    char* event = "PROC_EXIT";
    char *info = malloc(10 * sizeof(char));
    sprintf(info, "%d", WEXITSTATUS(wstat));
    unsigned long instant = calculateInstant();
    fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, child_pid, event, info);
  }
  fclose(fptr);
  return;
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


  if (file_mode != *final_mode) {
      unsigned int instant = calculateInstant();
      event = "FILE_MODF";
      char *info = malloc(100*sizeof(char));
      sprintf(info, "%s : %u : %u", path, (unsigned int) file_mode, (unsigned int)(*final_mode));
      FILE *fptr = NULL;
      char *log_filename = getenv("LOG_FILENAME");
      if ((fptr = fopen(log_filename, "a")) == NULL) {
        printf("Error opening file\n");
        exit(1);
      }
      fprintf(fptr, "%u ; %d ; %s ; %s\n", instant, getpid(), event, info);
      fclose(fptr);
    }
  nfmod++;
  
  return 0;
}

int xmod(char *path, char *mode_string, struct stat stat_buffer, char *options_string, struct option options) {
  verbose = options.verbose;

  //Signals that will be handled
  signal(SIGINT, sigINTHandler);
  signal(SIGCHLD, procExitHandler);
  signal(SIGUSR1, sigUSR1Handler);

  nftot++;
  this_path = path;

  if(isdir(path) && options.recursive){

    struct dirent *directory_entry;
    DIR *dr = opendir(path);
    if (options.verbose) {
      printf("Opened directory with path %s\n", path);
    }
    
    if (dr == NULL) { fprintf(stderr, "Could not open directory in path %s\n", path); }
    //traverses directory
    while ((directory_entry = readdir(dr)) != NULL) {

      //Prevents from traversing own directory or parent directory
      char *filename = malloc(sizeof(directory_entry->d_name) + sizeof(NULL));
      sprintf(filename, "%s", directory_entry->d_name);
      if ((strcmp(filename, ".") == 0) || (strcmp(filename, "..") == 0)) { continue; }

      //Creates new path according to directory entry's file name
      char *new_path = malloc(sizeof(path) + sizeof('/') +sizeof(directory_entry->d_name));
      sprintf(new_path, "%s%c%s", path, '/' , directory_entry->d_name);
      
      //If new path is a directory, creates child process.
      if(isdir(new_path)) {
        pid_t childPid;
        
        if ((childPid = fork()) < 0) {
          perror("fork()");
          exit(1);
        }

        //Child process
        else if (childPid == 0) {
          unsigned long instant = calculateInstant();
          char* event = "PROC_CREAT";
          char *info = malloc(sizeof(options_string) + sizeof(' ') + sizeof(mode_string) + sizeof(' ') +sizeof(new_path));
          sprintf(info, "%s %s %s", options_string, mode_string, new_path);
          FILE *fptr = NULL;
            if ((fptr = fopen(getenv("LOG_FILENAME"), "a")) == NULL) {
              printf("Error opening file\n");
              exit(1);
            }
          fprintf(fptr, "%lu ; %d ; %s ; %s\n", instant, getpid(), event, info);
          fclose(fptr);
          if (options.verbose) {
            printf("Calling xmod function with arguments %s %s %s\n", options_string, mode_string, new_path);
          }
          execl("./xmod", "./xmod",options_string, mode_string, new_path, NULL );
          perror("execl");
        }
        
        //Father process, waits for all child processes to finish
        else {
          if (options.verbose) {
            printf("From parent process %i created child process %i\n", (int) getpid(), childPid);
          }
          pause();

        }
      }
    }
    closedir(dr);
    if (options.verbose) {
      printf("Closed directory %s\n", path);
    }
  }
  mode_t final_mode = 0;
  processMode(mode_string, &final_mode, path, stat_buffer);
  char final_mode_string[10];
  sprintf(final_mode_string, "%o", (unsigned int)final_mode);

  if (options.verbose || options.cVerbose) {
    printf("Changed mode of file %s to 0%s\n", path, final_mode_string);
  }

  chmod(path, final_mode);

  return 0;
}



