#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include "../utils/Utils.hpp"

struct Response
{
    int status;
    std::string reason;
    std::map<std::string, std::string> headers;
    std::string body;

    Response();

    // Define cabeçalho simples
    void setHeader(const std::string &key, const std::string &value);

    // Define status e reason
    void setStatus(int code, const std::string &msg);

    // Gera o texto completo HTTP (para enviar ao socket)
    std::string toString() const;

    private:
        static std::string reasonPhrase(int code);
};

#endif
