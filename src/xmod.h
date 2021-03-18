#ifndef xmod_H_
#define xmod_H_

int isdir(const char *path, struct stat statbuffer);

int checkValidMode(const char *mode_string);

int processMode(const char *mode_string, mode_t *final_mode, char *path, struct stat stat_buffer);

// int getFinalMode(char *path, char  *new_mode, char *set_mode, struct stat stat_buffer, mode_t *final_mode);

int xmod(char *path, char *mode_string);



#endif