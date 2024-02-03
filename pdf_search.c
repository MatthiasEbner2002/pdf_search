#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poppler.h>
#include <gio/gio.h>
#include <omp.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/stat.h>

static int MAX_PATH_LENGTH = 1024;

/*
    Struct to store the parameters of the search
    - word_to_search: The word to search for
    - sensitive_search: If true, the search is case sensitive
    - recursive_search: If true, the search is recursive in subfolders
*/
typedef struct
{
    char *word_to_search;
    bool sensitive_search;
    bool recursive_search;
    bool print_lines_with_occurences;
} Parameters;

/*
    Struct to store the result of the search
    - pdf_path: The path of the pdf file
    - num_occurences: The number of occurences of the word in the pdf file
    - first_page_with_occurence: The number of the first page with an occurence of the word
*/
typedef struct
{
    const char *pdf_path;
    int num_occurences;
    int first_page_with_occurence;

} Result;

/*
    Struct to store the data of a page
    - page_number: The number of the page
    - num_occurences: The number of occurences of the word in the page
*/
typedef struct
{
    int page_number;
    int num_occurences;
    char **lines;
} PageData;

/*
    Compares two PageData structs by page number

    PARAMETERS:
        - a     : is a pointer to the first PageData struct
        - b     : is a pointer to the second PageData struct
    RETURN:
        - 0 if the page numbers are equal
        - a negative value if the page number of the first struct is smaller than the page number of the second struct
        - a positive value if the page number of the first struct is greater than the page number of the second struct
*/
int compare_page_data(const void *a, const void *b){
    const PageData *data_a = (const PageData *)a;
    const PageData *data_b = (const PageData *)b;
    return data_a->page_number - data_b->page_number;
}

/*
    Searches for a word in a pdf file and returns the result of the search

    PARAMETERS:
        - pdf_path      : is the path of the pdf file
        - parameters    : is a pointer to the parameters of the search
    RETURN:
        - a pointer to the result of the search
        - NULL if the pdf file doesn't contain the word
*/
Result *search_in_pdf(const char *pdf_path, Parameters *parameters){
    PopplerDocument *doc;
    GError *error = NULL;
    int i, num_pages;
    int max_matches = 100; // Maximum number of matches to store, for start only
    int num_matches = 0;
    PageData *matches = calloc(max_matches, sizeof(PageData));

    gchar *file_uri = g_filename_to_uri(pdf_path, NULL, &error);
    if (error != NULL)
    {
        fprintf(stderr, "Error converting file path to URI: %s\n", error->message);
        g_error_free(error);
        free(matches);
        return NULL;
    }

    doc = poppler_document_new_from_file(file_uri, NULL, &error);
    if (error != NULL)
    {
        fprintf(stderr, "Error opening PDF file: %s\n", error->message);
        g_error_free(error);
        g_free(file_uri);
        free(matches);
        return NULL;
    }

    
    num_pages = poppler_document_get_n_pages(doc);
    #pragma omp parallel for private(i) shared(num_matches) // num_threads(8)
    for (i = 0; i < num_pages; i++){
        PopplerPage *page = poppler_document_get_page(doc, i);
        gchar *text = poppler_page_get_text(page);
        gchar *word = parameters->word_to_search;
        
        if (parameters->sensitive_search == false){
            text = g_ascii_strdown(text, -1);
            word = g_ascii_strdown(parameters->word_to_search, -1);
        }

        char *lines_found[100];
        gboolean local_match_found = false;
        int local_num_occurences = 0;
        if (parameters->print_lines_with_occurences == true){
            gchar **lines = g_strsplit(text, "\n", -1);
            for (int j = 0; lines[j] != NULL; j++){
                if (strstr(lines[j], word) != NULL){
                    if (local_match_found == false)
                        local_match_found = true;
                    lines_found[local_num_occurences] = lines[j];
                    local_num_occurences++;
                }
            }
        }
        else {
            char *found_word = strstr(text, word);
            while (found_word != NULL){
                if (local_match_found == false)
                    local_match_found = true;

                local_num_occurences++;
                text = g_strdup(found_word + strlen(word));
                if (text == NULL)
                    break;
                found_word = strstr(text, word);
            }
        }

        if (local_match_found){
            #pragma omp critical
            {
                if (num_matches >= max_matches){
                    max_matches *= 2;
                    matches = realloc(matches, max_matches * sizeof(PageData));
                }
                matches[num_matches].page_number = i + 1;
                matches[num_matches].num_occurences = local_num_occurences;
                if (parameters->print_lines_with_occurences == true){
                    matches[num_matches].lines = malloc(local_num_occurences * sizeof(char *));
                    for (int j = 0; j < local_num_occurences; j++){
                        matches[num_matches].lines[j] = malloc(1 + strlen(lines_found[j]) * sizeof(char));
                        strcpy(matches[num_matches].lines[j], lines_found[j]);
                    }
                }
                num_matches++;
            }
        }
    // g_object_unref(page); // this breaks it, dont know why TODO: fix
    }

    g_free(file_uri);
    g_object_unref(doc);

    if (num_matches == 0){
        free(matches);
        return NULL;
    }

    // Sort the matches by page number
    qsort(matches, num_matches, sizeof(PageData), compare_page_data);

    Result *result = malloc(sizeof(Result));
    result->pdf_path = pdf_path;
    result->first_page_with_occurence = matches[0].page_number;
    result->num_occurences = 0;
    if (parameters->print_lines_with_occurences == true){
        for (i = 0; i < num_matches; i++){
            for (int j = 0; j < matches[i].num_occurences; j++){
                printf("\033[1;31m%s\033[0m - \033[1;31m%d\033[0m: %s\n",
                    g_path_get_basename(pdf_path),
                    matches[i].page_number,
                    matches[i].lines[j]
                );
                result->num_occurences += 1;
            }
        }
    }
    else {
        // Print the matched page numbers
        printf("\033[1;31m%s\033[0m:", g_path_get_basename(pdf_path)); // Print the file name in red
        for (i = 0; i < num_matches; i++){
            printf(" %d", matches[i].page_number);
            if (matches[i].num_occurences > 1)
                printf("(x%d)", matches[i].num_occurences);
            if (i < num_matches - 1) 
                printf(",");
            result->num_occurences += matches[i].num_occurences;
        }
        printf("\n");
    }

    // Free the memory used by the matches
    for (int i = 0; i < num_matches; i++){
        if (parameters->print_lines_with_occurences == true){
            for (int j = 0; j < matches[i].num_occurences; j++)
                free(matches[i].lines[j]);
            free(matches[i].lines);
        }
    }
    free(matches);

    return result;
}

