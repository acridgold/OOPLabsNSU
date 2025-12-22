#pragma once
#include <tuple>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <iterator>

using namespace std;

// ===== SUBTASK #1: Print tuple =====

namespace detail {
    template<typename Tuple, size_t I = 0>
    struct TuplePrinter {
        static void print(ostream& os, const Tuple& t) {
            if constexpr (I > 0) os << ", ";
            os << get<I>(t);
            if constexpr (I + 1 < tuple_size_v<Tuple>) {
                TuplePrinter<Tuple, I + 1>::print(os, t);
            }
        }
    };
}

template<typename Ch, typename Tr, typename... Args>
ostream& operator<<(basic_ostream<Ch, Tr>& os, const tuple<Args...>& t) {
    os << "(";
    if constexpr (sizeof...(Args) > 0) {
        detail::TuplePrinter<tuple<Args...>>::print(os, t);
    }
    os << ")";
    return os;
}

// ===== SUBTASK #2: CSV Parser =====

struct CSVParseException : runtime_error {
    size_t line;
    size_t column;

    CSVParseException(const string& msg, size_t l, size_t c)
        : runtime_error(msg + " at line " + to_string(l) + ", column " + to_string(c)),
          line(l), column(c) {}
};

inline string trim(const string& str) {
    size_t start = 0, end = str.length();
    while (start < end && isspace(str[start])) ++start;
    while (end > start && isspace(str[end - 1])) --end;
    return str.substr(start, end - start);
}

template<typename T>
T parseValue(const string& str) {
    string clean = trim(str);
    stringstream ss(clean);
    T val;
    if (!(ss >> val)) {
        throw runtime_error("Cannot parse value: " + str);
    }
    return val;
}

template<>
inline string parseValue<string>(const string& str) {
    return trim(str);
}






template<typename... Types>
class CSVParser {
private:
    istream& file_;
    size_t skipLines_;
    char rowDelim_;
    char colDelim_;

    string readLine() {
        string line;
        if (!getline(file_, line, rowDelim_)) {
            return "";
        }
        return line;
    }

    vector<string> splitLine(const string& line, size_t& lineNum) {
        vector<string> result;
        string current;
        size_t colNum = 1;

        for (size_t i = 0; i < line.length(); ++i) {
            char c = line[i];

            if (c == colDelim_) {
                result.push_back(current);
                current.clear();
                ++colNum;
            } else {
                current += c;
            }
        }

        result.push_back(current);


        return result;
    }

    template<typename Tuple, size_t I = 0>
    void fillTuple(Tuple& t, const vector<string>& fields, size_t lineNum) {
        if constexpr (I < tuple_size_v<Tuple>) {
            if (I >= fields.size()) {
                throw CSVParseException("Not enough fields", lineNum, I + 1);
            }
            try {
                get<I>(t) = ::parseValue<tuple_element_t<I, Tuple>>(fields[I]);
            } catch (const runtime_error& e) {
                throw CSVParseException(e.what(), lineNum, I + 1);
            }
            if constexpr (I + 1 < tuple_size_v<Tuple>) {
                fillTuple<Tuple, I + 1>(t, fields, lineNum);
            }
        }
    }

public:
    class Iterator {
    private:
        CSVParser* parser_;
        tuple<Types...> current_;
        bool isEnd_;
        size_t lineNum_;

        void advance() {
            if (!parser_) {
                isEnd_ = true;
                return;
            }

            string line;
            while (true) {
                line = parser_->readLine();
                if (line.empty() && parser_->file_.eof()) {
                    isEnd_ = true;
                    return;
                }
                // Skip empty lines
                if (!line.empty()) {
                    break;
                }
                ++lineNum_;
            }

            try {
                auto fields = parser_->splitLine(line, lineNum_);
                parser_->fillTuple(current_, fields, lineNum_);
                ++lineNum_;
            } catch (const CSVParseException&) {
                throw;
            }
        }

    public:
        using difference_type = ptrdiff_t;
        using value_type = tuple<Types...>;
        using pointer = tuple<Types...>*;
        using reference = tuple<Types...>&;
        using iterator_category = input_iterator_tag;

        Iterator() : parser_(nullptr), isEnd_(true), lineNum_(0) {}

        Iterator(CSVParser* p) : parser_(p), isEnd_(false), lineNum_(1) {
            advance();
        }

        reference operator*() { return current_; }
        pointer operator->() { return &current_; }

        Iterator& operator++() {
            advance();
            return *this;
        }

        Iterator operator++(int) {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(const Iterator& other) const {
            return (isEnd_ && other.isEnd_) ||
                   (parser_ == other.parser_ && isEnd_ == other.isEnd_);
        }

        bool operator!=(const Iterator& other) const {
            return !(*this == other);
        }
    };

    CSVParser(istream& file, size_t skipLines = 0,
              char rowDelim = '\n', char colDelim = ',')
        : file_(file), skipLines_(skipLines), rowDelim_(rowDelim),
          colDelim_(colDelim)
    {
        for (size_t i = 0; i < skipLines_; ++i) {
            string dummy;
            getline(file_, dummy);
        }
    }

    Iterator begin() {
        return Iterator(this);
    }

    Iterator end() {
        return Iterator();
    }
};

