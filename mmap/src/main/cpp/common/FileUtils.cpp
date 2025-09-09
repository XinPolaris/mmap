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
                LOGE("FileUtils::ensureFileExists failed to create file: $s", path.c_str());
                return;
            }
        }
    } catch (const fs::filesystem_error &e) {
        LOGE("FileUtils::ensureFileExists filesystem_error: $s", e.what());
    }
}

void FileUtils::ensureDicExists(const std::string &path) {
    try {
        if (!fs::exists(path)) {
            fs::create_directories(path);
        }
    } catch (const fs::filesystem_error &e) {
        LOGE("FileUtils::ensureDicExists filesystem_error: $s", e.what());
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
