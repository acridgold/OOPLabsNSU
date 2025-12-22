#include "CSVParser.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace std;

int main() {
    try {
        cout << "=== Test 1 ===" << endl;

        ifstream file1("test.csv");
        if (!file1) {
            cerr << "Cannot open test.csv" << endl;
            return 1;
        }

        CSVParser<int, string> parser1(file1, 0);
        cout << "Parsed rows:" << endl;
        for (const auto& row : parser1) {
            cout << row << endl;
        }
        file1.close();

        cout << "\n=== Test 2 ===" << endl;

        ifstream file2("test2.csv");
        if (!file2) {
            cerr << "Cannot open test2.csv" << endl;
            return 1;
        }

        CSVParser<int, double> parser2(file2, 0, '\n', ';');
        cout << "Parsed rows:" << endl;
        for (auto row : parser2) {
            cout << row << endl;
        }
        file2.close();

        return 0;

    } catch (const CSVParseException& e) {
        cerr << "Parse error: " << e.what() << endl;
        return 2;
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 3;
    }
}
