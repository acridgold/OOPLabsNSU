#pragma once
#include <gtest/gtest.h>
#include "CSVParser.h"
#include <sstream>
#include <fstream>
#include <cstdio>

class TuplePrintTest : public ::testing::Test {};

TEST_F(TuplePrintTest, PrintsSimpleTuple) {
    auto t = make_tuple(1, "hello");
    stringstream ss;
    ss << t;
    EXPECT_EQ(ss.str(), "(1, hello)");
}

TEST_F(TuplePrintTest, PrintsEmptyTuple) {
    auto t = make_tuple();
    stringstream ss;
    ss << t;
    EXPECT_EQ(ss.str(), "()");
}

TEST_F(TuplePrintTest, PrintsThreeElements) {
    auto t = make_tuple(42, "test", 3.14);
    stringstream ss;
    ss << t;
    string result = ss.str();
    EXPECT_TRUE(result.find("42") != string::npos);
    EXPECT_TRUE(result.find("test") != string::npos);
}

class CSVParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        ofstream file("test.csv");
        file << "1,Alice\n2,Bob\n3,Charlie\n";
        file.close();

        ofstream file3("test_semicolon.csv");
        file3 << "10;2.5\n20;3.5\n30;4.5\n";
        file3.close();
    }

    void TearDown() override {
        remove("test.csv");
        remove("test_semicolon.csv");
    }
};

TEST_F(CSVParserTest, ParsesBasicCSV) {
    ifstream file("test.csv");
    CSVParser<int, string> parser(file, 0);

    vector<tuple<int, string>> results;
    for (auto row : parser) {
        results.push_back(row);
    }
    file.close();

    ASSERT_EQ(results.size(), 3);
    EXPECT_EQ(get<0>(results[0]), 1);
    EXPECT_EQ(get<1>(results[0]), "Alice");
    EXPECT_EQ(get<0>(results[1]), 2);
    EXPECT_EQ(get<1>(results[1]), "Bob");
}

TEST_F(CSVParserTest, SkipsLines) {
    ifstream file("test.csv");
    CSVParser<int, string> parser(file, 1);

    vector<tuple<int, string>> results;
    for (auto row : parser) {
        results.push_back(row);
    }
    file.close();

    ASSERT_EQ(results.size(), 2);
    EXPECT_EQ(get<0>(results[0]), 2);
    EXPECT_EQ(get<1>(results[0]), "Bob");
}

TEST_F(CSVParserTest, IteratorInterface) {
    ifstream file("test.csv");
    CSVParser<int, string> parser(file, 0);

    auto begin = parser.begin();
    auto end = parser.end();

    EXPECT_NE(begin, end);
    auto row1 = *begin;
    ++begin;
    EXPECT_EQ(get<0>(row1), 1);
    file.close();
}

TEST_F(CSVParserTest, ThrowsOnMissingFields) {
    stringstream ss("1,Alice,extra\n2,Bob\n");
    CSVParser<int, string> parser(ss, 0);

    try {
        int count = 0;
        for (auto row : parser) {
            ++count;
        }
    } catch (const CSVParseException& e) {
        EXPECT_TRUE(e.line > 0);
        EXPECT_TRUE(e.column > 0);
    }
}
