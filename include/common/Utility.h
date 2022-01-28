#ifndef __UTILITY_H__
#define __UTILITY_H__
#include <string>
#include <vector>

#include <locale>
#include <codecvt>

namespace Util
{
    inline std::string ToByteString(const std::wstring& input)
    {
        //std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        return converter.to_bytes(input);
    }

    // split a string by token
    void Split(std::string& in, std::vector<std::string>& out, char token)
    {
        out.clear();
        int i = 0, last = 0;
        for (; i < in.size(); i++)
        {
            if (in[i] == token)
            {
                out.emplace_back(in.substr(last, i - last));
                last = i + 1;
            }
        }
        out.emplace_back(in.substr(last, in.size() - last));
    }

    std::vector<std::string> Filter(std::vector<std::string>& in, std::string&& target)
    {
        std::vector<std::string> out;
        for (auto& str: in)
        {
            if (str.compare(target) != 0)
            {
                out.emplace_back(str);
            }
        }
        return out;
    }
}
#endif