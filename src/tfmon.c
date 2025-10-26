//###########################################################################
//# Text File MONitor(tfmon): watch a single or multiple small text         # 
//# files in the command line and follow changes.                           #
//# https://github.com/kadlubski88/tfmon                                    #
//#                                                                         #
//# The MIT License (MIT)                                                   #
//# Copyright © 2025 Georges Kadlubski                                      #
//# URL: https://mit-license.org/                                           #
//###########################################################################

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/inotify.h>
#include <time.h>

#define VERSION "v0.1"
#define PATH_SIZE 128
#define TEXT_SIZE 80
#define TIME_SIZE 24
#define EVENT_BUFFER 4096
#define MAX_FILES 8


//##############################
//# Global variable definition #
//##############################

struct file_info {
    char path[PATH_SIZE];
    int watcher_descriptor;
    char timestamp[TIME_SIZE];
    char text[TEXT_SIZE];
    int change_count;
    int changed;
};

//######################
//# Function prototype #
//######################

void print_help(void);
void redraw(struct file_info files[], int number_of_files);

//########
//# Main #
//########

int main(int argc, char *argv[]) {
    
    //#######################
    //# Variable definition #
    //#######################

    struct file_info files[MAX_FILES];
    int number_of_files = 0;
    int inotify_descriptor;
    ssize_t event_length;
    char event_buffer[EVENT_BUFFER];
    time_t now;
    FILE *file_stream;

    if ((inotify_descriptor = inotify_init()) < 0) {
        fprintf(stderr, "not able to initialize inotify\n");
        exit(EXIT_FAILURE);
    }

    //####################
    //# Argument parsing #
    //####################

    argc--;
    argv++;
    while (argc > 0 && argv[0][0] == '-' ) {
        switch (argv[0][1]) {
        //help option
        case 'h':
            print_help();
            exit(EXIT_SUCCESS);
        //print version
        case 'v':
            printf("%s\n", VERSION);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "\"%s\" is not a valid option\n", *argv);
            exit(EXIT_FAILURE);
            break;
        }
        argv++;
    }
    while (argc > 0) {
        strncpy(files[number_of_files].path, *argv, PATH_SIZE - 1);
        files[number_of_files].watcher_descriptor = inotify_add_watch(inotify_descriptor, *argv, IN_CLOSE_WRITE);
        files[number_of_files].changed = 1;
        files[number_of_files].change_count = 0;
        now = time(NULL);
        struct tm *tm_info = localtime(&now);
        strftime(files[number_of_files].timestamp, TIME_SIZE, "%Y-%m-%d %H:%M:%S", tm_info);
        if ((file_stream = fopen(files[number_of_files].path, "r")) != NULL) {
            fgets(files[number_of_files].text, TEXT_SIZE, file_stream);
            fclose(file_stream);
        } else {
            strcpy(files[number_of_files].timestamp, "File doesnt exists ");
        }

        printf("\n");
        number_of_files++;
        argc--;
        argv++;
    }

    redraw(files, number_of_files);

    //##############
    //# Event loop #
    //##############

    while (1) {
        if ((event_length = read(inotify_descriptor, event_buffer, EVENT_BUFFER)) < 0) {
            fprintf(stderr, "not able to read from the inotify descriptor\n");
            exit(EXIT_FAILURE);
        }
        for (char *event_pointer = event_buffer; event_pointer < (event_buffer + event_length); ) {
            struct inotify_event *event = (struct inotify_event *) event_pointer;
            files[event->wd - 1].changed = 1;
            now = time(NULL);
            struct tm *tm_info = localtime(&now);
            strftime(files[event->wd - 1].timestamp, TIME_SIZE, "%Y-%m-%d %H:%M:%S", tm_info);
            file_stream = fopen(files[event->wd - 1].path, "r");
            fgets(files[event->wd - 1].text, TEXT_SIZE, file_stream);
            fclose(file_stream);
            files[event->wd - 1].change_count++;
            event_pointer += sizeof(struct inotify_event) + event->len;
        }
        redraw(files, number_of_files);
    }

    for (int i = 0; i < number_of_files; i++) {
        printf("%s\n", files[i].path);
    }
    
    exit(EXIT_SUCCESS);
}

//#######################
//# Function definition #
//#######################

void print_help(void) {
    printf("Text File MONitor (tfmon) %s\n\n", VERSION);
    printf(
    "Usage:\n"
    "  tfmon [options] [file 1] [file 2] ... [file n]\n\n"
    "Options:\n"
    "  -v           Print the version\n"
    "  -h           Print this help\n"
    );
}

void redraw(struct file_info files[], int number_of_files) {
    printf("\033[%dA", number_of_files);
    for (int i = 0; i < number_of_files; i++) {
        files[i].changed = 0;
        printf("\033[K%s | %s | %i | %s\n", files[i].path, files[i].timestamp,files[i].change_count, files[i].text);
    }
    return;
}