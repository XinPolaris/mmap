//
// Created by huangxina on 2025/9/8.
//
#include "FileUtils.h"
#include "Log.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <vector>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <cstddef>

void FileUtils::ensureFileExists(const std::string &path) {
    try {
        // 创建父目录（如果不存在）
        if (auto parent = fs::path(path).parent_path(); !parent.empty() && !fs::exists(parent)) {
            fs::create_directories(parent);
        }

        // 如果文件不存在，创建空文件
        if (!fs::exists(path)) {
            std::ofstream ofs(path, std::ios::binary);
            if (!ofs) {
                LOGE("FileUtils::ensureFileExists failed to create file: %s", path.c_str());
                return;
            }
        }
    } catch (const fs::filesystem_error &e) {
        LOGE("FileUtils::ensureFileExists filesystem_error: %s", e.what());
    }
}

void FileUtils::ensureDicExists(const std::string &path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    } catch (const fs::filesystem_error &e) {
        LOGE("FileUtils::ensureDicExists filesystem_error: %s", e.what());
    }
}

std::string FileUtils::generateMmapFileName(const std::string &dir) {
    using namespace std::chrono;

    // 获取当前时间点
    auto now = system_clock::now();

    // 拆分出秒和毫秒
    auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&t, &tm);

    // 格式化成 YYYYMMDD_HHMMSS_mmm
    std::ostringstream oss;
    oss << dir << "/"
        << std::put_time(&tm, "%Y%m%d_%H%M%S")
        << "_" << std::setfill('0') << std::setw(3) << ms.count()
        << ".mmap";

    return oss.str();
}

std::vector<FileInfo> FileUtils::listFiles(const std::string &dir) {
    std::vector<FileInfo> files;
    DIR *dp = opendir(dir.c_str());
    if (!dp) return files;

    struct dirent *entry;
    while ((entry = readdir(dp)) != nullptr) {
        if (entry->d_name[0] == '.') continue; // 忽略隐藏文件
        std::string fullPath = dir + "/" + entry->d_name;
        struct stat st{};
        if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            //文件修改时间，纳秒级
#if defined(__linux__)// 如果是在 Linux 平台（包括 Android）
            long long mtimeNs =
                    static_cast<long long>(st.st_mtim.tv_sec) * 1'000'000'000LL +
                    st.st_mtim.tv_nsec;
#else
            long long mtimeNs = static_cast<long long>(st.st_mtime) * 1'000'000'000LL;
#endif
            files.push_back({fullPath, mtimeNs, static_cast<size_t>(st.st_size)});
        }
    }
    closedir(dp);

    std::sort(files.begin(), files.end(),
              [](const FileInfo &a, const FileInfo &b) { return a.mtimeNs < b.mtimeNs; });

    //    LOGD("File list (%zu files):", files.size());
//    for (const auto &file : files) {
//        // 格式化时间
//        std::tm *tm_info = std::localtime(&file.mtime);
//        char buf[32];
//        if (tm_info) {
//            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
//        } else {
//            std::snprintf(buf, sizeof(buf), "N/A");
//        }
//
//        LOGD("  path=%s, mtime=%s, size=%zu", file.path.c_str(), buf, file.size);
//    }
    return files;
}

void FileUtils::deleteFilesWithLimit(const std::vector<FileInfo> &files, size_t maxTotalSize) {
    size_t totalSize = 0;
    for (const auto &file: files)
        totalSize += file.size;


    for (auto it = files.begin(); it != files.end() && totalSize > maxTotalSize; ++it) {
        totalSize -= it->size;
        ::remove(it->path.c_str());
    }
}
