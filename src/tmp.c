#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILENAME_LENGTH 256
#define MAX_LINE_LENGTH 1024
#define MAX_FILES 100

int main() {
    FILE *listFile, *currentFile;
    char filenames[MAX_FILES][MAX_FILENAME_LENGTH];
    char firstLine[MAX_LINE_LENGTH];
    int fileCount = 0;

    // Open the file that contains the list of filenames
    listFile = fopen("file_list.txt", "r");
    if (listFile == NULL) {
        perror("Error opening file_list.txt");
        return EXIT_FAILURE;
    }

    // Read each filename from the list file and store it in an array
    while (fgets(filenames[fileCount], MAX_FILENAME_LENGTH, listFile) != NULL && fileCount < MAX_FILES) {
        // Remove the newline character from the filename, if present
        filenames[fileCount][strcspn(filenames[fileCount], "\n")] = '\0';
        fileCount++;
    }

    // Close the list file
    fclose(listFile);

    // Use a for loop to process each file in the array
    for (int i = 0; i < fileCount; i++) {
        // Open the current file
        currentFile = fopen(filenames[i], "r");
        if (currentFile == NULL) {
            perror("Error opening file");
            continue; // Skip to the next file
        }

        // Read and print the first line of the current file
        if (fgets(firstLine, MAX_LINE_LENGTH, currentFile) != NULL) {
            printf("First line of %s: %s", filenames[i], firstLine);
        } else {
            printf("File %s is empty or an error occurred.\n", filenames[i]);
        }

        // Close the current file
        fclose(currentFile);
    }

    return EXIT_SUCCESS;
}
