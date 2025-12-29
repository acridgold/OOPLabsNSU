#include "ConfigParser.h"
#include <fstream>
#include <sstream>

using namespace std;

ConfigParser::ConfigParser(const string& configFile) : configFile_(configFile)
{
}

vector<ConverterConfig> ConfigParser::Parse()
{
    ifstream f(configFile_);
    if (!f) throw runtime_error("Cannot open config file");

    vector<ConverterConfig> steps;
    string line;

    while (getline(f, line))
    {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        istringstream ss(line);
        string word;
        ConverterConfig c;
        ss >> c.type;
        while (ss >> word) c.args.push_back(word);
        steps.push_back(c);
    }
    return steps;
}
