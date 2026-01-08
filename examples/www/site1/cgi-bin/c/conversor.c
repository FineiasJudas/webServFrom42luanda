// Compilar:
// gcc ./examples/www/cgi-bin/c/conversor.c -o ./examples/www/cgi-bin/c/conversor.cgi

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_param_value(char *arg) {
    char *eq = strchr(arg, '=');
    if (!eq) return NULL;
    return eq + 1;
}

void html_header(const char *title) {
    printf("Content-Type: text/html\r\n\r\n");
    printf("<!DOCTYPE html>\n<html lang=\"pt\">\n<head>\n");
    printf("    <meta charset=\"UTF-8\">\n");
    printf("    <title>%s</title>\n", title);
    printf("    <style>\n");
    printf("        body { font-family: Arial, sans-serif; margin: 2em; }\n");
    printf("        .result { font-weight: bold; margin-top: 1em; }\n");
    printf("    </style>\n");
    printf("</head>\n<body>\n");
    printf("    <h1>%s</h1>\n", title);
}

void html_footer() {
    printf("</body>\n</html>\n");
}



void convert_units(char *value_str, char *unit) {
    if (!value_str || !unit) {
        printf("<p>Faltam parâmetros! Use ?value=10&unit=m</p>\n");
        return;
    }

    double value = atof(value_str);
    printf("<p>Valor original: %.2f %s</p>\n", value, unit);

    if (strcmp(unit, "m") == 0) {
        printf("<p class=\"result\">%.2f metros = %.2f quilômetros</p>\n", value, value / 1000);
        printf("<p class=\"result\">%.2f metros = %.2f centímetros</p>\n", value, value * 100);
    } 
    else if (strcmp(unit, "kg") == 0) {
        printf("<p class=\"result\">%.2f kg = %.2f gramas</p>\n", value, value * 1000);
    } 
    else if (strcmp(unit, "cm") == 0) {
        printf("<p class=\"result\">%.2f centímetros = %.2f metros</p>\n", value, value / 100);
    } 
    else {
        printf("<p>Unidade desconhecida: %s</p>\n", unit);
    }
}





int main(int argc, char *argv[]) {
    html_header("Conversor de Unidades CGI em C");

    char *value = NULL;
    char *unit = NULL;

    for (int i = 1; i < argc; i++)
    {
        if (strncmp(argv[i], "value=", 6) == 0) {
            value = get_param_value(argv[i]);
        }
        if (strncmp(argv[i], "unit=", 5) == 0) {
            unit = get_param_value(argv[i]);
        }
    }

    convert_units(value, unit);

    html_footer();
    return 0;
}
