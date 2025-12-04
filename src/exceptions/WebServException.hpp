#ifndef WEB_SERV_EXCEPTION
#define WEB_SERV_EXCEPTION

#include "../../includes/Headers.hpp"


class WebServException : public std::exception {
protected:
    std::string msg;

public:
    inline WebServException(const std::string &value) : msg(value) {}
    inline virtual ~WebServException() throw() {}
    inline virtual const char *what() const throw() { return msg.c_str(); }
};

//EXCEÇÕES ESPECÍFICAS
class PortException : public WebServException {
public:
    inline PortException(const std::string &value)
        : WebServException("PortException: " + value) {}
};

class SocketException : public WebServException {
public:
    inline SocketException(const std::string &value)
        : WebServException("SocketException: " + value) {}
};

#endif