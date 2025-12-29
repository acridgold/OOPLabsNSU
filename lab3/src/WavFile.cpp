#include "WavFile.h"
#include <fstream>
#include <stdexcept>
#include <cstring>

using namespace std;

WavFile::WavFile(const string& filename)
{
    filename_ = filename;
    ifstream f(filename, ios::binary);
    if (!f) throw runtime_error("Cannot open WAV file");

    char header[12];
    f.read(header, 12);
    if (strncmp(header, "RIFF", 4) || strncmp(header + 8, "WAVE", 4))
        throw runtime_error("Not a valid RIFF WAVE file");

    // Читаем subchunks до data
    bool fmtFound = false, dataFound = false;
    uint16_t audioFmt = 0, numCh = 0, bitsPerSample = 0;
    int32_t sampleRate = 0;
    while (f && (!fmtFound || !dataFound))
    {
        char subchunkId[4];
        uint32_t subchunkSize;

        f.read(subchunkId, 4);
        f.read(reinterpret_cast<char*>(&subchunkSize), 4);

        if (strncmp(subchunkId, "fmt ", 4) == 0)
        {
            fmtFound = true;
            f.read(reinterpret_cast<char*>(&audioFmt), 2);
            f.read(reinterpret_cast<char*>(&numCh), 2);
            f.read(reinterpret_cast<char*>(&sampleRate), 4);

            f.seekg(6, ios::cur); // byte rate (4) + block align (2)

            f.read(reinterpret_cast<char*>(&bitsPerSample), 2);
            f.seekg(subchunkSize - 16, ios::cur); // теорема скипа fmt
        }
        else if (strncmp(subchunkId, "data", 4) == 0)
        {
            dataFound = true;
            if (!fmtFound) throw runtime_error("fmt chunk not found before data");
            if (audioFmt != 1 || numCh != 1 || bitsPerSample != 16 || sampleRate != 44100)
                throw runtime_error("Unsupported WAV format");
            samples_.resize(subchunkSize / 2);
            dataOffset_ = static_cast<size_t>(f.tellg());
            numSamples_ = subchunkSize / 2;
            f.read(reinterpret_cast<char*>(samples_.data()), subchunkSize);
        }
        else
        {
            f.seekg(subchunkSize, ios::cur); // теорема скипа чанка
        }
    }
    if (!dataFound || samples_.empty())
        throw runtime_error("No audio data found or empty data chunk");
}

WavFile::WavFile(const vector<int16_t>& samples) : samples_(samples)
{
}

void WavFile::save(const string& filename) const
{
    ofstream f(filename, ios::binary);
    int numSamples = samples_.size();
    int byteRate = sampleRate_ * 2;
    int dataSz = numSamples * 2;
    int chunkSz = 36 + dataSz;

    // RIFF header
    f.write("RIFF", 4);
    f.write(reinterpret_cast<const char*>(&chunkSz), 4);
    f.write("WAVE", 4);

    // fmt subchunk
    f.write("fmt ", 4);
    int subChunk1Sz = 16;
    f.write(reinterpret_cast<const char*>(&subChunk1Sz), 4);

    int16_t audioFmt = 1, numCh = 1;
    f.write(reinterpret_cast<const char*>(&audioFmt), 2);
    f.write(reinterpret_cast<const char*>(&numCh), 2);
    f.write(reinterpret_cast<const char*>(&sampleRate_), 4);
    f.write(reinterpret_cast<const char*>(&byteRate), 4);

    int16_t blockAlign = 2, bitsPerSample = 16;
    f.write(reinterpret_cast<const char*>(&blockAlign), 2);
    f.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    // data subchunk
    f.write("data", 4);
    f.write(reinterpret_cast<const char*>(&dataSz), 4);
    f.write(reinterpret_cast<const char*>(samples_.data()), dataSz);
}

bool WavFile::IsSupportedFormat(const string& filename)
{
    try
    {
        WavFile file(filename);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

size_t WavFile::sampleCount() const
{
    if (!samples_.empty()) return samples_.size();
    return numSamples_;
}

vector<int16_t> WavFile::readBlock(size_t frstSampleIndex, size_t count) const
{
    // Если места нет
    if (!samples_.empty())
    {
        vector<int16_t> out;
        if (frstSampleIndex >= samples_.size()) return out;
        size_t avail = min(count, samples_.size() - frstSampleIndex);
        out.insert(out.end(), samples_.begin() + frstSampleIndex, samples_.begin() + frstSampleIndex + avail);
        return out;
    }

    // Иначе из файла
    if (frstSampleIndex >= numSamples_) return {};
    size_t avail = min(count, numSamples_ - frstSampleIndex);
    vector<int16_t> out(avail);

    ifstream f(filename_, ios::binary);
    if (!f) throw runtime_error("Cannot open WAV file for block read");
    f.seekg(dataOffset_ + frstSampleIndex * 2, ios::beg);
    f.read(reinterpret_cast<char*>(out.data()), avail * 2);

    size_t readBytes = f.gcount();
    if (readBytes < avail * 2)
    {
        out.resize(readBytes / 2);
    }

    return out;
}
