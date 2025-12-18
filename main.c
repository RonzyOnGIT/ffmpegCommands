#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <sys/types.h>
#include <pthread.h>

#define MAX_DIR_LEN 521
#define MAX_COMMAND_LEN 1024
#define MAX_WORKERS 3

// #define DEBUG 1

int change_audio_to_japanese(DIR *, char *);

int amplify_audio(DIR *, char *);

void destroyFfprobeArgs(char **);

// returns an array of strings from one single string, command with the input file name and output filename
char ** construct_ffmpeg_command(char[], char *, char *);

// returns -1 if failed, returns 0 otherwise
int perform_command(char *, char **);

int get_word_count(char *);


typedef struct files_queue {
    char **files; // array of all files
    char *prefix_path; // files prepended with number for unique rename
    size_t size; // current size of queue
    size_t head; // index of head to process
    size_t capacity; // max size
    float amplify_value;
} queue;



/**
 * @brief initializes job queue.
 *
 * This function takes the size of files to process and initializes . 
 *
 * @param file_count amount of files to process (integer).
 * @param prefix_path store the prefix (../../dir) path to rename (char *)
 * @param amplify_value keep track of amplify value so worker threads know how much, -1 to ignore (float)
 * @return files_queue 
 */
struct files_queue * initialize_queue(int file_count, char *prefix_path, float amplify_value);


void destroy_queue(struct files_queue *q);


/**
 * @brief adds job to queue
 *
 * add to the queue a new file path appending it either to the tail or to the head
 *
 * @param files_queue queue to add job the new job to (queue *)
 * @param file_path file path of the new job (string).
 * @param prefix_path prefix_path prepended to file_path "../../ "(string).
 * @return -1 on fail and 0 for success
 */
int add_job(queue *files_queue, char *file_path, char *prefix_path);

/**
 * @brief routine to process a file
 *
 * gets next file to process from queue and performs amplification command
 *
 * @param files_queue queue of all files to be able to pick up file to process
 * @return -1 on fail or NULL for success
 */
void * worker_thread(void *files_queue);


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t file_cond = PTHREAD_COND_INITIALIZER;

// BUGS: 
// When u type in a decimal or something, it starts going in an infinite loop or something, so reset values or something

// To Do:
// right now program only supports concurrency for amplification since that takes longer than switching audio track
// so create two thread routines one for amplification and one for audio track to support concunrrency for audio track as well


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
        int japanese_audio_track_idx = 2; // which audio track index is the japanese one, by default its track 2 (zero-index)
        int curr_audio_track_idx = 1;

        // need to keep track of how many total tracks there are cause in case that theres only one track it fails

        // loop through each line of the ffprobe output, and see if japanese track is not in first track
        // This assumes that there is a japanese track in the first place, by looking for japanese tag and returning that idx, otherwise defaults to track 1 being japanese
        while (fgets(buffer, sizeof(buffer), fP) != NULL) {

            // printf("%s\n", buffer);

            buffer[strcspn(buffer, "\n")] = 0; // Remove the newline, if present

            if (strncmp(buffer, "index=", 6) == 0) {
                // Example -> convert "index=2" → 2 by skipping first 6 bytes
                curr_audio_track_idx = atoi(buffer + 6);  
            }

            // found a tag for the japaense track, now check to make sure its not already default
            if (strcmp(buffer, "TAG:language=jpn") == 0) {
                if (curr_audio_track_idx == 1) {
                    printf("------defeault audio track is already Japanese------\n");
                    is_default_jpn = true;
                    break;
                }

                japanese_audio_track_idx = curr_audio_track_idx;
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

            char command[MAX_COMMAND_LEN];
            int zero_idx_track = japanese_audio_track_idx - 1;
            snprintf(command, sizeof(command), "ffmpeg -i -map 0:v:0 -map 0:a:%d -map -0:s -c copy -disposition:a:0 default output.mkv", zero_idx_track);
            char **ffmpeg_command = construct_ffmpeg_command(command, original_file_name, destination);

            // int commandCount = get_word_count("ffmpeg -i -map 0:v:0 -map 0:a:m:language:jpn -map -0:s -c copy -disposition:a:0 default ") + 2;

            //                                                     Assumes only up to 9 tracks so 1 byte
            int commandCount = get_word_count("ffmpeg -i -map 0:v:0 -map 0:a:1 -map -0:s -c copy -disposition:a:0 default ") + 2;

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

    // prefixPath is '../../' where by default it starts looking
    pthread_t workers_tid[MAX_WORKERS];

    #ifdef DEBUG
    printf("prefix path: %s\n", prefixPath);
    #endif

    if (dir == NULL) {
        perror("opendir");
        return - 1;
    }

    float amplify_value = 0;

    // value over 1 to amplify or less to decrease volume
    printf("how much to amplify: ");
    scanf("%f", &amplify_value);

    struct dirent *entry; // this is one file from the directory

    int file_count = 0;

    // first get count of files to process to allocate on heap
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // ignore subs folder
        if (strcmp(entry->d_name, "subs") == 0 || strcmp(entry->d_name, "Subs") == 0 || strcmp(entry->d_name, "SUBS") == 0) {
            continue;
        }

        file_count++;

        printf("\n");
    }

    // start pointer to directory at start
    rewinddir(dir);

    struct files_queue *file_q = initialize_queue(file_count, prefixPath, amplify_value);

    if (file_q == NULL) {
        return -1;
    }

    // loop back now filling up job queue
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // ignore subs folder
        if (strcmp(entry->d_name, "subs") == 0 || strcmp(entry->d_name, "Subs") == 0 || strcmp(entry->d_name, "SUBS") == 0) {
            continue;
        }

        #ifdef DEBUG
        printf("adding file: \"%s\" to job queue\n", entry->d_name);
        #endif

        // push file to job queue
        if (add_job(file_q, entry->d_name, prefixPath) != 0) {
            return -1;
        }
    }

    int i;

    int new_thread_res = 0;

    void *status;

    #ifdef DEBUG
    for (i = 0; i < file_count; i++) {
        printf("job: %s\n", file_q->files[i]);
    }
    #endif

    // create worker threads to take on jobs
    for (i = 0; i < MAX_WORKERS; i++) {
        new_thread_res = pthread_create(&workers_tid[i], NULL, worker_thread, (void *)file_q);

        if (new_thread_res != 0) {
            perror("pthread_create");
        }
    }

    for (i = 0; i < MAX_WORKERS; i++) {
        pthread_join(workers_tid[i], &status);
    }

    if (status != NULL) {
        printf("thread fail\n");
        return -1;
    }

    destroy_queue(file_q);

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

