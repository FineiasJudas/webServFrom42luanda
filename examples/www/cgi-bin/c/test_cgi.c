#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Content-Type: text/plain\r\n\r\n");
    printf("=== C CGI TEST ===\n\n");

    printf("REQUEST_METHOD: %s\n", getenv("REQUEST_METHOD"));
    printf("QUERY_STRING: %s\n", getenv("QUERY_STRING"));
    printf("CONTENT_LENGTH: %s\n", getenv("CONTENT_LENGTH"));
    printf("SCRIPT_NAME: %s\n", getenv("SCRIPT_NAME"));
    printf("SCRIPT_FILENAME: %s\n", getenv("SCRIPT_FILENAME"));
    printf("REQUEST_URI: %s\n", getenv("REQUEST_URI"));
    printf("DOCUMENT_ROOT: %s\n\n", getenv("DOCUMENT_ROOT"));

    char *method = getenv("REQUEST_METHOD");
    if (method && strcmp(method, "POST") == 0) {
        char *len_str = getenv("CONTENT_LENGTH");
        int len = len_str ? atoi(len_str) : 0;

        printf("POST DATA:\n");

        if (len > 0) {
            char *buffer = malloc(len + 1);
            if (buffer) {
                fread(buffer, 1, len, stdin);
                buffer[len] = '\0';
                printf("%s\n", buffer);
                free(buffer);
            }
        }
    }

    return 0;
}
