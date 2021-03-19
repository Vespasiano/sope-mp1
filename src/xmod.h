#ifndef xmod_H_
#define xmod_H_

struct option {
    bool verbose;
    bool cVerbose;
    bool recursive;
};

void upcase(char *s);

int isdir(const char *path);

unsigned long calculateInstant();

void registerSignal(int signo);

void sigINTHandler(int signo);

void sigUSR1Handler(int signo);

void procExitHandler(int signo);

int processMode(const char *mode_string, mode_t *final_mode, char *path, struct stat stat_buffer);

int xmod(char *path, char *mode_string, struct stat stat_buffer, char *option_string, struct option options);



#endif