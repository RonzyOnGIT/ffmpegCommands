#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_DICT_LEN 140

int change_audio_to_japanese(DIR *, char *);

void destroyFfprobeArgs(char **);

// returns an array of strings from one single string, command with the input file name and output filename
char ** construct_ffmpeg_command(char[], char *, char *);

int get_word_count(char *);



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

    strcpy(finalPath, prefix);
    strcat(finalPath, dictName);

    printf("final path: %s\n", finalPath);

    DIR *dirp = opendir(finalPath);

    if (dirp == NULL) {
        perror("opendir");
        return 1;
    }

    // entry in directory
    // struct dirent *entry;

    // while ((entry = readdir(dirp)) != NULL) {
    //     printf("%s\n", entry->d_name);
    // }

    int response = -1;

    printf("change audio 1: ");
    scanf("%d", &response);

    int res = change_audio_to_japanese(dirp, finalPath);


    // while (response != 0) {

    //     printf("1: change audio to japanese\n");
    //     printf("2: make audio louder\n");

    //     scanf("%d", &response);

    //     if (response == 1) {
    //         // change audio to jpn
    //         // before I try to run the command, check first that its not already defaulted to japanese
    //         // change_audio_to_japanese()
    //         int res = change_audio_to_japanese(dirp);
    //     } else if (response == 2) {
    //         // make audio louder
    //         printf("ni");
    //     } else {
    //         printf("not a valid input\n");
    //     }


    //     printf("response: %d\n", response);

    // }

    free(finalPath);
    closedir(dirp);

    // I guess use fork() to create child process, which the child will be running execvp() then capture it from the parent

    //  To do this, first need to use fork(), then pipe() to capture the output of the child process



    //    char *args[] = {"ffmpeg", "-i", "input.mp4", "-filter:a", "volume=2.0", "output.mp4", NULL};

    

    return 0;
}

// check if jp by checking index 1 TAG: language == eng
int change_audio_to_japanese(DIR *dir, char *prefixPath) {

    // fist check that in a valid directory
    if (dir == NULL) {
        perror("opendir");
        return - 1;
    }

    // first gotta fork()

    struct dirent *entry;

    // create dynamic array

    // first store original name of the file, then change the original file name to inputfile.mkv, then use command, then change output filename to original, then delete the original file

    // int changeCount = 0;

    while ((entry = readdir(dir)) != NULL) {

        char *destination = malloc(strlen("outputfile.mkv") + strlen(prefixPath) + 2);
        strcpy(destination, prefixPath);
        strcat(destination, "/");

        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            free(destination);
            continue;
        }

        if (strcmp(entry->d_name, "subs") == 0 || strcmp(entry->d_name, "Subs") == 0 || strcmp(entry->d_name, "SUBS") == 0) {
            free(destination);
            continue;
        }


        // store the arguments to perform probe command
        // char **probeArgs = NULL;
        char **changeAudioArr = NULL;
        bool is_default_jpn = false;

        // store original file name + 1 for terminating string char + '/'
        // create_probe_arr
        // create_probe_args
        char *original_file_name = malloc(strlen(entry->d_name) + strlen(prefixPath) + 2);
        strcpy(original_file_name, prefixPath);
        strcat(original_file_name, "/");
        strcat(original_file_name, entry->d_name);

        // probeArgs = construct_ffmpeg_command("ffprobe -i "inputfile.mkv", "-show_streams", "-select_streams", "a", "-loglevel", "error"")

        // plus 5 includes one for null terminating and 4 for two '""'
        char *probe_args = malloc(strlen(original_file_name) + strlen("ffprobe -i -show_streams -select_streams a -loglevel error") + 5);

        strcpy(probe_args, "ffprobe -i");
        strcat(probe_args, " \"");
        strcat(probe_args, original_file_name);
        strcat(probe_args, "\" ");
        strcat(probe_args, "-show_streams -select_streams a -loglevel error");

        // check if mkv or mp4
        int len = strlen(original_file_name);
        char *fileExtension = original_file_name + len - 3;

        FILE *fP = popen(probe_args, "r");
        char buffer[1000];


        if (fP == NULL) {
            printf("Failed to run ffprobe command\n");
            free(probe_args);
            free(original_file_name);
            free(destination);
            return -1;
        }

        int index_is_one = false;

        while (fgets(buffer, sizeof(buffer), fP) != NULL) {

            // printf("%s\n", buffer);

            buffer[strcspn(buffer, "\n")] = 0; // Remove the newline, if present
            if (strcmp(buffer, "index=1") == 0) {
                index_is_one = true;
            }

            if (strcmp(buffer, "index=2") == 0) {
                index_is_one = false;
            }

            if ((strcmp(buffer, "TAG:language=jpn") == 0) && index_is_one) {
                // this means default language is already japanese, continue
                printf("audio is already in japanese default\n");
                is_default_jpn = true;
                break;
            }

        }
        pclose(fP);

        int fileRenameResult = -2;

        // only performs audio track change, if default audio is not japanese
        if (!is_default_jpn) {
            if (strcmp(fileExtension, "mkv") == 0) {
                strcat(destination, "outputfile.mkv");
                printf("%s IS NOT japanese by default\n", original_file_name);
                char command[200] = "ffmpeg -i -map 0:v:0 -map 0:a:m:language:jpn -map -0:s -c copy -disposition:a:0 default ";
                char **mm = construct_ffmpeg_command(command, original_file_name, destination);
                int commandCount = get_word_count("ffmpeg -i -map 0:v:0 -map 0:a:m:language:jpn -map -0:s -c copy -disposition:a:0 default ") + 2;

                for (int i = 0; i < commandCount; i++) {
                    printf("%s ", mm[i]);
                }

                printf("\n");

                pid_t pid = fork();

                if (pid == 0) {
                    execvp("ffmpeg", mm);
                    perror("execvp failed");
                } else if (pid > 0) {

                    int status;
                    waitpid(pid, &status, 0);
                    if (WIFEXITED(status)) {
                        int exit_code = WEXITSTATUS(status);
                        if (exit_code == 0) {
                            printf("successfully changed to japanese default!\n");
                            // changeCount++;
                            // delete old english default clip and rename output to original

                            int del_res = remove(original_file_name);

                            if (del_res != 0) {
                                printf("failed to delete file\n");
                            }

                            fileRenameResult = rename(destination, original_file_name);

                            // printf("original_file_name: %s\n", original_file_name);
                            // printf("destination: %s\n", destination);

                            // first delete the old english file

                            // rename the new destination one to original file
                        } else {
                            printf("some error happened\n");
                        }
                        printf("Child exited with code %d\n", WEXITSTATUS(status));
                    } else {
                        printf("Child terminated abnormally\n");
                    }
                } else {
                    perror("fork failed\n");
                }


                for (int i = 0; i < commandCount; i++) {
                    free(mm[i]);
                }

                free(mm);
                
                // fileRenameResult = rename(original_file_name, destination);
            } else if (strcmp(fileExtension, "mp4") == 0) {
                strcat(destination, "outputfile.mp4");

                // fileRenameResult = rename(original_file_name, destination);
            } else {
                strcat(destination, "outputfile.mov");
                // fileRenameResult = rename(original_file_name, destination);
            }

            if (fileRenameResult == -1) {
                printf("failed to rename file\n");
                free(probe_args);
                free(original_file_name);
                free(destination);
                return -1;
            }
        }

        free(probe_args);
        free(original_file_name);
        free(destination);

    }

    return 1;

    // first check to see if not already in japanese, if so then move on

    //    execvp("ffmpeg", args);


}


