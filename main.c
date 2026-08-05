#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include "ffmpeg_ops.h"

#define MAX_DIR_LEN 521

int main(void) {

    // first get the name of the directory to get access to all files to be able to apply changes to all files
    char dirName[MAX_DIR_LEN];
    char prefix[7] = "../../";

    printf("enter folder name: \n");
    scanf("%[^\n]", dirName);

    int totalLen = strlen(dirName) + strlen(prefix) + 1;

    // the sizeof(char) can be ommitted since by default malloc allocates in bytes so redundant
    char *finalPath = malloc(totalLen);

    strcpy(finalPath, prefix);
    strcat(finalPath, dirName);

    printf("opening: %s\n\n", finalPath);

    DIR *dirp = opendir(finalPath);

    if (dirp == NULL) {
        perror("opendir");
        return 1;
    }

    int response = -1;

    while (response != 0) {

        printf("1: change audio to japanese\n");
        printf("2: make audio louder\n");
        printf("0: quit\n");

        scanf("%d", &response);

        if (response == 1) {
            // reset directory pointer to point back to beginning if not already
            rewinddir(dirp);
            int res = change_audio_to_japanese(dirp, finalPath);
        } else if (response == 2) {
            rewinddir(dirp);
            int res = amplify_audio(dirp, finalPath);
        } else if (response == 0) {
            printf("exiting application\n");
            free(finalPath);
            closedir(dirp);
            return 0;
        } else {
            printf("not a valid input, try again\n");
        }

        printf("response: %d\n", response);

    }

    free(finalPath);
    closedir(dirp);

    return 0;
}
