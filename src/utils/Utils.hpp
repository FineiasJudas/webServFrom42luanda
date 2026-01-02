#ifndef UTILS_HPP
#define UTILS_HPP

#include "../../includes/Headers.hpp"
#include "../http/Response.hpp"
class Utils
{
public:
    static std::string toString(size_t n)
    {
        std::ostringstream ss;
        ss << n;
        return (ss.str());
    }
    static bool isNumber(const std::string &s)
    {
        if (s.empty())
            return (false);

        for (size_t i = 0; i < s.size(); ++i)
            if (!std::isdigit(s[i]))
                return (false);

        return (true);
    }
    
    static std::string getExtension(const std::string &path)
    {
        size_t pos = path.rfind('.');

        if (pos == std::string::npos)
            return "";
        return path.substr(pos);
    }
    static std::vector<std::string> split(const std::string &s, char delimiter)
    {
        std::vector<std::string> result;
        std::string item;

        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == delimiter)
            {
                result.push_back(item);
                item.clear();
            }
            else
            {
                item += s[i];
            }
        }

        // último item
        result.push_back(item);

        return result;
    }

    static Response makeErrorResponse(int status,
                                    const std::string &body,
                                    const std::string &contentType = "text/html")
    {
        Response res;
        res.status = status;
        res.body = body;
        res.headers["Content-Type"] = contentType;
        res.headers["Content-Length"] = Utils::toString(res.body.size());
        return res;
    }

    static std::string getInterpreterCGI(std::string path, std::string extension)
    {
        if (path.empty())
        {
            if (extension == ".php") {return ("/usr/bin/php-cgi");}
            else if (extension == ".py") {return ("/usr/bin/python3");}
            else {return "";}
        }
        else {return (path);}
    }

};

#endif
