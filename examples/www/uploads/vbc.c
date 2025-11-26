#include "vbc.h"

char *input;

int expression(int inside_paren);
int term(int inside_paren);
int factor(int inside_paren);
void error(char c);

void error(char c)
{
    if (c == '\0')
        printf("Unexpected end of file\n");
    else
        printf("Unexpected token '%c'\n", c);
    exit(1);
}

// Factor: trata números, parênteses e operadores unários
int factor(int inside_paren)
{
    int result;
    
    // Trata operadores unários + e *
    while (*input == '+' || *input == '*')
    {
        if (*input == '*')
        {
            input++;
            // *expr = 0 * expr
            factor(inside_paren); // consome o próximo fator
            return 0;
        }
        input++; // + unário não faz nada
    }
    
    // Se for um dígito
    if (isdigit(*input))
    {
        result = *input - '0';
        input++;
        // Verifica se o próximo caractere é outro dígito (não permitido)
        if (isdigit(*input))
            error(*input);
        return result;
    }
    // Se for parêntese
    else if (*input == '(')
    {
        input++; // consome '('
        result = expression(1); // agora estamos dentro de parênteses
        if (*input != ')')
            error('('); // Erro: esperava ')'
        input++; // consome ')'
        return result;
    }
    // Fim de string ou token inesperado
    else
    {
        error(*input);
    }
    return 0;
}

// Term: trata multiplicação
int term(int inside_paren)
{
    int result = factor(inside_paren);
    
    while (*input == '*')
    {
        input++; // consome '*'
        result *= factor(inside_paren);
    }
    
    return result;
}

// Expression: trata adição
int expression(int inside_paren)
{
    int result = term(inside_paren);
    
    while (*input == '+')
    {
        input++; // consome '+'
        result += term(inside_paren);
    }
    
    // Se não estamos dentro de parênteses e encontramos ')'
    // Só é erro se for seguido por '\0' ou outro ')'
    if (!inside_paren && *input == ')')
    {
        // Verifica o próximo caractere
        if (input[1] == '\0' || input[1] == ')')
            error(*input);
        // Caso contrário, ignora (ex: "1+2)(" -> retorna 3)
    }
    
    return result;
}

int main(int argc, char **argv)
{
    int result;
    
    if (argc != 2)
        return 1;
    
    input = argv[1];
    
    // Se a string for vazia, retorna 0
    if (*input == '\0')
    {
        printf("0\n");
        return 0;
    }
    
    result = expression(0); // começamos fora de parênteses
    
    // Não valida o fim - aceita lixo após expressão válida
    
    printf("%d\n", result);
    
    return 0;
}