void destroyFfprobeArgs(char **args) {


    for (int i = 0; i < 9; i++) {
        free(args[i]);
    }

    free(args);

}

// have to tokenize the char *command input to be able to get individual arguments
// returns an array of strings from one single string, command, with the input file name
char ** construct_ffmpeg_command(char command[], char *input_file_name, char *output_file) {
    // char **command = malloc()
    // first we need to tokenize this shit so use strtoken??
    int count = get_word_count(command);

    // plus 2 for input_file_name and output_file and plus extra one for NULL argument
    int fullCommandWordCount = count + 3;
    char **new_command = malloc(fullCommandWordCount * sizeof(char *));

    char *tok = strtok(command, " ");
    int tokenCount = 0;

    while (tok != NULL) {
        // if tokenCount == 2, then want to increment tokenCount to not occupy the 3rd element in the command array
        if (tokenCount == 2) {
            tokenCount++;
        }
        int len = strlen(tok);
        new_command[tokenCount] = malloc(len + 1);
        strcpy(new_command[tokenCount], tok);
        tok = strtok(NULL, " ");
        tokenCount++;
    }

    // after creating array of arguments, append the output file and input file
    new_command[2] = malloc(strlen(input_file_name) + 1);
    strcpy(new_command[2], input_file_name);
    new_command[fullCommandWordCount - 2] = malloc(strlen(output_file) + 1);
    strcpy(new_command[fullCommandWordCount - 2], output_file);
    new_command[fullCommandWordCount - 1] = NULL;


    // for (int i = 0; i < fullCommandWordCount; i++) {
    //     printf("%s ", new_command[i]);
    // }
    // printf("\n");

    // store enough for commands plus 2 for input and output file locations

    return new_command;

}

int get_word_count(char *word) {

    int count = 0;

    for (int i = 0; i < strlen(word); i++) {
        if (word[i] == ' ') {
            count++;
        }
    }

    return count;
}