/*
    Compares two strings

    PARAMETERS:
        - a     : is a pointer to the first string
        - b     : is a pointer to the second string
    RETURN:
        - a negative value if the first string is smaller than the second one
        - 0 if the strings are equal
        - a positive value if the first string is greater than the second one
 */
int compare_paths(const void *a, const void *b){
    return strcmp(*(const char **)a, *(const char **)b);
}

/*
    Function to check if a file has a .pdf extension

    PARAMETERS:
        - filename  : is the name of the file to check

    RETURN:
        - 1 if the file has a .pdf extension
        - 0 otherwise
*/
int is_pdf_file(const char *filename){
    const char *extension = strrchr(filename, '.');
    if (extension != NULL && strcmp(extension, ".pdf") == 0)
        return 1;
    return 0;
}

/*
    Searches for PDF files in a folder and its subfolders
    Returns an array of paths to the PDF files

    PARAMETERS:
        - folderPath        : is the path to the folder to search in
        - pdfPaths          : is a pointer to an array of strings that will contain the paths to the PDF files
        - count             : is the number of PDF files found
        - searchSubfolders  : indicates whether to search in subfolders
*/
void search_pdf_files_recursive(const char *folderPath, char ***pdfPaths, int *count, bool searchSubfolders){
    DIR *dir = opendir(folderPath);
    if (dir != NULL)
    {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL){
            if (strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0){
                // Create the complete file path
                int pathLength = strlen(folderPath) + strlen(ent->d_name) + 2;
                char *filePath = (char *)malloc(pathLength * sizeof(char));
                snprintf(filePath, pathLength, "%s/%s", folderPath, ent->d_name);

                struct stat st;
                if (searchSubfolders && stat(filePath, &st) == 0 && S_ISDIR(st.st_mode)){
                    // Recursively search in subfolders
                    search_pdf_files_recursive(filePath, pdfPaths, count, searchSubfolders);
                }

                if (is_pdf_file(ent->d_name)){
                    // Add the PDF file path to the array
                    (*pdfPaths)[(*count)++] = filePath;
                    *pdfPaths = (char **)realloc(*pdfPaths, (*count + 1) * sizeof(char *));
                }
                else
                    free(filePath);
            }
        }
        closedir(dir);
    }
}

