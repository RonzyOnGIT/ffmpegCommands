#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/types.h>

#define MAX_DIR_LEN 256

int change_audio_to_japanese(DIR *, char *);

int amplify_audio(DIR *, char *);

void destroyFfprobeArgs(char **);

// returns an array of strings from one single string, command with the input file name and output filename
char ** construct_ffmpeg_command(char[], char *, char *);

// returns -1 if failed, returns 0 otherwise
int perform_command(char *, char **);

int get_word_count(char *);

// BUGS: 
// When u type in a decimal or something, it starts going in an infinite loop or something, so reset values or something

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

    printf("final path: %s\n", finalPath);

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

// prefixPath is the '../../'
// check if jp by checking index 1 TAG: language == eng
int change_audio_to_japanese(DIR *dir, char *prefixPath) {

    // fist check that in a valid directory
    if (dir == NULL) {
        perror("opendir");
        return - 1;
    }

    struct dirent *entry;

    // first store original name of the file, then change the original file name to inputfile.mkv, then use command, then change output filename to original, then delete the original file

    while ((entry = readdir(dir)) != NULL) {
    

        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(entry->d_name, "subs") == 0 || strcmp(entry->d_name, "Subs") == 0 || strcmp(entry->d_name, "SUBS") == 0) {
            continue;
        }

        // ignore subs directory
        if (strcmp(entry->d_name, "subs") == 0) {
            continue;
        }

        char *destination = malloc(strlen("outputfile.mkv") + strlen(prefixPath) + 2);
        strcpy(destination, prefixPath);
        strcat(destination, "/");

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
        char buffer[1000]; // buffer for each file


        if (fP == NULL) {
            printf("Failed to run ffprobe command\n");
            free(probe_args);
            free(original_file_name);
            free(destination);
            return -1;
        }

        int index_is_one = false;
        int japanese_stream_index = -1; // for some files, they are not tagged the same, so manually keep track of japanese audio track
        int curr_idx = -1;

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
                
            } else if (strcmp(fileExtension, "mp4") == 0) {

                strcat(destination, "outputfile.mp4");

            } else {

                strcat(destination, "outputfile.mov");
            
            }

            printf("%s IS NOT japanese by default\n", original_file_name);

            char command[200] = "ffmpeg -i -map 0:v:0 -map 0:a:m:language:jpn -map -0:s -c copy -disposition:a:0 default ";
            char **ffmpeg_command = construct_ffmpeg_command(command, original_file_name, destination);
            int commandCount = get_word_count("ffmpeg -i -map 0:v:0 -map 0:a:m:language:jpn -map -0:s -c copy -disposition:a:0 default ") + 2;

            for (int i = 0; i < commandCount; i++) {
                printf("%s ", ffmpeg_command[i]);
            }

            printf("\n");

            int res = perform_command("ffmpeg", ffmpeg_command);

            if (res == 1) {

                printf("Successfully changed to japanese track!\n");

                // audio changed, now delete the original file and rename the new file with original name
                int del_res = remove(original_file_name);
                if (del_res < 0) {

                    printf("failed to delete file\n");

                    for (int i = 0; i < commandCount; i++) {
                        free(ffmpeg_command[i]);
                    }
                
                    free(ffmpeg_command);
                    return -1;
                }

                int file_rename_res = rename(destination, original_file_name);

                if (file_rename_res < 0) {

                    for (int i = 0; i < commandCount; i++) {
                        free(ffmpeg_command[i]);
                    }
                
                    free(ffmpeg_command);
                    perror("rename");
                    return -1;
                }
            } else {
                return -1;
            }

            for (int i = 0; i < commandCount; i++) {
                free(ffmpeg_command[i]);
            }

            free(ffmpeg_command);

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

}

int amplify_audio(DIR *dir, char *prefixPath) {
    // command -> ffmpeg -i input.mp4 -filter:a "volume=1.5" -c:v copy output.mp4

    if (dir == NULL) {
        perror("opendir");
        return - 1;
    }

    float amplify_value = 0;

    // value over 1 to amplify or less to decrease volume
    printf("how much to amplify: ");
    scanf("%f", &amplify_value);

    struct dirent *entry; // this is one file from the directory

    while ((entry = readdir(dir)) != NULL) {

        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        if (strcmp(entry->d_name, "subs") == 0 || strcmp(entry->d_name, "Subs") == 0 || strcmp(entry->d_name, "SUBS") == 0) {
            continue;
        }

        // skip subs folder
        if (strcmp(entry->d_name, "subs") == 0) {
            continue;
        }

        //                                                      plus 2 for '/' and '\0'
        char *destination = malloc(strlen("outputfile.mkv") + strlen(prefixPath) + 2);
        strcpy(destination, prefixPath);
        strcat(destination, "/");


        char *original_file_name = malloc(strlen(entry->d_name) + strlen(prefixPath) + 2);
        strcpy(original_file_name, prefixPath);
        strcat(original_file_name, "/");
        strcat(original_file_name, entry->d_name);

        int len = strlen(original_file_name);
        char *file_extension = original_file_name + len - 3;

        // check to see what exension it is to rename it appropriately
        if (strcmp(file_extension, "mkv") == 0) {
            strcat(destination, "outputfile.mkv");
        } else if (strcmp(file_extension, "mp4") == 0) {
            strcat(destination, "outputfile.mp4");
        } else if (strcmp(file_extension, "mov") == 0) {
            strcat(destination, "outputfile.mov");
        }

        char command[200] = "ffmpeg -i -filter:a volume=1.5 -c:v copy ";

        // this is the acutal command thats gonna get executed
        char **amp_command = construct_ffmpeg_command(command, original_file_name, destination);
        
        int commandCount = get_word_count("ffmpeg -i -filter:a volume=1.5 -c:v copy ") + 2;
        
        for (int i = 0; i < commandCount; i++) {
            printf("%s ", amp_command[i]);
        }

        int res = perform_command("ffmpeg", amp_command);

        if (res == 1) {

            printf("successfully amplified audio!\n");

            // delete the old video file
            int del_res = remove(original_file_name);

            if (del_res < 0) {
                printf("failed to delete file %s\n", original_file_name);
            }

            int fileRenameResult = rename(destination, original_file_name);
            
            if (fileRenameResult < 0) {
                printf("file rename failed\n");
                return -1;
            }

        } else {
            return -1;
        }

        for (int i = 0; i < commandCount; i++) {
            free(amp_command[i]);
        }

        free(amp_command);

        free(destination);
        free(original_file_name);

        printf("\n");

    }

    return 1;

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

    return new_command;

}

int perform_command(char *command, char **command_args) {

    if (command_args == NULL) {
        printf("invalid command args\n");
        return -1;
    }

    // create child process
    pid_t pid = fork();

    if (pid == 0) {
        // child process

        // perform command
        int res = execvp(command, command_args);

        if (res < 0) {
            perror("failed to exec");
            _exit(1);
        }

    } else if (pid > 0) {
        // parent process

        int status;

        waitpid(pid, &status, 0);

        if (WIFEXITED(status)) {

            // child process exitted succesfully
            if (WEXITSTATUS(status) == 0) {
                return 1;
            } else {
                printf("something went wrong with child process\n");
                return -1;
            }

        } else {
            printf("Child terminated abnormally\n");
            return -1;
        }

    } else {
        perror("fork failed");
        return -1;
    }

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


