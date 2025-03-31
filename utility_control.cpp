#include <windows.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <io.h>
#include <fcntl.h>
#include <sstream>
#include <string>

//добавить восстановление данных (мердж фулл и ласт бэкап в сорсдир)
//добавить HANDLE в класс (свойство, конструктор, сер и десер), проверку на running process (GetExitCodeProcess, в него inst.handle)

using namespace std;

namespace fs = std::filesystem;

class UtilInstance {
    private:
        int pid;
        string sourcePath;
        string backupPath;
        size_t maxCopies;
        HANDLE pHandle;
        
    public:
        // Конструктор для инициализации свойств
        UtilInstance(int pid, string sourcePath, string backupPath, size_t maxCopies)
            : pid(pid), sourcePath(sourcePath), backupPath(backupPath), maxCopies(maxCopies) {}
        
        UtilInstance() {}

        int getPid() const {
            return pid;
        }

        void setPid(int newpid)
        {
            pid = newpid;
        }

        string getSourcePath() const {
            return sourcePath;
        }

        string getBackupPath() const {
            return backupPath;
        }

        size_t getMaxCopies() const {
            return maxCopies;
        }

         // Метод для записи объекта в бинарный файл
    
        void serialize(ofstream& out) const {
        // Записываем pid
            out.write(reinterpret_cast<const char*>(&pid), sizeof(pid));

        // Записываем длину строки sourcePath и саму строку
            size_t sourcePathLength = sourcePath.size();
            out.write(reinterpret_cast<const char*>(&sourcePathLength), sizeof(sourcePathLength));
            out.write(sourcePath.c_str(), sourcePathLength);

            // Записываем длину строки backupPath и саму строку
            size_t backupPathLength = backupPath.size();
            out.write(reinterpret_cast<const char*>(&backupPathLength), sizeof(backupPathLength));
            out.write(backupPath.c_str(), backupPathLength);

            // Записываем maxCopies
            out.write(reinterpret_cast<const char*>(&maxCopies), sizeof(maxCopies));
    }
        // Метод для чтения объекта из бинарного файла
        void deserialize(ifstream& in) {
            // Читаем pid
            in.read(reinterpret_cast<char*>(&pid), sizeof(pid));

            // Читаем длину строки sourcePath и саму строку
            size_t sourcePathLength;
            in.read(reinterpret_cast<char*>(&sourcePathLength), sizeof(sourcePathLength));
            sourcePath.resize(sourcePathLength);
            in.read(&sourcePath[0], sourcePathLength);

            // Читаем длину строки backupPath и саму строку
            size_t backupPathLength;
            in.read(reinterpret_cast<char*>(&backupPathLength), sizeof(backupPathLength));
            backupPath.resize(backupPathLength);
            in.read(&backupPath[0], backupPathLength);

            // Читаем maxCopies
            in.read(reinterpret_cast<char*>(&maxCopies), sizeof(maxCopies));
    }
    };

vector < UtilInstance > uList;

string escapeBackslashes(string& input) {
    string result;
    for (char ch : input) {
        result += ch;
        if (ch == '\\') {
            result += '\\';
        }
    }
    return result;
}

string replaceDoubleBackslashes(const std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.length(); ++i) {
        if (input[i] == '\\' && i + 1 < input.length() && input[i + 1] == '\\') {
            result += '\\'; 
            ++i;
        } else {
            result += input[i];
        }
    }
    return result;
}

int newUtilInst() {
    fflush(stdin);

    string sourcePath, backupPath;
    size_t maxCopies;

    cout << "Catalogue to copy: ";
    getline(cin, sourcePath);
    sourcePath = escapeBackslashes(sourcePath);

    cout << "Where to store the copies: ";
    getline(cin, backupPath);
    backupPath = escapeBackslashes(backupPath);

    cout << "Max copy count: ";
    cin >> maxCopies;

    stringstream ss;
    ss << "backup_utility.exe " << sourcePath << " " << backupPath << " " << maxCopies;
    string s = ss.str();

    LPSTR backup_args = const_cast<char *>(s.c_str());

    PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter

    STARTUPINFO StartupInfo; //This is an [in] parameter

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field

    if(CreateProcess("backup_utility.exe", (char *)backup_args, 
        NULL,NULL,FALSE,DETACHED_PROCESS,NULL,
        NULL,&StartupInfo,&ProcessInfo))
    { 
        if(ProcessInfo.hThread)
            CloseHandle(ProcessInfo.hThread);
        if(ProcessInfo.hProcess)
            CloseHandle(ProcessInfo.hProcess);
    }  
    else
    {
        cout << "The proccess could not be started." << endl;
        return -1;
    }
    
    cout << "New directory was added to the watch list!" << endl;
    cout << "Watched directory: " << sourcePath << endl;
    cout << "Backup directory: " << backupPath << endl;

    UtilInstance u((int)ProcessInfo.dwProcessId, sourcePath, backupPath, maxCopies); 

    uList.insert(uList.begin(), u);

    return 0;
}

