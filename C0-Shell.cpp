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

const string HISTORY_FILE = "shell_history.txt";

vector<string> commands = {
    "help", "clear", "exit", "list",
    "open", "create", "mkdir", "del",
    "rename", "copy"
};

void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void printError(const string& msg) {
    setColor(12);
    cout << "Error: " << msg << "\n";
    setColor(7);
}

void printSuccess(const string& msg) {
    setColor(10);
    cout << msg << "\n";
    setColor(7);
}

void loadHistory() {
    ifstream f(HISTORY_FILE);
    if (!f.is_open()) return;
    string line;
    while (getline(f, line))
        if (!line.empty()) history.push_back(line);
}

void appendHistory(const string& cmd) {
    ofstream f(HISTORY_FILE, ios::app);
    if (f.is_open()) f << cmd << "\n";
}

void printBanner() {
    setColor(11);
    cout << "\n";
    cout << "   _____ ___     _____ _          _ _ \n";
    cout << "  / ____/ _ \\   / ____| |        | | |\n";
    cout << " | |   | | | | | (___ | |__   ___| | |\n";
    cout << " | |   | | | |  \\___ \\| '_ \\ / _ \\ | |\n";
    cout << " | |___| |_| |  ____) | | | |  __/ | |\n";
    cout << "  \\_____\\___/  |_____/|_| |_|\\___|_|_|\n";
    cout << "\n";
    setColor(8);
    cout << "  ChickenGuyRblx Shell  |  Type 'help' for commands\n";
    setColor(7);
    cout << "  ------------------------------------------\n\n";
}

void printPrompt() {
    setColor(13);
    cout << "[";
    setColor(11);
    cout << fs::current_path().string();
    setColor(13);
    cout << "]";
    setColor(10);
    cout << " > ";
    setColor(7);
}

void listDirectory() {
    vector<fs::directory_entry> dirs, files;
    try {
        for (const auto& e : fs::directory_iterator(fs::current_path()))
            (fs::is_directory(e) ? dirs : files).push_back(e);
    } catch (...) { printError("Could not list directory."); return; }

    for (auto& e : dirs) {
        setColor(9);
        cout << "  [DIR]  ";
        setColor(11);
        cout << e.path().filename().string() << "\n";
    }
    for (auto& e : files) {
        setColor(8);
        cout << "  [FILE] ";
        setColor(7);
        cout << e.path().filename().string() << "\n";
    }
    setColor(7);
}

string extractFullArgument(const string& input, size_t startPos) {
    if (startPos >= input.size()) return "";
    string result = input.substr(startPos);
    if (result.size() >= 2 && result.front() == '"' && result.back() == '"')
        result = result.substr(1, result.size() - 2);
    return result;
}

string parseToken(const string& s, size_t& pos) {
    while (pos < s.size() && s[pos] == ' ') pos++;
    if (pos >= s.size()) return "";
    if (s[pos] == '"') {
        size_t end = s.find('"', pos + 1);
        if (end == string::npos) end = s.size();
        string r = s.substr(pos + 1, end - pos - 1);
        pos = end + 1;
        return r;
    }
    size_t end = s.find(' ', pos);
    if (end == string::npos) end = s.size();
    string r = s.substr(pos, end - pos);
    pos = end;
    return r;
}

