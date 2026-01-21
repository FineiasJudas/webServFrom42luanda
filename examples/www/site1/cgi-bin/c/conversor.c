#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_param_value(char *arg) {
    char *eq = strchr(arg, '=');
    if (!eq) return NULL;
    return eq + 1;
}

void convert_units(char *value_str, char *unit) {
    if (!value_str || !unit) {
        printf("{\"error\": \"Faltam parâmetros! Use ?value=10&unit=m\"}\n");
        return;
    }

    double value = atof(value_str);

    printf("{");
    printf("\"original\": {\"value\": %.2f, \"unit\": \"%s\"},", value, unit);
    
    if (strcmp(unit, "m") == 0) {
        printf("\"conversions\": {");
        printf("\"km\": %.3f,", value / 1000);
        printf("\"cm\": %.2f", value * 100);
        printf("}");
    } 
    else if (strcmp(unit, "kg") == 0) {
        printf("\"conversions\": {\"g\": %.2f}", value * 1000);
    } 
    else if (strcmp(unit, "cm") == 0) {
        printf("\"conversions\": {\"m\": %.2f}", value / 100);
    } 
    else {
        printf("\"error\": \"Unidade desconhecida: %s\"", unit);
    }

    printf("}\n");
}

int main(int argc, char *argv[]) {
    // Cabeçalho JSON
    printf("Content-Type: application/json\r\n\r\n");

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
    return 0;
}