int contUtilInst(size_t i, string sourcePath, string backupPath, size_t maxCopies) {

    stringstream ss;
    ss << "backup_utility.exe " << sourcePath << " " << backupPath << " " << maxCopies;
    string s = ss.str();

    LPSTR backup_args = const_cast<char *>(s.c_str());

    PROCESS_INFORMATION ProcessInfo; //This is what we get as an [out] parameter

    STARTUPINFO StartupInfo; //This is an [in] parameter

    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo ; //Only compulsory field

    if(CreateProcess("backup_utility.exe", (char *)backup_args, 
        NULL,NULL,FALSE,DETACHED_PROCESS,NULL,
        NULL,&StartupInfo,&ProcessInfo))
    { 
        if(ProcessInfo.hThread)
            CloseHandle(ProcessInfo.hThread);
        if(ProcessInfo.hProcess)
            CloseHandle(ProcessInfo.hProcess);
    }  
    else
    {
        cout << "The proccess could not be started." << endl;
        return -1;
    }
    
    cout << "Watched directory: " << sourcePath << endl;
    cout << "Backup directory: " << backupPath << endl;
    cout << "---------------------------------------------" << endl;

    uList[i].setPid((int)ProcessInfo.dwProcessId);

    return 0;
}

int removeUtilInstance(size_t i)
{
    const auto explorer = OpenProcess(PROCESS_TERMINATE, false, uList[i].getPid());
    TerminateProcess(explorer, 1);
    CloseHandle(explorer);
    uList.erase(uList.begin() + i);
    return 0;
}

int recoverData(size_t index)
{
    //по индексу получаем изначальную дир и дир бэкапов
    //в дир бэкапов находим фулл и посл дифф
    //объединяем фулл и посл дифф в темп дир
    //заменяем содержимое изначальной дир на темп дир, темп дир удаляем
    return 0;
}

int showList()
{
    cout << "List of all watched directories: " << endl;
    int num = 1;
    if (uList.empty())
    {
        cout << "No watched directories!" << endl;
        return 0;
    }
    for (size_t i = 0; i < uList.size(); ++i) {
        string path = uList[i].getSourcePath();
        cout << num << ". " << replaceDoubleBackslashes(path)  << " " << uList[i].getPid() << endl;
        i++;
    } 
    
    int choice, index;
    size_t i;
    bool showing_menu = true;

    while (showing_menu) {
    cout << "Available actions: " << endl;
    cout << "1. Remove from watch list" << endl;
    cout << "2. Data recovery" << endl;
    cout << "3. Return to main menu" << endl;
    cin >> choice;
    fflush(stdin);
    if (choice == 1 || choice == 2) {
        cout << "For which directory? (Index): " << endl;
        cin >> index;
        i = index - 1;
    }
        switch (choice) {
            case 1:
                removeUtilInstance(i);
                if (uList.empty()) return 0;
                else break;
            case 2: {
                recoverData(i);
                break;
            }
            case 3:
                showing_menu = false;
                break;
            default:
                cout << "Error. Choose again." << endl;
                break;
        }
    }
    return 0;
}

void writeListToFile(const vector<UtilInstance>& instances, const string& filename) {
    ofstream outFile(filename, ios::binary);
    if (!outFile) {
        cerr << "Cannot open file: " << filename << endl;
        return;
    }

    // Записываем количество элементов в списке
    size_t count = instances.size();
    outFile.write(reinterpret_cast<const char*>(&count), sizeof(count));

    // Записываем каждый объект в файл
    for (const auto& instance : instances) {
        instance.serialize(outFile);
    }

    outFile.close();
    cout << "File saved: " << filename << endl;
}

vector<UtilInstance> readListFromFile(const string& filename) {
    vector<UtilInstance> instances;
    ifstream inFile(filename, ios::binary);
    if (!inFile) {
        cerr << "Cannot open file: " << filename << endl;
        return instances;
    }

    // Читаем количество элементов в списке
    size_t count;
    inFile.read(reinterpret_cast<char*>(&count), sizeof(count));

    // Читаем каждый объект из файла
    for (size_t i = 0; i < count; ++i) {
        UtilInstance instance;
        instance.deserialize(inFile);
        instances.push_back(instance);
    }

    inFile.close();
    cout << "File read: " << filename << endl;
    return instances;
}

void mainMenu ()
{
    int choice;
    bool running = true;

    while (running) {
        cout << "Menu:" << endl;
        cout << "1. Choose directory" << endl;
        cout << "2. List" << endl;
        cout << "3. Quit" << endl;
        cout << "Choose an option: ";
        cin >> choice;
        switch (choice) {
            case 1:
                newUtilInst();
                break;
            case 2: {
                showList();
                break;
            }
            case 3:
                cout << "Quitting..." << endl;
                running = false;
                break;
            default:
                cout << "Error. Choose again." << endl;
                break;
        }
    }
}

int main () {
    const string filelist = "dir_list.bin";
    if (fs::exists(filelist)) 
    { 
        uList = readListFromFile(filelist);
        // ДОБАВИТЬ ПРОВЕРКУ с HANDLE
        for (size_t i = 0; i < uList.size(); ++i) {

            contUtilInst(i, uList[i].getSourcePath(), uList[i].getBackupPath(), uList[i].getMaxCopies());
        } 
        writeListToFile(uList, filelist);
    }
    mainMenu();
    writeListToFile(uList, filelist);
    return 0;
}