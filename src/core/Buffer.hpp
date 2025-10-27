#ifndef BUFFER_HPP
#define BUFFER_HPP

#include "../../includes/Headers.hpp"

class   Buffer
{
    private:
        std::vector<char> data; // Dados armazenados

    public:
        Buffer();
        void    append(const char* data, size_t size); // Adiciona dados ao buffer
        const std::vector<char> &getData() const;   // Obtém dados do buffer
        void    clear();                              // Limpa o buffer
};

#endif