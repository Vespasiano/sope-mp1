#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include "utilities.h"

struct option {
    bool verbose;
    bool cVerbose;
    bool recursive;
};

int processFile() {
    return 0;
}

int isValidMode(char *mode) {
    if (strlen(mode) < 3) {
        fprintf(stderr, "Mode string is too small.\n");
        return 0;
    }
    
    if (strlen(mode) > 5) {
        fprintf(stderr, "Mode string is too big.\n");
        return 0;
    }
    if (mode[0] != 'u' && mode[0] != 'g' && mode[0] != 'o' && mode[0] != 'a') {
        fprintf(stderr, "Character 1 in mode isn't valid.\n");
        return 0;
    }
    if (mode[1] != '-' && mode[1] != '+' && mode[1] != '=') {
        fprintf(stderr, "Character 2 in mode isn't valid.\n");
        return 0;
    }
    for (int i = 2; i < strlen(mode); i++) {
        if (mode[i] != 'r' && mode[i] != 'w' && mode[i] != 'x') {
            printf("Character %c\n", mode[i]);
            fprintf(stderr, "Character %i in mode isn't valid.\n", i);
            return 0;
        }
    }
    return 1;
}



int main(int argc, char** argv) {

    char *temp_dir = "file.txt";
    char *new_mode = NULL;
    char *dir;


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
    new_mode = argv[arg];
    arg++;
    if (!(isValidMode(new_mode))) {
        return 1;
    }
    
    /* FILE/DIR */
    while (argv[arg]) {
        dir = argv[arg];
        arg++;
        processFile();
        printf("Processed file %s\n", dir);
    }
    
    return 0;
}
