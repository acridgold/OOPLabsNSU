#pragma once
#include <string>
#include <vector>


using namespace std;

struct ConverterConfig {
    string type;
    vector<string> args;
};

class ConfigParser {
public:
    explicit ConfigParser(const string& configFile);
    vector<ConverterConfig> Parse();
private:
    string configFile_;
};
