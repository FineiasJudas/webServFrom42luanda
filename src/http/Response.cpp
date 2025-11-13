#include "Response.hpp"
#include <sstream>

Response::Response() : status(200), reason("OK") {}

void    Response::setHeader(const std::string &key, const std::string &value)
{
    headers[key] = value;
}

void    Response::setStatus(int code, const std::string &msg)
{
    status = code;
    reason = msg;
}

// ------------------------------------------------------------
// 1️⃣ Mapeia códigos HTTP para mensagens
// ------------------------------------------------------------
std::string Response::reasonPhrase(int code)
{
    switch (code)
    {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 413: return "Payload Too Large";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        default:  return "Unknown";
    }
}

// ------------------------------------------------------------
// 2️⃣ Constrói o texto completo da resposta HTTP
// ------------------------------------------------------------
std::string Response::toString() const
{
    std::ostringstream  oss;
    oss << "HTTP/1.1 " << status << " " << reasonPhrase(status) << "\r\n";

    // adiciona todos os headers
    for (std::map<std::string, std::string>::const_iterator it = headers.begin();
         it != headers.end(); ++it)
        oss << it->first << ": " << it->second << "\r\n";

    oss << "\r\n"; // separa headers do body

    // adiciona o corpo (se houver)
    oss << body;

    return oss.str();
}