struct files_queue *initialize_queue(int file_count, char *prefix_path, float amplify_value) {
    struct files_queue *file_q = malloc(sizeof(struct files_queue));

    if (file_q == NULL) {
        perror("malloc");
        return NULL;
    }

    file_q->files = malloc(sizeof(char *) * file_count);

    if (file_q->files == NULL) {
        perror("malloc");
        return NULL;
    }

    file_q->size = 0;
    file_q->head= 0;
    file_q->capacity = file_count;
    file_q->prefix_path = malloc(strlen(prefix_path) + 1);
    file_q->amplify_value = amplify_value;
    strcpy(file_q->prefix_path, prefix_path);

    return file_q;
}

void destroy_queue(struct files_queue *q) {
    if (q == NULL) {
        return;
    }

    size_t file_count = q->capacity;
    for (size_t i = 0; i < file_count; i++) {
        free(q->files[i]);
    }

    free(q->files);
    free(q->prefix_path);
    free(q);
}

int add_job(queue *files_queue, char *file_path, char *prefix_path) {
    // first need to check if queue is empty, to just append at the head and yep

    if (files_queue == NULL) {
        printf("null queue\n");
        return -1;
    }

    if (files_queue->size == 0) {
        //                                                 + 2 for null terminating and '/'
        files_queue->files[0] = malloc(strlen(file_path) + strlen(prefix_path) + 2);
        strcpy(files_queue->files[0], prefix_path);
        strcat(files_queue->files[0], "/");
        strcat(files_queue->files[0], file_path);
        files_queue->size++;
    } else {
        // case for when queue is not empty
        size_t tail = files_queue->size;
        files_queue->files[tail] = malloc(strlen(file_path) + strlen(prefix_path) + 2);
        strcpy(files_queue->files[tail], prefix_path);
        strcat(files_queue->files[tail], "/");
        strcat(files_queue->files[tail], file_path);
        files_queue->size++;    
    }

    return 0;
}

void * worker_thread(void *files_queue) {

    queue *files_q = (queue *)files_queue;

    while (1) {

        char *original_name = NULL;
        char *destination = NULL;
        
        pthread_mutex_lock(&mutex);
        {
            while (files_q->size < 1) {
                // finished processing all files or last files currently being processed, exit
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }

            // ../../temp/[Judas] Yofukashi - S02E06.mkv
            // get idx of next file to process
            size_t idx = files_q->head;

            // save original name so thread can rename it back after finished processing
            original_name = malloc(strlen(files_q->files[idx]) + 1);
            strcpy(original_name, files_q->files[idx]);

            size_t allocated_size = strlen(files_q->files[idx]) + 6; // + 5 bytes for file number + '\0'

            destination = malloc(allocated_size);

            char *filename_part = &files_q->files[idx][strlen(files_q->prefix_path) + 1];

            snprintf(destination, allocated_size, "%s/%zu%s", files_q->prefix_path, idx, filename_part);

            printf("output: %s\n", destination);

            files_q->head++;
            files_q->size--;
        }

        pthread_mutex_unlock(&mutex);

        // perform ffmpeg command
        char *command = malloc(MAX_COMMAND_LEN);
        snprintf(command, MAX_COMMAND_LEN, "ffmpeg -i -filter:a volume=%.2f -c:v copy ", files_q->amplify_value);

        // this is the acutal command thats gonna get executed
        char **amp_command = construct_ffmpeg_command(command, original_name, destination);

        //                                                      plus 2 for the input and output arguments
        int commandCount = get_word_count("ffmpeg -i -filter:a volume=1.5 -c:v copy ") + 2;
        
        for (int i = 0; i < commandCount; i++) {
            printf("%s ", amp_command[i]);
        }

        int res = perform_command("ffmpeg", amp_command);

        if (res == 1) {

            printf("successfully amplified audio!\n");

            // delete the old video file
            int del_res = remove(original_name);

            if (del_res < 0) {
                printf("failed to delete file %s\n", original_name);
            }

            int fileRenameResult = rename(destination, original_name);
            
            if (fileRenameResult < 0) {
                printf("file rename failed\n");
                return (void *)-1;
            }

        } else {
            return (void *)-1;
        }

        for (int i = 0; i < commandCount; i++) {
            free(amp_command[i]);
        } 
        
        free(amp_command);
        free(original_name);
        free(destination);
    }
}


