#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <list>
#include <vector>

#include "txtparser.h"

using namespace std;

txtparser::txtparser()
{
    std::ifstream infile("/home/ubuntu/Desktop/SNMP/cppsetdata2redis.txt");
    std::string key;
    std::string value;

    while (std::getline(infile, key))
    {
        miblist_.push_back(key);
        std::getline(infile, value);
        miblist_.push_back(value);
    }
}

list<string> txtparser::getData()
{
    return miblist_;
}
