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

    inline
    std::string trim(const std::string& str) {
        std::string trimmedStr = str;
        
        // Find the index of the first non-whitespace character
        size_t startPos = trimmedStr.find_first_not_of(" \t\r\n");
        if (startPos == std::string::npos) {
            // The string only contains whitespace characters
            return "";
        }
        
        // Find the index of the last non-whitespace character
        size_t endPos = trimmedStr.find_last_not_of(" \t\r\n");
        
        // Copy the substring from the first to last non-whitespace character
        trimmedStr = trimmedStr.substr(startPos, endPos - startPos + 1);
        
        return trimmedStr;
    }

}

#endif // !_STR_UTILS_H