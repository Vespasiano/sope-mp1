#ifndef xmod_H_
#define xmod_H_

struct option {
    bool verbose;
    bool cVerbose;
    bool recursive;
};

int isdir(const char *path);

int processMode(const char *mode_string, mode_t *final_mode, char *path, struct stat stat_buffer);

int xmod(char *path, char *mode_string, struct stat stat_buffer, char *option_string, struct option options);



#endif