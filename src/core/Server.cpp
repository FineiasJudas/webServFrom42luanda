#include "../../includes/Headers.hpp"

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
    last_activity[client_fd] = time(0);
    std::cout << "Nova conexão aceita do clinete: " << client_addr.sin_addr.s_addr << std::endl;

    send(client_fd, "Bem-vindo ao WebServ!\n", 24, 0);
    char buffer[1024];

    ssize_t bytes_recvd = recv(client_fd, buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (bytes_recvd > 0)
    {
        buffer[bytes_recvd] = '\0';
        std::cout << "Dados recebidos do cliente: " << buffer << std::endl;
    }

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
            last_activity.erase(conn.getFd());
            conn.close();
        }
        return ;
    }
    last_activity[conn.getFd()] = time(0);
    // Ecoa os dados recebidos (exemplo simples)
    conn.write(buffer, bytes);
}

void Server::start()
{
    while (true)
    {
        std::vector<struct epoll_event> events = poller.wait(1000);
        time_t      now = time(0);
        for (std::map<int, time_t>::iterator it = last_activity.begin(); it != last_activity.end();)
        {
            if (now - it->second > 30)
            {
                std::cout << "Timeout: fechando fd=" << it->first << std::endl;
                Connection conn(it->first);
                poller.removeFd(it->first);
                conn.close();
                last_activity.erase(it++);
            }
            else
                ++it;
        }

        // Processa eventos
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
