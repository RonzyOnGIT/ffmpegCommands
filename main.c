#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#define MAX_DICT_LEN 140


int main(void) {


    // to be able to dynamically make an array of strings, have to allocate on the heap


    // first get the name of the directory to get access to all files to be able to apply changes to all files
    char dictName[MAX_DICT_LEN];
    char prefix[7] = "../../";
        
    printf("enter folder name: \n");
    scanf("%[^\n]", dictName);

    int totalLen = strlen(dictName) + strlen(prefix) + 1;

    // the sizeof(char) can be ommitted since by default malloc allocates in bytes so redundant
    char *finalPath = malloc(totalLen);

    strcat(finalPath, prefix);
    strcat(finalPath, dictName);

    printf("final path: %s\n", finalPath);

    DIR *dirp = opendir(finalPath);

    if (dirp == NULL) {
        perror("opendir");
        return 1;
    }

    // entry in directory
    struct dirent *entry;

    while ((entry = readdir(dirp)) != NULL) {
        printf("%s\n", entry->d_name);
    }


    free(finalPath);
    closedir(dirp);

    int response = -1;

    while (response != 0) {

        printf("1: change audio to japanese\n");
        printf("2: make audio louder\n");

        scanf("%d", &response);

        if (response == 1) {
            // change audio to jpn
        } else if (response == 2) {
            // make audio louder
        } else {
            printf("not a valid input\n");
        }


        printf("response: %d\n", response);

    }

    //    char *args[] = {"ffmpeg", "-i", "input.mp4", "-filter:a", "volume=2.0", "output.mp4", NULL};
    //    execvp("ffmpeg", args);

    

    return 0;
}


