#ifndef WEB_SERV_EXCEPTION
#define WEB_SERV_EXCEPTION

#include "../../includes/Headers.hpp"

class WebServException : public std::exception
{
private:
    std::string msg;

public:
    explicit WebServException(const std::string &value);
    virtual ~WebServException() throw();

    virtual const char *what() const throw();
};





//exceções derivadas específicas
class PortException : public WebServException
{
public:
    explicit PortException(const std::string &value);
};

class SocketException : public WebServException
{
public:
    explicit SocketException(const std::string &value);
};

class BindException : public WebServException
{
public:
    explicit BindException(const std::string &value);
};

class ListenException : public WebServException
{
public:
    explicit ListenException(const std::string &value);
};

class AcceptException : public WebServException
{
public:
    explicit AcceptException(const std::string &value);
};

class RequestException : public WebServException
{
public:
    explicit RequestException(const std::string &value);
};

class ResponseException : public WebServException
{
public:
    explicit ResponseException(const std::string &value);
};

#endif