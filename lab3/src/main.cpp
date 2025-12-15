#include "WavFile.h"
#include "ConfigParser.h"
#include "Converter.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

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
        if (arg == "-c")
        {
            if (i + 1 >= argc)
            {
                cerr << "Missing config file after -c\n";
                return 1;
            }
            configFile = argv[++i];
        }
        else if (outFile.empty())
        {
            outFile = arg;
        }
        else
        {
            inFiles.push_back(arg);
        }
    }

    if (configFile.empty() || outFile.empty() || inFiles.empty())
    {
        cerr << "Missing arguments\n";
        return 1;
    }

    // --- загрузка WAV-файлов ---
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

    // --- разбор конфига ---
    auto configs = ConfigParser(configFile).Parse();
    const size_t blockSize = 4096;
    size_t totalSamples = wavs[0].sampleCount();

    cout << "Total samples: " << totalSamples << "\n";

    // --- применение конвертеров ---
    for (const auto& conf : configs)
    {
        try
        {
            auto conv = ConverterFactory::Create(conf.type, conf.args);

            // Определение индексов дополнительных потоков ($1, $2, ...)
            vector<int> extraId;
            for (const auto& arg : conf.args)
            {
                if (arg.size() > 1 && arg[0] == '$')
                {
                    int idx = stoi(arg.substr(1)) - 1;
                    if (idx >= static_cast<int>(wavs.size()) || idx < 0)
                        throw runtime_error("Extra stream $" + to_string(idx + 1) + " out of range");
                    extraId.push_back(idx);
                }
            }

            vector<int16_t> transformedAll;
            transformedAll.reserve(totalSamples);

            // Блочная обработка
            for (size_t off = 0; off < totalSamples; off += blockSize)
            {
                size_t cnt = min(blockSize, totalSamples - off);
                auto mainBlock = wavs[0].readBlock(off, cnt);

                vector<vector<int16_t>> extraBlocks;
                extraBlocks.reserve(extraId.size());
                for (int idx : extraId)
                    extraBlocks.push_back(wavs[idx].readBlock(off, cnt));

                auto outBlock = conv->Process(mainBlock, extraBlocks);

                if (outBlock.size() != mainBlock.size())
                    outBlock.resize(mainBlock.size());

                transformedAll.insert(transformedAll.end(), outBlock.begin(), outBlock.end());
            }

            wavs[0] = WavFile(transformedAll); // обновляем основной поток
            cout << "Applied converter: " << conf.type << "\n";
        }
        catch (const runtime_error& e)
        {
            cerr << "Config error in step '" << conf.type << "': " << e.what() << endl;
            return 3;
        }
    }

    // --- сохранение ---
    WavFile(wavs[0].samples()).save(outFile);
    cout << "Done. Output: " << outFile << endl;
    return 0;
}
