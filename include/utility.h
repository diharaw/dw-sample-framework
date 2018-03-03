#pragma once

#include <string>
#include <vector>
#include <sstream>
#include <cassert>

using String = std::string;
using StringList = std::vector<std::string>;
using PositionList = std::vector<size_t>;

namespace Utility
{
	extern bool ReadText(std::string path, std::string& out);
    
    inline int find(std::string _keyword, std::string _source, int _startIndex = -1)
    {
        std::string::size_type n;
        
        if (_startIndex == -1)
            n = _source.find(_keyword);
        else
            n = _source.find(_keyword, _startIndex);
        
        if (n == std::string::npos)
            return -1;
        else
            return n;
    }
    
    inline StringList find_line(std::string _keyword, std::string _source)
    {
        StringList lineList;
        std::string line;
        std::istringstream sourceStream(_source);
        
        while (std::getline(sourceStream, line))
        {
            int col = find(_keyword, line);
            
            if (col != -1)
            {
                lineList.push_back(line);
            }
        }
        
        return lineList;
    }
    
    inline StringList delimit(std::string _delimiter, std::string _source)
    {
        StringList list;
        std::string token;
        std::string::size_type n = 0;
        
        while ((n = _source.find(_delimiter)) != std::string::npos)
        {
            token = _source.substr(0, n);
            list.push_back(token);
            _source.erase(0, n + _delimiter.length());
        }
        
        list.push_back(_source);
        
        return list;
    }
    
    inline std::string substring_by_ifdef(std::string _define, std::string _source)
    {
        std::string substring;
        std::string defineString = "#ifdef";
        defineString += " ";
        defineString += _define;
        
        int startIndex = find(defineString, _source);
        int endIndex;
        
        if (startIndex != -1)
        {
            endIndex = find("#endif", _source, startIndex + defineString.length());
            substring = _source.substr(startIndex + defineString.length(), endIndex - (startIndex + defineString.length()));
            
            return substring;
        }
        
        return substring;
    }
    
    inline void replace_substring(std::string _sourceSubstring, std::string _destSubstring, std::string& _source)
    {
        int line = find(_sourceSubstring, _source);
        _source.replace(line, _sourceSubstring.length(), _destSubstring);
    }
    
    inline StringList Split(String str, char delimiter)
    {
        StringList internal;
        std::stringstream ss(str);
        String tok;
        
        while (getline(ss, tok, delimiter))
        {
            internal.push_back(tok);
        }
        
        return internal;
    }
    
    inline String remove_special_char(String str)
    {
        if (!str.empty() && str[str.size() - 1] == '\r')
            str.erase(str.size() - 1);
        
        return str;
    }
    
    inline bool DefineExists(String name, StringList defines)
    {
        String clean_define = remove_special_char(name);
        
        for (auto define : defines)
        {
            if (clean_define == define)
                return true;
        }
        
        return false;
    }
    
    inline PositionList FindAllSubstringPositions(String source, String substring)
    {
        PositionList positions;
        
        size_t pos = source.find(substring, 0);
        while (pos != std::string::npos)
        {
            positions.push_back(pos);
            pos = source.find(substring, pos + 1);
        }
        
        return positions;
    }
    
    inline String GenerateSource(String source, StringList defines)
    {
        String result = "";
        String remaining = source;
        
        while (true)
        {
            bool is_ifndef = false;
            size_t pos = remaining.find("#ifdef ");
            size_t ndef_pos = remaining.find("#ifndef ");
            
            if (ndef_pos < pos)
            {
                is_ifndef = true;
                pos = ndef_pos;
            }
            
            if (pos != std::string::npos)
            {
                if (pos != 0)
                    result += remaining.substr(0, pos - 1);
                
                remaining = remaining.substr(pos, remaining.size() - pos);
                
                size_t new_line_pos = remaining.find_first_of('\n');
                String line = remaining.substr(0, new_line_pos);
                
                StringList tokens = Split(line, ' ');
                
                size_t else_pos = remaining.find("#else");
                size_t endif_pos = remaining.find("#endif");
                bool has_else = false;
                
                // Assert to ensure this is a valid source file.
                assert(endif_pos != std::string::npos);
                // Find if 'else' exists
                if (else_pos != std::string::npos && else_pos < endif_pos)
                {
                    has_else = true;
                }
                
                bool cond;
                
                if (is_ifndef)
                    cond = !DefineExists(tokens[1], defines);
                else
                    cond = DefineExists(tokens[1], defines);
                
                if (cond)
                {
                    if (has_else)
                    {
                        result += remaining.substr(new_line_pos + 1, (else_pos - 1) - (new_line_pos + 1));
                    }
                    else
                    {
                        result += remaining.substr(new_line_pos + 1, endif_pos - (new_line_pos + 1));
                    }
                }
                else
                {
                    if (has_else)
                    {
                        result += remaining.substr(else_pos + 5, (endif_pos - 1) - (else_pos + 5));
                    }
                }
                
                remaining = remaining.substr(endif_pos + 6, remaining.size() - (endif_pos + 6));
            }
            else
            {
                result += remaining;
                break;
            }
        }
        
        return result;
    }
}

