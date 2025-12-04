#ifndef UTILS_HPP
#define UTILS_HPP

#include "../../includes/Headers.hpp"

class   Utils
{
    public:
        static std::string  toString(size_t n)
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

};

#endif

