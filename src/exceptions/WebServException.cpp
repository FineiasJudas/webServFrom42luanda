#include "WebServException.hpp"

WebServException::WebServException(const std::string &value) : msg(value) {}
WebServException::~WebServException() throw() {}
const char *WebServException::what() const throw()
{
    return msg.c_str();
}


PortException::PortException(const std::string &value)
    : WebServException("PortException: " + value) {}

SocketException::SocketException(const std::string &value)
    : WebServException("SocketException: " + value) {}

BindException::BindException(const std::string &value)
    : WebServException("BindException: " + value) {}

ListenException::ListenException(const std::string &value)
    : WebServException("ListenException: " + value) {}

AcceptException::AcceptException(const std::string &value)
    : WebServException("AcceptException: " + value) {}

RequestException::RequestException(const std::string &value)
    : WebServException("RequestException: " + value) {}

ResponseException::ResponseException(const std::string &value)
    : WebServException("ResponseException: " + value) {}