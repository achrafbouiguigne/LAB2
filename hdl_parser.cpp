#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

using namespace std;

class FileOpenException : public runtime_error {
public:
    FileOpenException(const string& filename) 
        : runtime_error("Cannot open file: " + filename) {}
};

class ParseException : public runtime_error {
public:
    ParseException(const string& msg, int lineNum)
        : runtime_error("Error at line " + to_string(lineNum) + ": " + msg) {}
};

string cleanName(const string& raw) {
    string result;
    for (size_t i = 0; i < raw.length(); i++) {
        char c = raw[i];
        if (isalnum(c) || c == '_') {
            result += c;
        } else if (c == ',' || c == ';' || c == ')' || c == ':') {
            break;
        }
    }
    while (!result.empty() && result.back() == ' ') {
        result.pop_back();
    }
    return result;
}

vector<string> getSignals(const string& line, const string& keyword) {
    vector<string> signals;
    string lowerLine = line;
    transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
    
    size_t pos = lowerLine.find(keyword);
    if (pos == string::npos) return signals;
    
    size_t start = pos + keyword.length();
    size_t colon = line.find(':', start);
    if (colon != string::npos && colon < line.find(';')) {
        start = colon + 1;
    }
    
    size_t semicolon = line.find(';', start);
    if (semicolon == string::npos) {
        semicolon = line.length();
    }
    
    string signal_part = line.substr(start, semicolon - start);
    stringstream ss(signal_part);
    string item;
    
    while (getline(ss, item, ',')) {
        string cleaned = cleanName(item);
        if (!cleaned.empty()) {
            signals.push_back(cleaned);
        }
    }
    
    return signals;
}

string getFileType(const string& filename) {
    size_t dot = filename.find_last_of('.');
    if (dot == string::npos) {
        return "unknown";
    }
    
    string ext = filename.substr(dot);
    for (size_t i = 0; i < ext.length(); i++) {
        ext[i] = tolower(ext[i]);
    }
    
    if (ext == ".vhd" || ext == ".vhdl") {
        return "vhdl";
    } else if (ext == ".v" || ext == ".sv") {
        return "verilog";
    }
    return "unknown";
}

void parseVHDL(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw FileOpenException(filename);
    }
    
    string line;
    int lineNum = 0;
    vector<string> inputs;
    vector<string> outputs;
    bool inPort = false;
    
    while (getline(file, line)) {
        lineNum++;
        string lowerLine = line;
        transform(lowerLine.begin(), lowerLine.end(), lowerLine.begin(), ::tolower);
        
        if (lowerLine.find("entity") != string::npos && lowerLine.find("port") != string::npos) {
            inPort = true;
            continue;
        }
        
        if (inPort) {
            if (lowerLine.find("in") != string::npos) {
                vector<string> sigs = getSignals(line, "in");
                for (size_t i = 0; i < sigs.size(); i++) {
                    inputs.push_back(sigs[i]);
                }
            }
            if (lowerLine.find("out") != string::npos) {
                size_t outPos = lowerLine.find("out");
                if (outPos == 0 || lowerLine[outPos-1] == ' ' || lowerLine[outPos-1] == '(') {
                    vector<string> sigs = getSignals(line, "out");
                    for (size_t i = 0; i < sigs.size(); i++) {
                        outputs.push_back(sigs[i]);
                    }
                }
            }
            
            if (lowerLine.find(");") != string::npos || lowerLine.find("end") != string::npos) {
                break;
            }
        }
    }
    
    file.close();
    
    cout << "\n========== VHDL RESULTS ==========\n";
    cout << "File: " << filename << "\n\n";
    cout << "Inputs (" << inputs.size() << "):\n";
    for (size_t i = 0; i < inputs.size(); i++) {
        cout << "  - " << inputs[i] << "\n";
    }
    cout << "\nOutputs (" << outputs.size() << "):\n";
    for (size_t i = 0; i < outputs.size(); i++) {
        cout << "  - " << outputs[i] << "\n";
    }
}

