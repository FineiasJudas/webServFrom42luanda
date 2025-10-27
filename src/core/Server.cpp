#include "../../includes/Headers.hpp"

void countPrint()
{
    int count = 10;
    while (count > 0)
    {
        usleep(500000);
        std::cout << "Respondendo cliente em: [" << count << "]" << std::endl;
        count --;
    }
    
}

Server::Server(const std::string &host, int port)
    : server_fd(-1), port(port), host(host)
{
    server_fd = createSocket();
    poller.addFd(server_fd, EPOLLIN);// Monitora socket do servidor para novas conexões
}

Server::~Server()
{
    if (server_fd != -1)
        close(server_fd);
}

int Server::createSocket()
{
    int     fd;
    sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return std::cerr << "Erro ao criar socket: " << strerror(errno) << std::endl, (-1);

    setNonBlocking(fd);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return std::cerr << "Erro no bind: " << strerror(errno) << std::endl, (-1);

    if (listen(fd, 10) < 0)
        return std::cerr << "Erro no listen: " << strerror(errno) << std::endl, (-1);

    std::cout << "Servidor ouvindo em " << host << ":" << port << std::endl;

    return (fd);
}

void    Server::setNonBlocking(int sockfd)
{
    int     flags;

    flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1)
    {
        std::cerr << "Erro em fcntl F_GETFL: " << strerror(errno) << std::endl;
        return ;
    }
    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1)
    {
        std::cerr << "Erro em fcntl F_SETFL: " << strerror(errno) << std::endl;
        return ;
    }
}

void    Server::handleNewConnection()
{
    int     client_fd;
    sockaddr_in     client_addr;
    socklen_t       addr_len;

    addr_len = sizeof(client_addr);
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

    if (client_fd < 0)
    {
        std::cerr << "Erro no accept: " << strerror(errno) << std::endl;
        return ;
    }

    setNonBlocking(client_fd);
    poller.addFd(client_fd, EPOLLIN); // Monitora cliente para leitura
    std::cout << "Nova conexão aceita do clinete: " << client_addr.sin_addr.s_addr << std::endl;
    countPrint();
    send(client_fd, "Bem-vindo ao WebServ!\n", 24, 0);
    close(client_fd);
}

void    Server::handleClientData(Connection &conn)
{
    ssize_t     bytes;
    char    buffer[1024];

    bytes = conn.read(buffer, sizeof(buffer));
    if (bytes <= 0)
    {
        if (bytes == 0 || errno == ECONNRESET)
        {
            std::cout << "Conexão fechada: fd=" << conn.getFd() << std::endl;
            poller.removeFd(conn.getFd());
            conn.close();
        }
        return ;
    }
    // Ecoa os dados recebidos (exemplo simples)
    conn.write(buffer, bytes);
}

void    Server::start()
{
    while (true)
    {
        std::vector<struct epoll_event> events = poller.wait(1000); // Timeout de 1 segundo
        for (std::vector<struct epoll_event>::iterator it = events.begin(); it != events.end(); ++it)
        {
            if (it->data.fd == server_fd)
                handleNewConnection();
            else
            {
                Connection conn(it->data.fd);
                handleClientData(conn);
            }
        }
    }
}
