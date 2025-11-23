#pragma once
#include <string>
#include <vector>
#include <cstdint>

using namespace std;

class WavFile
{
public:
    // Чтение 16 bit, 1 channel, 44100 Hz
    explicit WavFile(const string& filename);

    // Создать из семплов
    explicit WavFile(const vector<int16_t>& samples);

    void save(const string& filename) const;
    const vector<int16_t>& samples() const { return samples_; }
    static bool IsSupportedFormat(const string& filename);

private:
    vector<int16_t> samples_;
    int sampleRate_ = 44100;
};
