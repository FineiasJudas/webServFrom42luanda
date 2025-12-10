#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include "../../includes/Headers.hpp"
#include "../config/Config.hpp"
#include "../utils/Utils.hpp"

struct  Response
{
    int     status;
    std::map<std::string, std::string> headers;
    std::string body;
    
    Response() : status(200) {}

    static std::string reasonPhrase(int code)
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
            default:  return "Unknown";
        }
    }

    std::string toString() const
    {
        std::ostringstream  oss;

        oss << "HTTP/1.1 " << Utils::toString(status) << " " << reasonPhrase(status) << "\r\n";
        for (std::map<std::string,std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
            oss << it->first << ": " << it->second << "\r\n";
        oss << "\r\n";
        oss << body;
        return oss.str();
    }
};

Response    methodGet(const ServerConfig &config,
                           const LocationConfig &loc,
                           const std::string &path,
                           const std::string &uri);

Response   methodPost(const Request &req,
    const ServerConfig &config, const std::string &path);

Response    methodDelete(const std::string &path, const ServerConfig &config);

Response    notAloweMethodResponse(const ServerConfig &config);

Response forbiddenPageResponse(const ServerConfig &config);

Response methodPostMultipart(const Request &req,
                             const std::string &uploadDir);

std::string  readFile(const std::string &path);
bool    fileExists(const std::string &path);
bool    dirExists(const std::string &path);

#endif
