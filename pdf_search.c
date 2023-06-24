#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poppler.h>
#include <gio/gio.h>
#include <omp.h>
#include <dirent.h>
#include <unistd.h>

typedef struct {
    const char *pdf_path;
    int num_occurences;
    int first_page_with_occurence;
} Result;


typedef struct {
    int page_number;
    gboolean match_found;
    int num_occurences;
} PageData;


int compare_page_data(const void *a, const void *b) {
    const PageData *data_a = (const PageData *)a;
    const PageData *data_b = (const PageData *)b;

    return data_a->page_number - data_b->page_number;
}

Result* search_in_pdf(const char *pdf_path, const char *word) {
    PopplerDocument *doc;
    GError *error = NULL;
    int i, num_pages;
    int max_matches = 100; // Maximum number of matches to store
    int num_matches = 0;
    PageData *matches = malloc(max_matches * sizeof(PageData));

    gchar *file_uri = g_filename_to_uri(pdf_path, NULL, &error);
    if (error != NULL) {
        fprintf(stderr, "Error converting file path to URI: %s\n", error->message);
        g_error_free(error);
        free(matches);
        return NULL;
    }

    doc = poppler_document_new_from_file(file_uri, NULL, &error);
    if (error != NULL) {
        fprintf(stderr, "Error opening PDF file: %s\n", error->message);
        g_error_free(error);
        g_free(file_uri);
        free(matches);
        return NULL;
    }

    num_pages = poppler_document_get_n_pages(doc);

    #pragma omp parallel for private(i) shared(num_matches) // num_threads(8)
    for (i = 0; i < num_pages; i++) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        gchar *text = poppler_page_get_text(page);
        gchar *lowercase_text = g_ascii_strdown(text, -1);
        gchar *lowercase_word = g_ascii_strdown(word, -1);
        gboolean local_match_found = FALSE;
        int local_num_occurences = 0;

        // search for all occurences of the word in the page
        while(strstr(lowercase_text, lowercase_word) != NULL) {
            if(local_match_found == FALSE){
                local_match_found = TRUE;
            }
            local_num_occurences++;
            lowercase_text = g_strdup(strstr(lowercase_text, lowercase_word) + strlen(lowercase_word));
        }


        if (local_match_found) {
            #pragma omp critical
            {
                if (num_matches >= max_matches) {
                    max_matches *= 2;
                    matches = realloc(matches, max_matches * sizeof(PageData));
                }

                matches[num_matches].page_number = i + 1;
                matches[num_matches].match_found = TRUE;
                matches[num_matches].num_occurences = local_num_occurences;
                num_matches++;
            }
        }

        g_free(lowercase_word);
        g_free(lowercase_text);
        g_free(text);
        g_object_unref(page);
    }

    g_free(file_uri);
    g_object_unref(doc);

    if (num_matches == 0) {
        free(matches);
        return NULL;
    }

    // Sort the matches by page number
    qsort(matches, num_matches, sizeof(PageData), compare_page_data);

    Result *result = malloc(sizeof(Result));
    result->pdf_path = pdf_path;
    result->first_page_with_occurence = matches[0].page_number;

    // Print the matched page numbers
    printf("\033[1;31m%s\033[0m:", g_path_get_basename(pdf_path));      // Print the file name in red
    for (i = 0; i < num_matches; i++) {
        printf(" %d", matches[i].page_number);
        if (matches[i].num_occurences > 1) {        // Print the number of occurences if it's greater than 1
            printf("(x%d)" , matches[i].num_occurences);
        }
        if (i < num_matches - 1) {         // Print a comma after the page number if it's not the last one
            printf(",");
        }
        result->num_occurences += matches[i].num_occurences;
    }
    printf("\n");
    free(matches);

    return result;
}

int comparePaths(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}


