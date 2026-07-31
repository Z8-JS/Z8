#ifndef ZANE_URLDECODE_H
#define ZANE_URLDECODE_H

#include <string>
#include <cctype>

inline int from_hex(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
    return -1;
}

inline std::string url_decode(const std::string& text) {
    std::string result;
    result.reserve(text.length());
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '+') {
            result += ' ';
        } else if (text[i] == '%' && i + 2 < text.length()) {
            int hi = from_hex(text[i + 1]);
            int lo = from_hex(text[i + 2]);
            if (hi != -1 && lo != -1) {
                result += static_cast<char>((hi << 4) | lo);
                i += 2;
            } else {
                result += '%';
            }
        } else {
            result += text[i];
        }
    }
    return result;
}

#endif // ZANE_URLDECODE_H
