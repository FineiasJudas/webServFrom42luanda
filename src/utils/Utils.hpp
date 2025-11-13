#ifndef UTILS_HPP
# define UTILS_HPP

#include <sstream>
#include <string>

namespace Utils
{
    template <typename T>
    std::string toString(T value) {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    }
}

#endif
