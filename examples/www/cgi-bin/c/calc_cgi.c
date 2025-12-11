#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    printf("Content-Type: text/plain\r\n\r\n");

    printf("=== CGI CALCULATOR (C + ARGV) ===\n\n");

    if (argc < 3) {
        printf("Error: missing arguments.\n");
        printf("Usage: /cgi-bin/calc.cgi?x=10&y=20\n");
        return 1;
    }

    int a = atoi(argv[1]);
    int b = atoi(argv[2]);

    printf("A = %d\n", a);
    printf("B = %d\n", b);
    printf("SUM = %d\n", a + b);
    printf("SUB = %d\n", a - b);
    printf("MUL = %d\n", a * b);

    if (b != 0) {
        printf("DIV = %d\n", a / b);
    } else {
        printf("DIV = ERROR (division by zero)\n");
    }

    return 0;
}