void openPath(const string& target) {
    try {
        fs::path p(target);
        if (!p.is_absolute()) p = fs::current_path() / p;
        p = fs::weakly_canonical(p);
        if (!fs::exists(p)) { printError("Path not found: " + target); return; }
        if (fs::is_directory(p)) fs::current_path(p);
        else ShellExecuteW(NULL, L"open", p.wstring().c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    catch (...) { printError("Invalid path: " + target); }
}

void createFile(const string& name) {
    if (name == "!empty") { ofstream("empty.txt").close(); printSuccess("Created: empty.txt"); return; }
    if (!name.empty()) {
        if (fs::exists(name)) { printError("Already exists: " + name); return; }
        ofstream(name).close();
        printSuccess("Created: " + name);
        return;
    }
    printError("Specify a file name.");
}

void makeDir(const string& name) {
    if (name.empty()) { printError("Specify a folder name."); return; }
    if (fs::exists(name)) { printError("Already exists: " + name); return; }
    try { fs::create_directory(name); printSuccess("Created folder: " + name); }
    catch (...) { printError("Could not create folder: " + name); }
}

void deleteItem(const string& name) {
    if (name.empty()) { printError("Specify a file or folder to delete."); return; }
    fs::path p = fs::current_path() / name;
    if (!fs::exists(p)) { printError("Not found: " + name); return; }
    try { fs::remove_all(p); printSuccess("Deleted: " + name); }
    catch (...) { printError("Could not delete: " + name); }
}

void renameItem(const string& from, const string& to) {
    if (from.empty() || to.empty()) { printError("Usage: rename <old> <new>"); return; }
    fs::path src = fs::current_path() / from;
    fs::path dst = fs::current_path() / to;
    if (!fs::exists(src)) { printError("Not found: " + from); return; }
    if (fs::exists(dst)) { printError("Target already exists: " + to); return; }
    try { fs::rename(src, dst); printSuccess("Renamed: " + from + " -> " + to); }
    catch (...) { printError("Could not rename."); }
}

void copyItem(const string& from, const string& to) {
    if (from.empty() || to.empty()) { printError("Usage: copy <source> <dest>"); return; }
    fs::path src = fs::current_path() / from;
    fs::path dst = fs::current_path() / to;
    if (!fs::exists(src)) { printError("Not found: " + from); return; }
    try {
        fs::copy(src, dst, fs::copy_options::recursive | fs::copy_options::overwrite_existing);
        printSuccess("Copied: " + from + " -> " + to);
    }
    catch (...) { printError("Could not copy."); }
}

void executeCommand(const string& input) {
    stringstream ss(input);
    string cmd;
    ss >> cmd;
    transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

    if (cmd == "help") {
        setColor(11);
        cout << "\n  Navigation\n";
        setColor(7);
        cout << "    list                  List files and folders\n";
        cout << "    open <path>           Navigate into folder or open file\n";
        setColor(11);
        cout << "\n  File Operations\n";
        setColor(7);
        cout << "    create file <name>    Create a new file\n";
        cout << "    create file !empty    Create empty.txt\n";
        cout << "    mkdir <name>          Create a new folder\n";
        cout << "    del <name>            Delete a file or folder\n";
        cout << "    rename <old> <new>    Rename a file or folder\n";
        cout << "    copy <src> <dest>     Copy a file or folder\n";
        setColor(11);
        cout << "\n  General\n";
        setColor(7);
        cout << "    help                  Show this menu\n";
        cout << "    clear                 Clear the screen\n";
        cout << "    exit                  Exit the shell\n\n";
    }
    else if (cmd == "clear") { system("cls"); printBanner(); }
    else if (cmd == "exit") { exit(0); }
    else if (cmd == "list") { listDirectory(); }
    else if (cmd == "open") {
        size_t pos = input.find("open") + 5;
        string target = extractFullArgument(input, pos);
        if (target.empty()) printError("Specify a target.");
        else openPath(target);
    }
    else if (cmd == "create") {
        string type; ss >> type;
        size_t pos = input.find(type) + type.length() + 1;
        string name = extractFullArgument(input, pos);
        if (type == "file") createFile(name);
        else printError("Unknown create type. Try: create file <name>");
    }
    else if (cmd == "mkdir") {
        size_t pos = input.find("mkdir") + 6;
        makeDir(extractFullArgument(input, pos));
    }
    else if (cmd == "del") {
        size_t pos = input.find("del") + 4;
        deleteItem(extractFullArgument(input, pos));
    }
    else if (cmd == "rename") {
        size_t pos = input.find("rename") + 6;
        string a = parseToken(input, pos);
        string b = parseToken(input, pos);
        renameItem(a, b);
    }
    else if (cmd == "copy") {
        size_t pos = input.find("copy") + 5;
        string a = parseToken(input, pos);
        string b = parseToken(input, pos);
        copyItem(a, b);
    }
    else if (!cmd.empty()) {
        printError("Unknown command: '" + cmd + "'. Type 'help' to see available commands.");
    }
}

string getInput() {
    string input;
    historyIndex = (int)history.size();
    int cursorPos = 0;

    auto redrawInput = [&]() {
        cout << "\r";
        printPrompt();
        cout << input << " ";
        int moveBack = (int)input.size() - cursorPos + 1;
        if (moveBack > 0) cout << string(moveBack, '\b');
    };

    auto tryTabComplete = [&]() {
        size_t start = (cursorPos > 0) ? input.rfind(' ', cursorPos - 1) : string::npos;
        if (start == string::npos) start = 0;
        else start++;
        string fragment = input.substr(start, cursorPos - start);

        vector<string> matches;
        if (start == 0) {
            for (auto& c : commands)
                if (c.find(fragment) == 0) matches.push_back(c);
        } else {
            try {
                for (auto& entry : fs::directory_iterator(fs::current_path())) {
                    string name = entry.path().filename().string();
                    if (name.find(fragment) == 0) matches.push_back(name);
                }
            } catch (...) {}
        }

        if (matches.empty()) return;
        if (matches.size() == 1) {
            string completion = matches[0];
            if (completion.find(' ') != string::npos)
                completion = '"' + completion + '"';
            input.replace(start, cursorPos - start, completion);
            cursorPos = (int)(start + completion.size());
        } else {
            cout << "\n";
            setColor(14);
            for (auto& m : matches) cout << "  " << m;
            setColor(7);
            cout << "\n";
        }
        redrawInput();
    };

    while (true) {
        int ch = _getch();

        if (ch == 13) {
            cout << "\n";
            if (!input.empty()) {
                history.push_back(input);
                appendHistory(input);
            }
            return input;
        }
        else if (ch == 8) {
            if (cursorPos > 0) {
                input.erase(cursorPos - 1, 1);
                cursorPos--;
                redrawInput();
            }
        }
        else if (ch == 9) { tryTabComplete(); }
        else if (ch == 224) {
            ch = _getch();
            if (ch == 72 && !history.empty() && historyIndex > 0) {
                historyIndex--;
                input = history[historyIndex];
                cursorPos = (int)input.size();
                redrawInput();
            }
            else if (ch == 80 && !history.empty()) {
                if (historyIndex < (int)history.size() - 1) {
                    historyIndex++;
                    input = history[historyIndex];
                    cursorPos = (int)input.size();
                } else {
                    historyIndex = (int)history.size();
                    input.clear();
                    cursorPos = 0;
                }
                redrawInput();
            }
            else if (ch == 75 && cursorPos > 0) {
                cursorPos--;
                cout << "\b";
            }
            else if (ch == 77 && cursorPos < (int)input.size()) {
                cout << input[cursorPos];
                cursorPos++;
            }
        }
        else if (isprint(ch)) {
            input.insert(cursorPos, 1, (char)ch);
            cursorPos++;
            redrawInput();
        }
    }
}

int main() {
    SetConsoleOutputCP(437);
    loadHistory();
    printBanner();

    while (true) {
        printPrompt();
        string input = getInput();
        executeCommand(input);
    }
    return 0;
}