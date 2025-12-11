#ifndef UTILS_HPP
#define UTILS_HPP

#include "../../includes/Headers.hpp"

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
            return false;

        for (size_t i = 0; i < s.size(); ++i)
            if (!std::isdigit(s[i]))
                return false;

        return true;
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
};

#endif
