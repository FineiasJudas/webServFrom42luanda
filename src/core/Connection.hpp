#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../../includes/Headers.hpp"
#include "Buffer.hpp"

class   Connection
{
    private:
        int     fd; // File descriptor da conexão
        Buffer  buffer; // Buffer para leitura/escrita

    public:
        Connection(int fd);
        ~Connection();
        int     getFd() const;
        ssize_t     read(char* buffer, size_t size); // Lê dados do cliente
        ssize_t     write(const char* buffer, size_t size); // Escreve dados para o cliente
        void    close(); // Fecha a conexão

};

#endif