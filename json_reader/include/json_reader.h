#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#define COMMA ','
#define QOUTATION '\"'
#define COLON ':'
#define LBRACK '['
#define RBRACK ']'
#define LBRACE '{'
#define RBRACE '}'

class Json_Reader{
public:
    static std::string get_json(const std::string& path);

    static std::string find_value(std::string json, const std::string& key);

    static std::vector<std::string> split_array(std::string array);
};