void parseVerilog(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw FileOpenException(filename);
    }
    
    string line;
    int lineNum = 0;
    vector<string> inputs;
    vector<string> outputs;
    string wholeFile = "";
    
    while (getline(file, line)) {
        wholeFile += " " + line;
        lineNum++;
    }
    
    file.close();
    
    string lowerFile = wholeFile;
    transform(lowerFile.begin(), lowerFile.end(), lowerFile.begin(), ::tolower);
    
    size_t modulePos = lowerFile.find("module");
    if (modulePos != string::npos) {
        size_t parenStart = lowerFile.find('(', modulePos);
        if (parenStart != string::npos) {
            size_t parenEnd = lowerFile.find(')', parenStart);
            if (parenEnd != string::npos) {
                string portArea = wholeFile.substr(parenStart + 1, parenEnd - parenStart - 1);
                string lowerPort = portArea;
                transform(lowerPort.begin(), lowerPort.end(), lowerPort.begin(), ::tolower);
                
                size_t pos = 0;
                while (pos < lowerPort.length()) {
                    size_t inputPos = lowerPort.find("input", pos);
                    size_t outputPos = lowerPort.find("output", pos);
                    
                    size_t found = string::npos;
                    string type = "";
                    
                    if (inputPos != string::npos && (outputPos == string::npos || inputPos < outputPos)) {
                        found = inputPos;
                        type = "input";
                    } else if (outputPos != string::npos) {
                        found = outputPos;
                        type = "output";
                    }
                    
                    if (found == string::npos) break;
                    
                    size_t startName = found + type.length();
                    while (startName < lowerPort.length() && (lowerPort[startName] == ' ' || lowerPort[startName] == '\t')) {
                        startName++;
                    }
                    
                    string name = "";
                    while (startName < lowerPort.length()) {
                        char c = portArea[startName];
                        if (isalnum(c) || c == '_') {
                            name += c;
                            startName++;
                        } else if (c == '[') {
                            while (startName < lowerPort.length() && portArea[startName] != ']') {
                                startName++;
                            }
                            startName++;
                            while (startName < lowerPort.length() && (portArea[startName] == ' ' || portArea[startName] == '\t')) {
                                startName++;
                            }
                        } else if (c == ',' || c == ')' || c == ' ' || c == '\t') {
                            break;
                        } else {
                            break;
                        }
                    }
                    
                    if (!name.empty()) {
                        if (type == "input") {
                            inputs.push_back(name);
                        } else {
                            outputs.push_back(name);
                        }
                    }
                    
                    pos = found + 1;
                }
            }
        }
    }
    
    cout << "\n========== VERILOG RESULTS ==========\n";
    cout << "File: " << filename << "\n\n";
    cout << "Inputs (" << inputs.size() << "):\n";
    for (size_t i = 0; i < inputs.size(); i++) {
        cout << "  - " << inputs[i] << "\n";
    }
    cout << "\nOutputs (" << outputs.size() << "):\n";
    for (size_t i = 0; i < outputs.size(); i++) {
        cout << "  - " << outputs[i] << "\n";
    }
}

int main(int argc, char* argv[]) {
    string filename;
    
    if (argc > 1) {
        filename = argv[1];
    } else {
        cout << "Enter file name (.vhd or .v): ";
        cin >> filename;
    }
    
    try {
        string type = getFileType(filename);
        
        if (type == "vhdl") {
            parseVHDL(filename);
        } else if (type == "verilog") {
            parseVerilog(filename);
        } else {
            cout << "Unknown file type. Use .vhd or .v" << endl;
            return 1;
        }
        
        cout << "\nDone!" << endl;
        
    } catch (const FileOpenException& e) {
        cerr << "\nERROR: " << e.what() << endl;
        return 1;
    } catch (const ParseException& e) {
        cerr << "\nERROR: " << e.what() << endl;
        return 1;
    } catch (const exception& e) {
        cerr << "\nERROR: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}