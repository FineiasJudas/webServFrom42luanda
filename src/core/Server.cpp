#include "../../includes/Headers.hpp"

Server::Server(int port)
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        std::cerr << "Erro no socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Erro no bind\n";
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Erro no listen\n";
        return;
    }

    // Não-bloqueante
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

    poller.addFd(server_fd, EPOLLIN);
    std::cout << "Servidor ouvindo em 0.0.0.0:" << port << "\n";
}

Server::~Server() {
    for (std::map<int, Connection*>::iterator it = connections.begin(); it != connections.end(); ++it) {
        delete it->second;
    }
    close(server_fd);
}

void Server::handleNewConnection() {
    sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if (client_fd < 0) {
        return;  // EAGAIN esperado
    }

    Connection* conn = new Connection(client_fd);
    connections[client_fd] = conn;
    last_activity[client_fd] = time(NULL);
    poller.addFd(client_fd, EPOLLIN);
    std::cout << "Nova conexão aceita: fd=" << client_fd << "\n";
}

void Server::tryParseAndRespond(Connection* conn) {
    Buffer& in = conn->getInputBuffer();
    if (!in.contains("\r\n\r\n")) return;

    // Membro 2: parsing HTTP
    std::string request = in.toString();
    std::cout << "Requisição completa recebida:\n" << request << "\n";

    // Resposta HTTP simulada
    std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: 13\r\n"
        "\r\n"
        "Hello, World!";

    Buffer& out = conn->getOutputBuffer();
    out.append(response);

    // Ativa escrita
    poller.modifyFd(conn->getFd(), EPOLLIN | EPOLLOUT);
    in.clear();
}

void Server::sendFromOutputBuffer(Connection* conn) {
    Buffer& out = conn->getOutputBuffer();
    if (out.empty()) return;

    ssize_t sent = conn->write(out.data(), out.size());
    if (sent > 0) {
        out.consume(sent);
        last_activity[conn->getFd()] = time(NULL);
    }

    if (out.empty()) {
        poller.modifyFd(conn->getFd(), EPOLLIN);
    }
}

void Server::handleClientEvent(struct epoll_event& ev) {
    int fd = ev.data.fd;
    Connection* conn = connections[fd];

    if (ev.events & EPOLLIN) {
        char temp[1024];
        ssize_t bytes = conn->read(temp, sizeof(temp));

        if (bytes > 0) {
            conn->getInputBuffer().append(temp, bytes);
            last_activity[fd] = time(NULL);
            tryParseAndRespond(conn);
        } else if (bytes == 0) {
            std::cout << "Cliente fechou conexão: fd=" << fd << "\n";
            goto cleanup;
        } else {
            // EAGAIN → nada a fazer
        }
    }

    if (ev.events & EPOLLOUT) {
        sendFromOutputBuffer(conn);
    }

    return;

cleanup:
    poller.removeFd(fd);
    delete conn;
    connections.erase(fd);
    last_activity.erase(fd);
}

void Server::start() {
    while (true) {
        std::vector<struct epoll_event> events = poller.wait(1000);

        // Timeout: fecha inativos
        time_t now = time(NULL);
        for (std::map<int, time_t>::iterator it = last_activity.begin(); it != last_activity.end(); ) {
            if (now - it->second > 30) {
                std::cout << "Timeout: fd=" << it->first << "\n";
                Connection* conn = connections[it->first];
                poller.removeFd(it->first);
                delete conn;
                connections.erase(it->first);
                last_activity.erase(it++);
            } else {
                ++it;
            }
        }

        for (size_t i = 0; i < events.size(); ++i) {
            if (events[i].data.fd == server_fd) {
                handleNewConnection();
            } else {
                handleClientEvent(events[i]);
            }
        }
    }
}