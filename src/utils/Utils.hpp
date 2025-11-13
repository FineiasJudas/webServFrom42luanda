#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <sstream>

class Utils
{
    public:
        static std::string toString(size_t n)
        {
            std::ostringstream ss;
            ss << n;
            return ss.str();
        }
};

#endif
