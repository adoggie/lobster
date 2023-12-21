#ifndef _STR_UTILS_H
#define _STR_UTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>

namespace lobutils {

    inline
    std::vector<std::string> splitString(const std::string& input, char delimiter) {
        std::vector<std::string> result;
        std::stringstream ss(input);
        std::string item;

        while (std::getline(ss, item, delimiter)) {
            result.push_back(item);
        }

        return result;
    }

}

#endif // !_STR_UTILS_H