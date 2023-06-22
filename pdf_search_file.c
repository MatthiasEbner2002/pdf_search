#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poppler.h>
#include <gio/gio.h>
#include <omp.h>

void search_in_pdf(const char *pdf_path, const char *word, int search_method) {
    PopplerDocument *doc;
    GError *error = NULL;
    int i, num_pages;
    gboolean found = FALSE;

    gchar *file_uri = g_filename_to_uri(pdf_path, NULL, &error);
    if (error != NULL) {
        fprintf(stderr, "Error converting file path to URI: %s\n", error->message);
        g_error_free(error);
        return;
    }

    doc = poppler_document_new_from_file(file_uri, NULL, &error);
    if (error != NULL) {
        fprintf(stderr, "Error opening PDF file: %s\n", error->message);
        g_error_free(error);
        g_free(file_uri);
        return;
    }

    num_pages = poppler_document_get_n_pages(doc);

    gboolean match_found = FALSE;
    //printf("Word '%s' found in file '\033[1m%s\033[0m' pages:", word, g_path_get_basename(pdf_path));
    printf("\033[1m%s\033[0m:", g_path_get_basename(pdf_path));

    #pragma omp parallel shared(match_found)
    {
        #pragma omp for private(i, found) 
        for (i = 0; i < num_pages; i++) {
            PopplerPage *page = poppler_document_get_page(doc, i);
            gchar *text = poppler_page_get_text(page);
            gchar *lowercase_text = g_ascii_strdown(text, -1);
            gchar *lowercase_word = g_ascii_strdown(word, -1);
            gboolean local_match_found = FALSE;

            if (search_method == 1) {
                // Method 1: Using strstr to find the first occurrence
                if (strstr(lowercase_text, lowercase_word) != NULL)
                    local_match_found = TRUE;
            } else if (search_method == 2) {
                // Method 2: Using a loop to find all occurrences
                gchar *occurrence = lowercase_text;
                while ((occurrence = strstr(occurrence, lowercase_word)) != NULL) {
                    local_match_found = TRUE;
                    occurrence += strlen(lowercase_word);
                }
            }

            if (local_match_found) {
                #pragma omp atomic write
                match_found = TRUE;

                if (!found) {
                    found = TRUE;
                }

                printf(" %d,", i + 1);
            }

            g_free(lowercase_word);
            g_free(lowercase_text);
            g_free(text);
            g_object_unref(page);
        }
    }

    if (match_found) {
        printf("\n");
    }

    g_free(file_uri);
    g_object_unref(doc);
}



int main(int argc, char *argv[]) {
    // TODO: make it possible to have a strict search (case sensitive)
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <pdf_path> <word_to_search> <search_method>\n", argv[0]);
        fprintf(stderr, "Search method: 1 - First occurrence, 2 - All occurrences\n");
        return 1;
    }

    const char *pdf_path = argv[1];         // pdf file path
    const char *word_to_search = argv[2];   // word to search
    int search_method = atoi(argv[3]);      // search method (1 or 2) - see main() for details

    search_in_pdf(pdf_path, word_to_search, search_method);

    return 0;
}
