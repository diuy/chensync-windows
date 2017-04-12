#pragma once
#include <string>
std::string nowTimeStr();
std::string nowDateStr();

#define COUT cout<<nowTimeStr()<<": "
#define CERR cerr<<nowTimeStr()<<": "