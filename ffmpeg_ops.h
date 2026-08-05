#ifndef FFMPEG_OPS_H
#define FFMPEG_OPS_H

#include <dirent.h>

int change_audio_to_japanese(DIR *, char *);

int amplify_audio(DIR *, char *);

// returns an array of strings from one single string, command with the input file name and output file name
char **construct_ffmpeg_command(char[], char *, char *);

// returns -1 if failed, returns 0 otherwise
int perform_command(char *, char **);

int get_word_count(char *);


#endif 