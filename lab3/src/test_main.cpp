#include <gtest/gtest.h>
#include "Converter.h"
#include <cmath>

class MuteConverterTest : public ::testing::Test {
protected:
    vector<int16_t> createTestSignal(int16_t value, size_t size) {
        return vector<int16_t>(size, value);
    }
};

TEST_F(MuteConverterTest, MutesCorrectRange) {
    auto conv = ConverterFactory::Create("mute", {"1.0", "2.0"});
    vector<int16_t> input = createTestSignal(100, 44100 * 3);
    vector<vector<int16_t>> extra;

    auto output = conv->Process(input, extra);

    EXPECT_EQ(output[0], 100);
    EXPECT_EQ(output[44100 * 1.0f], 0);
    EXPECT_EQ(output[44100 * 2.0f], 100);
}


class MixConverterTest : public ::testing::Test {
protected:
    vector<int16_t> createTestSignal(int16_t value, size_t size) {
        return vector<int16_t>(size, value);
    }
};

TEST_F(MixConverterTest, MixesAtCorrectPosition) {
    auto conv = ConverterFactory::Create("mix", {"$1", "1"});
    vector<int16_t> input = createTestSignal(100, 44100 * 3);
    vector<int16_t> extra = createTestSignal(200, 44100 * 3);
    vector<vector<int16_t>> extras = {extra};

    auto output = conv->Process(input, extras);

    EXPECT_EQ(output[0], 100);
    int16_t expected = (100 + 200) / 2;
    EXPECT_EQ(output[44100], expected);
}

TEST_F(MixConverterTest, MixesFromStart) {
    auto conv = ConverterFactory::Create("mix", {"$1", "0"});
    vector<int16_t> input = createTestSignal(100, 44100);
    vector<int16_t> extra = createTestSignal(200, 44100);
    vector<vector<int16_t>> extras = {extra};

    auto output = conv->Process(input, extras);

    int16_t expected = (100 + 200) / 2;
    EXPECT_EQ(output[0], expected);
    EXPECT_EQ(output[44100 - 1], expected);
}

TEST_F(MixConverterTest, HandlesChunkedProcessing) {
    auto conv = ConverterFactory::Create("mix", {"$1", "0"});
    vector<int16_t> input1(44100, 100);
    vector<int16_t> input2(44100, 100);
    vector<int16_t> extra(88200, 200);
    vector<vector<int16_t>> extras = {extra};

    auto output1 = conv->Process(input1, extras);
    auto output2 = conv->Process(input2, extras);

    int16_t expected = (100 + 200) / 2;
    EXPECT_EQ(output1[0], expected);
    EXPECT_EQ(output2[0], expected);
}

TEST_F(MixConverterTest, HandlesInvalidStreamIndex) {
    auto conv = ConverterFactory::Create("mix", {"$5", "0"});
    vector<int16_t> input = createTestSignal(100, 44100);
    vector<vector<int16_t>> extras;

    auto output = conv->Process(input, extras);

    EXPECT_EQ(output[0], 100);
}


class EchoConverterTest : public ::testing::Test {
protected:
    vector<int16_t> createImpulse(size_t size) {
        vector<int16_t> v(size, 0);
        if (size > 0) v[0] = 1000;
        return v;
    }
};

TEST_F(EchoConverterTest, ProducesEcho) {
    auto conv = ConverterFactory::Create("echo", {"0.5", "0.5"});
    vector<int16_t> input = createImpulse(44100);
    vector<vector<int16_t>> extra;

    auto output = conv->Process(input, extra);

    EXPECT_EQ(output[0], 1000);
    int delayIdx = 44100 / 2;
    if (delayIdx < (int)output.size()) {
        EXPECT_NE(output[delayIdx], 0);
    }
}

TEST_F(EchoConverterTest, DecayReducesSignal) {
    auto conv1 = ConverterFactory::Create("echo", {"0.1", "1.0"});
    auto conv2 = ConverterFactory::Create("echo", {"0.1", "0.5"});

    vector<int16_t> input = createImpulse(44100);
    vector<vector<int16_t>> extra;

    auto output1 = conv1->Process(input, extra);
    auto output2 = conv2->Process(input, extra);

    int delayIdx = 44100 / 10;
    if (delayIdx < (int)output1.size() && delayIdx < (int)output2.size()) {
        EXPECT_LT(abs(output2[delayIdx]), abs(output1[delayIdx]));
    }
}



class BassBoostConverterTest : public ::testing::Test {
protected:
    vector<int16_t> createLowFreqSignal(size_t size, int16_t amplitude) {
        vector<int16_t> v(size);
        for (size_t i = 0; i < size; ++i) {
            v[i] = (i / 100) % 2 == 0 ? amplitude : -amplitude;
        }
        return v;
    }
};

TEST_F(BassBoostConverterTest, BoostsLowFrequency) {
    auto conv = ConverterFactory::Create("bass_boost", {"5"});
    vector<int16_t> input = createLowFreqSignal(1000, 100);
    vector<vector<int16_t>> extra;

    auto output = conv->Process(input, extra);

    EXPECT_EQ(output.size(), input.size());
    bool boosted = false;
    for (size_t i = 100; i < output.size(); ++i) {
        if (abs(output[i]) > abs(input[i])) {
            boosted = true;
            break;
        }
    }
    EXPECT_TRUE(boosted);
}

TEST_F(BassBoostConverterTest, HighFactorBoostsMore) {
    auto conv1 = ConverterFactory::Create("bass_boost", {"10"});
    auto conv2 = ConverterFactory::Create("bass_boost", {"5"});

    vector<int16_t> input = createLowFreqSignal(1000, 100);
    vector<vector<int16_t>> extra;

    auto output1 = conv1->Process(input, extra);
    auto output2 = conv2->Process(input, extra);

    int64_t sum1 = 0, sum2 = 0;
    for (size_t i = 100; i < output1.size(); ++i) {
        sum1 += abs(output1[i]);
        sum2 += abs(output2[i]);
    }

    EXPECT_GT(sum1, sum2);
}



class ConverterFactoryTest : public ::testing::Test {};

TEST_F(ConverterFactoryTest, CreatesValidConverters) {
    EXPECT_NO_THROW(ConverterFactory::Create("mute", {"0", "1"}));
    EXPECT_NO_THROW(ConverterFactory::Create("mix", {"$1", "0"}));
    EXPECT_NO_THROW(ConverterFactory::Create("echo", {"0.1", "0.5"}));
    EXPECT_NO_THROW(ConverterFactory::Create("bass_boost", {"5"}));
}


