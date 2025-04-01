#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <io.h>
#include <fcntl.h>
#include <sstream>
#include <string>

using namespace std;

namespace fs = std::filesystem;

void mergeDirectories(const string& sourceDir, const string& targetDir)
{
    // Создаем целевую директорию, если её нет
    fs::create_directories(targetDir);

    for (const auto& entry : fs::directory_iterator(sourceDir))
    {
        const auto targetPath = fs::path(targetDir) / entry.path().filename();

        if (entry.is_directory())
        {
            // Рекурсивно обрабатываем подкаталоги
            mergeDirectories(entry.path().string(), targetPath.string());
        }
        else if (entry.is_regular_file())
        {
            bool shouldCopy = true;

            // Проверяем, существует ли файл в целевой директории
            if (fs::exists(targetPath))
            {
                // Получаем время последнего изменения для обоих файлов
                auto sourceTime = fs::last_write_time(entry);
                auto targetTime = fs::last_write_time(targetPath);

                // Копируем только если исходный файл новее
                shouldCopy = (sourceTime > targetTime);
            }

            if (shouldCopy)
            {
                try {
                    fs::copy(entry.path(), targetPath, fs::copy_options::overwrite_existing);
                }
                catch (const fs::filesystem_error& e) {
                    // Обработка ошибок копирования
                    continue;
                }
            }
        }
    }
}

int main()
{
    string FullBackup = "C:\\VSProjects\\Backup\\full_backup_2025-4-1_19-1-43";
    string DiffBackup = "C:\\VSProjects\\Backup\\backup_2025-4-1_19-2-5";
    string SourcePath = "C:\\VSProjects\\Backup\\Temp";

    mergeDirectories(FullBackup, SourcePath);
    mergeDirectories(DiffBackup, SourcePath);

}