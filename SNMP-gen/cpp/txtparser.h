#include <sstream>
#include <fstream>
#include <list>

#include <iostream>

using namespace std;


class txtparser
{
    public:
    txtparser();
    list<string> getData();

    private:
    list<string> miblist_;

};