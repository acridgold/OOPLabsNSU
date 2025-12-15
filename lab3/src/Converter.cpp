#include "Converter.h"
#include <sstream>
#include <stdexcept>
#include <cmath>

using namespace std;

// --- MUTE ---
class MuteConverter : public Converter
{
    double t1_, t2_;

public:
    MuteConverter(double t1, double t2) : t1_(t1), t2_(t2) {}

    vector<int16_t> Process(const vector<int16_t>& input,
                            const vector<vector<int16_t>>&) override
    {
        vector<int16_t> out = input;
        int s = int(t1_ * 44100), e = int(t2_ * 44100);
        for (int i = s; i < e && i < (int)out.size(); ++i)
            out[i] = 0;
        return out;
    }

    string Help() const override
    {
        return "mute <start_sec> <end_sec> - Silence region [start,end] seconds.";
    }
};

// --- MIX ---
class MixConverter : public Converter
{
    int streamIdx_;
    int insertPos_;
    size_t processedSamples_;

public:
    MixConverter(int idx, int pos)
        : streamIdx_(idx), insertPos_(pos), processedSamples_(0)
    {
    }

    vector<int16_t> Process(const vector<int16_t>& input,
                            const vector<vector<int16_t>>& extra) override
    {
        vector<int16_t> out = input;
        if (streamIdx_ >= (int)extra.size())
        {
            processedSamples_ += input.size();
            return out;
        }

        const auto& ext = extra[streamIdx_];
        size_t insertSample = static_cast<size_t>(insertPos_) * 44100;

        for (size_t i = 0; i < out.size(); ++i)
        {
            size_t globalPos = processedSamples_ + i;
            if (globalPos < insertSample)
                continue;
            size_t j = globalPos - insertSample;
            int16_t extVal = (j < ext.size()) ? ext[j] : 0;
            int32_t sum = static_cast<int32_t>(out[i]) + static_cast<int32_t>(extVal);
            out[i] = int16_t(sum / 2);
        }

        processedSamples_ += input.size();
        return out;
    }

    string Help() const override
    {
        return "mix $n <insert_sec> - Mix extra stream at insert_sec, average samples.";
    }
};

// --- ECHO ---
class EchoConverter : public Converter
{
    double delay_, decay_;
    vector<int16_t> buffer_;
    size_t bufferPos_;

public:
    EchoConverter(double d, double c) : delay_(d), decay_(c), bufferPos_(0)
    {
        int bufSize = int(d * 44100) + 1;
        buffer_.resize(bufSize, 0);
    }

    vector<int16_t> Process(const vector<int16_t>& input,
                            const vector<vector<int16_t>>&) override
    {
        vector<int16_t> out = input;
        int dS = int(delay_ * 44100);

        for (size_t i = 0; i < out.size(); ++i)
        {
            int delayIdx = (bufferPos_ + buffer_.size() - dS) % buffer_.size();
            int val = int(out[i]) + int(buffer_[delayIdx] * decay_);
            out[i] = max(-32768, min(32767, val));
            buffer_[bufferPos_] = out[i];
            bufferPos_ = (bufferPos_ + 1) % buffer_.size();
        }

        return out;
    }

    string Help() const override
    {
        return "echo <delay_sec> <decay> - Feed back delayed signal (for echo effect).";
    }
};

// --- BASS_BOOST ---
class BassBoostConverter : public Converter
{
    double user_factor_;
    int window_;
    vector<int16_t> history_;

public:
    BassBoostConverter(double user_factor, int window = 32)
        : user_factor_(user_factor), window_(window)
    {
    }

    vector<int16_t> Process(const vector<int16_t>& input,
                            const vector<vector<int16_t>>&) override
    {
        vector<int16_t> out = input;
        double factor = user_factor_ / 10.0;
        double norm_window = double(window_) / 32.0;

        for (size_t i = 0; i < out.size(); ++i)
        {
            history_.push_back(input[i]);
            if ((int)history_.size() > window_)
                history_.erase(history_.begin());

            if ((int)history_.size() >= window_)
            {
                int64_t sum = 0;
                for (int j = 0; j < window_; ++j)
                    sum += history_[j];
                int64_t bass = sum / window_;
                int val = int(out[i]) + int(bass * factor * norm_window);
                out[i] = max(-32768, min(32767, val));
            }
        }

        return out;
    }

    string Help() const override
    {
        return "bass_boost <user_factor> <window> - Bass boost (scaled: user_factor/10).";
    }
};

// --- Фабрика ---
ConverterPtr ConverterFactory::Create(const string& name, const vector<string>& args)
{
    if (name == "mute")
    {
        if (args.size() < 2)
            throw runtime_error("mute needs <start_sec> <end_sec>");
        double t1 = stod(args[0]), t2 = stod(args[1]);
        return make_unique<MuteConverter>(t1, t2);
    }
    if (name == "mix")
    {
        if (args.size() < 2 || args[0][0] != '$')
            throw runtime_error("mix needs $n <insert_sec>");
        int idx = stoi(args[0].substr(1)) - 1;
        int pos = stoi(args[1]);
        return make_unique<MixConverter>(idx, pos);
    }
    if (name == "echo")
    {
        if (args.size() < 2)
            throw runtime_error("echo needs <delay_sec> <decay>");
        double d = stod(args[0]), c = stod(args[1]);
        return make_unique<EchoConverter>(d, c);
    }
    if (name == "bass_boost")
    {
        if (args.empty())
            throw runtime_error("bass_boost needs <factor>");
        double f = stod(args[0]);
        return make_unique<BassBoostConverter>(f);
    }

    throw runtime_error("Unknown converter: " + name);
}

map<string, string> ConverterFactory::AllHelps()
{
    return {
        {"mute", MuteConverter(0, 0).Help()},
        {"mix", MixConverter(0, 0).Help()},
        {"echo", EchoConverter(0, 0).Help()},
        {"bass_boost", BassBoostConverter(1).Help()}
    };
}