char** listPDFFiles(const char* folderPath, int* count) {
    DIR *dir;
    struct dirent *ent;

    // Open the directory
    if ((dir = opendir(folderPath)) != NULL) {
        // Count the number of PDF files
        int pdfCount = 0;
        while ((ent = readdir(dir)) != NULL) {
            // Check if the file has a .pdf extension
            if (strstr(ent->d_name, ".pdf") != NULL) {
                pdfCount++;
            }
        }
        
        // Allocate memory for the array of paths
        char** pdfPaths = (char**)malloc(pdfCount * sizeof(char*));
        int index = 0;
        rewinddir(dir);
        
        // Store the paths of PDF files in the array
        while ((ent = readdir(dir)) != NULL) {
            if (strstr(ent->d_name, ".pdf") != NULL) {
                // Allocate memory for the path string
                int pathLength = strlen(folderPath) + strlen(ent->d_name) + 2;
                char* filePath = (char*)malloc(pathLength * sizeof(char));
                
                // Create the complete file path
                snprintf(filePath, pathLength, "%s/%s", folderPath, ent->d_name);
                
                // Store the file path in the array
                pdfPaths[index] = filePath;
                index++;
            }
        }

        closedir(dir);
        *count = pdfCount;
         // Sort the paths alphabetically
        qsort(pdfPaths, pdfCount, sizeof(char*), comparePaths);

        return pdfPaths;
    } else {
        // Failed to open the directory
        perror("Unable to open the directory");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]) {

    char cwd[1024]; // Buffer to store the path

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("getcwd() error");
        return 1;
    }

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <word_to_search> \n", argv[0]);
        return 1;
    }

    
    const char *word_to_search = argv[1];   // word to search

    int count;
    char** pdfPaths = listPDFFiles(cwd, &count);


    ///////////////////////////////////////
    // search here and print the results //
    ///////////////////////////////////////
    Result *ret = NULL;
    // Print the paths to PDF files
    for (int i = 0; i < count; i++) {
        Result *result = search_in_pdf(pdfPaths[i], word_to_search);
        if (result != NULL && (ret == NULL || result->num_occurences > ret->num_occurences)) {
            // save the result if it has more occurences than the previous one
            if (ret != NULL) {
                free(ret);  // Free the previously allocated memory
            }
            ret = result;
        } else {
            free(result);  // Free the memory of the current result since it is not needed
        }
    }

    //////////////////////////////
    // exit if no matches found //
    //////////////////////////////
    if (ret == NULL) {
        // No matches found, free the memory and return
        printf("No matches found\n");
        for (int i = 0; i < count; i++) {
            free(pdfPaths[i]);
        }
        free(pdfPaths);
        return 0;
    
    }


    ////////////////////////////////////////////////////
    // Ask the user if they want to open the PDF file //
    ////////////////////////////////////////////////////
    char answer[10];
    do {
        printf("Open %s(%d)? ([Enter]y/n): ", g_path_get_basename(ret->pdf_path), ret->first_page_with_occurence);
         if (fgets(answer, sizeof(answer), stdin) == NULL) {
            printf("Error reading input.\n");
            return 1;
        }

        // Remove the trailing newline character
        answer[strcspn(answer, "\n")] = '\0';

        if (strcmp(answer, "y") == 0 || strlen(answer) == 0) {
            break; 
        } else if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0) {
            return 0;
        } 
    } while (1); 


    ///////////////////////////////////
    // Open the PDF file with okular //
    ///////////////////////////////////
    char pdf_path[strlen(ret->pdf_path) + 1];  // Create a copy of the path string
    strcpy(pdf_path, ret->pdf_path);
    
    // Create a string for the first page number
    char first_page_str[16];
    snprintf(first_page_str, sizeof(first_page_str), "%d", ret->first_page_with_occurence);

    const char* command = "okular"; // Command to execute
    char* const command_args[] = {"okular", pdf_path, "--find", (char*)word_to_search,
                                    "--page", first_page_str, NULL}; // Arguments to pass to the command

    free(ret);

    for (int i = 0; i < count; i++) {
        free(pdfPaths[i]);
    }
    free(pdfPaths);

    // Execute the command, replacing the current process
    execvp(command, command_args);

    // The following code will only execute if the execvp function fails
    perror("Command execution failed");
    return 1;



}
