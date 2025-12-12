#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>

using namespace std;

class Converter
{
public:
    virtual vector<int16_t> Process(const vector<int16_t>& input,
                                    const vector<vector<int16_t>>& extra) = 0;

    virtual vector<int16_t> ProcessBlock(const vector<int16_t>& input,
                                         const vector<vector<int16_t>>& extra)
    {
        return Process(input, extra);
    }
    virtual string Help() const = 0;
    virtual ~Converter() = default;
};

using ConverterPtr = unique_ptr<Converter>;



class ConverterFactory
{
public:
    static ConverterPtr Create(const string& name,
                               const vector<string>& args);
    static map<string, string> AllHelps();
};
