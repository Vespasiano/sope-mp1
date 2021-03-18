#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "utilities.h"

void inthandler(int signo);
void childhandler(int signo);
int processMode(const char *mode_string, bool ugo[3], mode_t *new_mode, int *set_mode);
int getFinalMode(char *path, bool ugo[3], mode_t new_mode, int set_mode, struct stat stat_buffer, mode_t *final_mode);
int recursiveSearch(char *path, bool ugo[3], mode_t new_mode, int set_mode, struct stat stat_buffer);
int nftot = 0;
int nfmod = 0;
char* path;

int main(int argc, char** argv) {

    char *temp_dir = "file.txt";
    char *dir;
    signal(SIGCHLD, childhandler);
    signal(SIGINT, inthandler);

    // if (argc < 4) {
    //     fprintf(stderr, "Not enough arguments.\n");
    //     return 1;
    // }
    
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
    mode_t new_mode = 0;
    int set_mode = -1;
    bool ugo[3] = {0, 0, 0};
    mode_string = argv[arg];
    arg++;    
    processMode(mode_string, ugo, &new_mode, &set_mode);

    /* FILE/DIR */
    while (argv[arg]) {
      struct stat stat_buffer;
      path = argv[arg];
      if (stat (path, &stat_buffer)) {
        fprintf(stderr,"Check path name.\n");
        return 1;
      }
      else nftot++;
      sleep(10);

      bool is_dir = stat_buffer.st_mode & S_IFDIR;
      mode_t file_mode = stat_buffer.st_mode & ~S_IFMT;
      if (options.recursive && is_dir) {
        recursiveSearch(path, ugo, new_mode, set_mode, stat_buffer);
      }
      else {
        mode_t final_mode = 0;
        getFinalMode(path, ugo, new_mode, set_mode, stat_buffer, &final_mode);
        sleep(10);
         chmod(path, final_mode);
      }
      arg++;
    }
    
    return 0;
}

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

  if (file_mode != *final_mode) nfmod++;

  return 0;
}

int recursiveSearch(char *path, bool ugo[3], mode_t new_mode, int set_mode, struct stat stat_buffer) {
  return 0;
}


void inthandler(int signo){
    pid_t pid;
    int n;
    pid = getpid();
    char filedir[100] = "bl";
    printf("\n%d ; %s ; %d ; %d\n", pid, path, nftot, nfmod);
    puts("\n\nTerminate(t) or proceed(any key)?");
    char strvar[1];
    sleep(3);
    fgets (strvar, 5, stdin);
    if (strvar[0] == 't')
      exit(0);
    else
      puts("\n\nThe program will continue normally");
}

void childhandler(int signo){
    pid_t pid;
    int status;
    pid = wait(&status);
    printf("Child with PID %d exited with status 0x%x.\n", pid, status);
}


  /* USEFUL CODE */



  //     #ifndef __MINGW32__
  //   bool is_dir;
  // #endif
  //   mode_t mode_mask, file_mode, new_mode;
  //   struct stat stat_buf;

  //   /* Read the current file mode. */


/*
pid_t pids[10];
int i;
int n = 10;

// Start children.
for (i = 0; i < n; ++i) {
  if ((pids[i] = fork()) < 0) {
    perror("fork");
    abort();
  } else if (pids[i] == 0) {
    DoWorkInChild();
    exit(0);
  }
}

// Wait for children to exit.
int status;
pid_t pid;
while (n > 0) {
  pid = wait(&status);
  printf("Child with PID %ld exited with status 0x%x.\n", (long)pid, status);
  --n;  // TODO(pts): Remove pid from the pids array.
}
*/