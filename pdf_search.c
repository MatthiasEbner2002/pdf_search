#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poppler.h>
#include <gio/gio.h>
#include <omp.h>
#include <dirent.h>
#include <unistd.h>

typedef struct {
    int page_number;
    gboolean match_found;
} PageData;

int compare_page_data(const void *a, const void *b) {
    const PageData *data_a = (const PageData *)a;
    const PageData *data_b = (const PageData *)b;

    return data_a->page_number - data_b->page_number;
}

void search_in_pdf(const char *pdf_path, const char *word) {
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
        return;
    }

    doc = poppler_document_new_from_file(file_uri, NULL, &error);
    if (error != NULL) {
        fprintf(stderr, "Error opening PDF file: %s\n", error->message);
        g_error_free(error);
        g_free(file_uri);
        free(matches);
        return;
    }

    num_pages = poppler_document_get_n_pages(doc);

    #pragma omp parallel for private(i) shared(num_matches) // num_threads(8)
    for (i = 0; i < num_pages; i++) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        gchar *text = poppler_page_get_text(page);
        gchar *lowercase_text = g_ascii_strdown(text, -1);
        gchar *lowercase_word = g_ascii_strdown(word, -1);
        gboolean local_match_found = FALSE;


        if (strstr(lowercase_text, lowercase_word) != NULL)
            local_match_found = TRUE;


        if (local_match_found) {
            #pragma omp critical
            {
                if (num_matches >= max_matches) {
                    max_matches *= 2;
                    matches = realloc(matches, max_matches * sizeof(PageData));
                }

                matches[num_matches].page_number = i + 1;
                matches[num_matches].match_found = TRUE;
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

    // Sort the matches by page number
    qsort(matches, num_matches, sizeof(PageData), compare_page_data);

    // Print the matched page numbers
    printf("\033[1;31m%s\033[0m:", g_path_get_basename(pdf_path));
    for (i = 0; i < num_matches; i++) {
        printf(" %d", matches[i].page_number);
        if (i < num_matches - 1) {
            printf(",");
        }
    }
    printf("\n");

    free(matches);
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

    // Print the paths to PDF files
    for (int i = 0; i < count; i++) {
        search_in_pdf(pdfPaths[i], word_to_search);
    }

    // Free the allocated memory
    for (int i = 0; i < count; i++) {
        free(pdfPaths[i]);
    }
    free(pdfPaths);


    return 0;
}
