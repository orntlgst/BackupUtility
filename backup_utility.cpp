#include <windows.h>
#include <iostream>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

std::string replaceDoubleBackslashes(const std::string& input) {
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

class BackupUtility {
public:
    BackupUtility(const std::string& source, const std::string& backup, size_t maxCopies)
        : sourceDir(source), backupDir(backup), maxCopies(maxCopies++) {}

    void start() {
        if (!fs::exists(sourceDir)) {
            std::cerr << "Ошибка: Исходный каталог не существует." << std::endl;
            return;
        }
        
        // Создаем полную копию при запуске
        createBackup(true);
        
        // Запуск отслеживания изменений в каталоге
        watchDirectory();
    }

private:
    std::string sourceDir;
    std::string backupDir;
    size_t maxCopies;

    int createBackup(bool fullBackup) {
        std::string timestamp = getTimestamp();

        sourceDir = replaceDoubleBackslashes(sourceDir);
        backupDir = replaceDoubleBackslashes(backupDir);

        try {

            if (fullBackup) {
                // Полное копирование
                std::string s = getFullBackup();
                if (s != "") return 0;
                std::string newBackupDir = backupDir + "\\full_backup_" + timestamp;
                fs::create_directories(newBackupDir);
                for (const auto& entry : fs::recursive_directory_iterator(sourceDir)) {
                    fs::path relativePath = fs::relative(entry.path(), sourceDir);
                    fs::path destPath = newBackupDir / relativePath;
                    if (fs::is_directory(entry)) {
                        fs::create_directories(destPath);
                    } else {
                        fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                    }
                }
            } else {
                // Дифференциальное копирование
                std::string newBackupDir = backupDir + "\\backup_" + timestamp;
                fs::create_directories(newBackupDir);
                std::string fullBackupDir = getFullBackup();
                if (fullBackupDir.empty()) return 0;

                for (const auto& entry : fs::recursive_directory_iterator(sourceDir)) {
                    fs::path relativePath = fs::relative(entry.path(), sourceDir);
                    fs::path destPath = newBackupDir / relativePath;
                    fs::path lastBackupPath = fullBackupDir / relativePath;
                
                    if (fs::is_directory(entry)) {
                        // Создаем директорию в новой резервной копии, если она не существует
                        fs::create_directories(destPath);
                    } else {
                        // Проверяем, существует ли файл в последнем бэкапе
                        if (!fs::exists(lastBackupPath)) {
                            // Если файла нет в последнем бэкапе, копируем его
                            fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                        } else {
                            // Если файл существует, сравниваем время последнего изменения
                            if (fs::last_write_time(entry) > fs::last_write_time(lastBackupPath)) {
                                // Если файл был изменен, копируем его
                                fs::copy_file(entry.path(), destPath, fs::copy_options::overwrite_existing);
                            }
                        }
                    }
                }
            }

            cleanupOldBackups();

        } catch (const fs::filesystem_error& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
        return 0;
    }

    std::string getFullBackup() {
        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (fs::is_directory(entry)) {
                // Получаем имя папки
                std::string folderName = entry.path().filename().string();
    
                // Проверяем, начинается ли имя папки с "full_"
                if (folderName.rfind("full_", 0) == 0) { // rfind возвращает 0, если строка начинается с "full_"
                    return entry.path().string(); // Возвращаем путь к папке
                }
            }
        }
    
        // Если папка не найдена, возвращаем пустой путь
        std::string s = "";
        return s;
    }

    void cleanupOldBackups() {
        std::vector<fs::path> backups;
        for (const auto& entry : fs::directory_iterator(backupDir)) {
            if (fs::is_directory(entry)) {
                backups.push_back(entry.path());
            }
        }

        if (backups.size() <= maxCopies) return;

        std::sort(backups.begin(), backups.end(), [](const fs::path& a, const fs::path& b) {
            return fs::last_write_time(a) < fs::last_write_time(b);
        });

        while (backups.size() > maxCopies) {
            // Проверяем, начинается ли имя каталога на "full_"
            if (backups.front().filename().string().rfind("full_", 0) != 0) {
                // Удаляем каталог, если он не начинается на "full_"
                fs::remove_all(backups.front());
                backups.erase(backups.begin());
            } else {
                // Если каталог начинается на "full_", пропускаем его и переходим к следующему
                backups.erase(backups.begin());
            }
        }
    }

std::string getTimestamp() {
        SYSTEMTIME localTime;
        GetSystemTime(&localTime);
        std::ostringstream oss;
        oss << (localTime.wYear) << "-"
            << (localTime.wMonth) << "-"
            << localTime.wDay << "_"
            << localTime.wHour << "-"
            << localTime.wMinute << "-"
            << localTime.wSecond;
        return oss.str();
    }

    void watchDirectory() {
        HANDLE hDir = CreateFileA(
            sourceDir.c_str(),
            FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS,
            nullptr
        );

        if (hDir == INVALID_HANDLE_VALUE) {
            std::cerr << "Ошибка открытия каталога для мониторинга." << std::endl;
            return;
        }

        char buffer[1024];
        DWORD bytesReturned;
        FILE_NOTIFY_INFORMATION* notifyInfo;

        while (true) {
            if (ReadDirectoryChangesW(
                hDir, buffer, sizeof(buffer), TRUE,
                FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
                FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_SIZE,
                &bytesReturned, nullptr, nullptr
            )) {
                notifyInfo = reinterpret_cast<FILE_NOTIFY_INFORMATION*>(buffer);
                if (notifyInfo->Action == FILE_ACTION_ADDED ||
                    notifyInfo->Action == FILE_ACTION_MODIFIED ||
                    notifyInfo->Action == FILE_ACTION_REMOVED) {
                    Sleep(2000);
                    createBackup(false);
                }
            }
        }

        CloseHandle(hDir);
    }
};

int main(int argc, char *argv[]) {

    std::cout << "New item added to the watchlist!" << std::endl;

    BackupUtility backup(static_cast<std::string>(argv[1]), static_cast<std::string>(argv[2]), atol(argv[3]));
    backup.start();
    return 0;
}