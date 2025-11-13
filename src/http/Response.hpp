#ifndef RESPONSE_HPP
#define RESPONSE_HPP

#include <string>
#include <map>
#include "../utils/Utils.hpp"

struct Response {
    int status;
    std::map<std::string, std::string> headers;
    std::string body;

    std::string statusMessage(int code) const {
        switch (code) {
            case 200: return "OK";
            case 204: return "No Content";
            case 404: return "Not Found";
            case 405: return "Method Not Allowed";
            default:  return "Internal Server Error";
        }
    }

    std::string toString() const {
        std::ostringstream ss;
        ss << "HTTP/1.1 " << Utils::toString(status) << " " << statusMessage(status) << "\r\n";
        for (std::map<std::string, std::string>::const_iterator it = headers.begin(); it != headers.end(); ++it)
            ss << it->first << ": " << it->second << "\r\n";
        ss << "\r\n" << body;
        return ss.str();
    }
};

#endif
