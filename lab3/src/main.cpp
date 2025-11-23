#include "WavFile.h"
#include "ConfigParser.h"
#include "Converter.h"
#include <iostream>
#include <vector>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    if (argc < 5)
    {
        cerr << "Usage: .\\lab3.exe -c config.txt output.wav input1.wav [input2.wav ...]\n";
        return 1;
    }

    string configFile, outFile;
    vector<string> inFiles;
    for (int i = 1; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-h" || arg == "--help")
        {
            cout << "Usage: .\\lab3.exe -c config.txt output.wav input1.wav [input2.wav ...]\n";
            cout << "Converters:\n";
            for (const auto& kv : ConverterFactory::AllHelps())
                cout << kv.first << ": " << kv.second << "\n";
            return 0;
        }
        if (arg == "-c") configFile = argv[++i];
        else if (outFile.empty()) outFile = arg;
        else inFiles.push_back(arg);
    }
    if (configFile.empty() || outFile.empty() || inFiles.empty())
    {
        cerr << "Missing arguments\n";
        return 1;
    }

    vector<WavFile> wavs;
    for (const auto& file : inFiles)
    {
        if (!WavFile::IsSupportedFormat(file))
        {
            cerr << "Unsupported WAV format: " << file << endl;
            return 2;
        }
        wavs.emplace_back(file);
    }

    auto configs = ConfigParser(configFile).Parse();
    vector<int16_t> mainStream = wavs[0].samples();
    cout << "Samples loaded: " << mainStream.size() << endl;

    for (const auto& conf : configs)
    {
        try
        {
            auto conv = ConverterFactory::Create(conf.type, conf.args);
            vector<vector<int16_t>> extraStreams;

            // Собираем потоки ($)
            for (const auto& arg : conf.args)
                if (arg.size() > 1 && arg[0] == '$')
                {
                    int idx = stoi(arg.substr(1)) - 1;
                    if (idx >= wavs.size())
                    {
                        throw runtime_error(
                            "Extra stream $" + to_string(idx + 1) + " out of range");
                    }
                    extraStreams.push_back(wavs[idx].samples());
                }
            mainStream = conv->Process(mainStream, extraStreams);
        }

        catch (runtime_error& e)
        {
            cerr << "Config error in step '" << conf.type << "': " << e.what() << endl;
            return 3;
        }
    }

    WavFile(mainStream).save(outFile);
    cout << "Done. Output: " << outFile << endl;
    return 0;
}
