#include <iostream>
#include <string>
#include <sstream>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <conio.h>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
vector<string> history;
int historyIndex = -1;

vector<string> commands = {"help", "clear", "exit", "list", "open", "create"};

void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void printPrompt() {
    setColor(10);
    cout << "{" << fs::current_path().string() << "}/ ";
    setColor(7);
}

void listDirectory() {
    for (const auto& entry : fs::directory_iterator(fs::current_path())) {
        if (fs::is_directory(entry)) {
            setColor(9); // Blue
            cout << entry.path().filename().string() << "/";
        } else {
            setColor(7);
            cout << entry.path().filename().string();
        }
        cout << endl;
    }
    setColor(7);
}

string extractFullArgument(const string& input, size_t startPos) {
    string result = input.substr(startPos);
    if (!result.empty() && result.front() == '"' && result.back() == '"')
        result = result.substr(1, result.size() - 2);
    return result;
}

void openPath(const string& target) {
    try {
        fs::path p(target);
        if (!p.is_absolute()) p = fs::current_path() / p;
        p = fs::weakly_canonical(p);
        if (!fs::exists(p)) { cout << "Path not found.\n"; return; }
        if (fs::is_directory(p)) fs::current_path(p);
        else ShellExecuteW(NULL, L"open", p.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    catch (...) { cout << "Invalid path.\n"; }
}

void createFile(const string& name) {
    if (name == "!empty") { ofstream("empty.txt").close(); cout << "Created empty file: empty.txt\n"; return; }
    if (!name.empty()) { ofstream(name).close(); cout << "Created file: " << name << endl; return; }
    cout << "Specify file name.\n";
}

void executeCommand(const string& input) {
    stringstream ss(input);
    string cmd;
    ss >> cmd;

    if (cmd == "help") {
        setColor(11);
        cout << "Commands:\nhelp\nclear\nexit\nlist\n";
        cout << "open <folder|file.exe>\ncreate file <name>\ncreate file !empty\n";
        setColor(7);
    }
    else if (cmd == "clear") system("cls");
    else if (cmd == "exit") exit(0);
    else if (cmd == "list") listDirectory();
    else if (cmd == "open") {
        size_t pos = input.find("open") + 5;
        string target = extractFullArgument(input, pos);
        if (target.empty()) cout << "Specify target.\n";
        else openPath(target);
    }
    else if (cmd == "create") {
        string type; ss >> type;
        size_t pos = input.find(type) + type.length() + 1;
        string name = extractFullArgument(input, pos);
        if (type == "file") createFile(name);
        else cout << "Invalid create syntax.\n";
    }
    else if (!cmd.empty()) cout << "Unknown command.\n";
}

// ---------------- Input with history, editing, and visible tab completion ----------------
string getInput() {
    string input;
    historyIndex = history.size();
    int cursorPos = 0;

    auto redrawInput = [&]() {
        cout << "\r";
        printPrompt();
        cout << input << " ";
        cout << string(input.size() - cursorPos, '\b');
    };

    auto tryTabComplete = [&]() {
        size_t start = input.rfind(' ', cursorPos - 1);
        if (start == string::npos) start = 0;
        else start++;
        string fragment = input.substr(start, cursorPos - start);

        vector<string> matches;

        // Command completion if at start
        if (start == 0) {
            for (auto& cmd : commands)
                if (cmd.find(fragment) == 0) matches.push_back(cmd);
        } else {
            // File/folder completion
            for (auto& entry : fs::directory_iterator(fs::current_path())) {
                string name = entry.path().filename().string();
                if (name.find(fragment) == 0) matches.push_back(name);
            }
        }

        if (matches.empty()) return;

        if (matches.size() == 1) {
            input.replace(start, cursorPos - start, matches[0]);
            cursorPos = start + matches[0].size();
        } else {
            // Show suggestions visibly
            cout << endl;
            for (auto& m : matches) cout << m << "    ";
            cout << endl;
        }

        redrawInput();
    };

    while (true) {
        int ch = _getch();

        if (ch == 13) { // Enter
            cout << endl;
            if (!input.empty()) history.push_back(input);
            return input;
        }
        else if (ch == 8) { // Backspace
            if (cursorPos > 0) {
                input.erase(cursorPos - 1, 1);
                cursorPos--;
                redrawInput();
            }
        }
        else if (ch == 9) { // Tab
            tryTabComplete();
        }
        else if (ch == 224) { // Arrow keys
            ch = _getch();
            if (ch == 72) { // Up
                if (!history.empty() && historyIndex > 0) {
                    historyIndex--;
                    input = history[historyIndex];
                    cursorPos = input.size();
                    redrawInput();
                }
            }
            else if (ch == 80) { // Down
                if (!history.empty() && historyIndex < (int)history.size() - 1) {
                    historyIndex++;
                    input = history[historyIndex];
                    cursorPos = input.size();
                    redrawInput();
                } else if (historyIndex == (int)history.size() - 1) {
                    historyIndex = history.size();
                    input.clear();
                    cursorPos = 0;
                    redrawInput();
                }
            }
        }
        else if (isprint(ch)) {
            input.insert(cursorPos, 1, ch);
            cursorPos++;
            redrawInput();
        }
    }
}

// ---------------- Main ----------------
int main() {
    while (true) {
        printPrompt();
        string input = getInput();
        executeCommand(input);
    }
    return 0;
}