/*
    Returns an array of paths to all PDF files in the specified folder

    PARAMETERS:
        - folderPath        : is the path to the folder to search in
        - count             : is a pointer to an integer that will contain the number of PDF files found
        - searchSubfolders  : indicates whether to search in subfolders

    RETURN:
        - an array of paths to the PDF files
*/
char **list_pdf_files(const char *folderPath, int *count, bool searchSubfolders){
    // Allocate memory for the array of paths
    char **pdfPaths = (char **)malloc(sizeof(char *));
    *count = 0;
    // Search for PDF files recursively
    search_pdf_files_recursive(folderPath, &pdfPaths, count, searchSubfolders);
    // Sort the paths alphabetically
    qsort(pdfPaths, *count, sizeof(char *), compare_paths);
    return pdfPaths;
}

/*
    prints the usage of the program

    PARAMETERS:
        - argv          : is the array of arguments
        - exit_code     : is the exit code to return
*/
void print_usage_and_exit(char *argv[], int exit_code){
    printf("Usage: %s <word_to_search> [-s] [-r] [-h/--help]\n", argv[0]);
    exit(exit_code);
}

int main(int argc, char *argv[]){

    char cwd[MAX_PATH_LENGTH]; // Buffer to store the current path
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        perror("getcwd() error");
        return EXIT_FAILURE;
    }

    if (argc < 2 || argc > 5)
        print_usage_and_exit(argv, EXIT_FAILURE);

    Parameters param = {
        .word_to_search = NULL,
        .sensitive_search = false,
        .recursive_search = false, 
        .print_lines_with_occurences = false
    };

    for (int i = 1; i < argc; i++){
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
            print_usage_and_exit(argv, EXIT_SUCCESS);
        else if (strcmp(argv[i], "-s") == 0)
            param.sensitive_search = true;
        else if (strcmp(argv[i], "-r") == 0)
            param.recursive_search = true;
        else if (strcmp(argv[i], "-l") == 0)
            param.print_lines_with_occurences = true;
        else if (param.word_to_search == NULL)
            param.word_to_search = argv[i];
        else
            print_usage_and_exit(argv, EXIT_FAILURE);
    }

    if (param.word_to_search == NULL){
        printf("Error: no word to search\n");
        print_usage_and_exit(argv, EXIT_FAILURE);
    }

    int count;
    char **pdfPaths = list_pdf_files(cwd, &count, param.recursive_search);

    ///////////////////////////////////////
    // search here and print the results //
    ///////////////////////////////////////

    Result *ret = NULL;
    // Print the paths to PDF files
    for (int i = 0; i < count; i++){
        Result *result = search_in_pdf(pdfPaths[i], &param);
        if (result != NULL && (ret == NULL || result->num_occurences > ret->num_occurences)){
            // save the result if it has more occurences than the previous one
            if (ret != NULL){
                free(ret); // Free the previously allocated memory
            }
            ret = result;
        }
        else
            free(result);
    }

    // exit if no matches found 
    if (ret == NULL){
        printf("No matches found\n");
        for (int i = 0; i < count; i++)
            free(pdfPaths[i]);
        free(pdfPaths);
        return 0;
    }
    printf("Total occurences: %d\n", ret->num_occurences);
    ////////////////////////////////////////////////////
    // Ask the user if they want to open the PDF file //
    ////////////////////////////////////////////////////
    char answer[10];
    do{
        printf("Open %s(%d)? ([Enter]y/n): ", g_path_get_basename(ret->pdf_path), ret->first_page_with_occurence);
        if (fgets(answer, sizeof(answer), stdin) == NULL){
            printf("Error reading input.\n");
            return 1;
        }

        // Remove the trailing newline character
        answer[strcspn(answer, "\n")] = '\0';

        if (strcmp(answer, "y") == 0 || strlen(answer) == 0)
            break;
        else if (strcmp(answer, "n") == 0 || strcmp(answer, "N") == 0)
            return 0;
    } while (1);

    ///////////////////////
    // Open the PDF file //
    ///////////////////////

    // try to open the file with okular
    char command[300];
    snprintf(command, sizeof(command), "okular \"%s\" --find=\"%s\" --page=%d &", ret->pdf_path, param.word_to_search, ret->first_page_with_occurence);
    printf("Executing command: '%s'\n", command);
    int result = system(command); // execute command

    // Check if could be opened with okular
    if (result == -1){
        perror("Error opening PDF file with okular");
        printf("Opening with default program...\n");
        snprintf(command, sizeof(command), "xdg-open %s", ret->pdf_path);
        int result = system(command); // execute command
        // check if could be opened with default program
        if (result == -1)
            perror("Error opening PDF file with default program");
    };

    if (ret != NULL)
        free(ret);

    for (int i = 0; i < count; i++)
        free(pdfPaths[i]);
    free(pdfPaths);
    return 0;
}
