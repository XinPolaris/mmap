//
// Created by huangxina on 2025/9/8.
//

#pragma once
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

struct FileInfo {
    std::string path;
    long long mtimeNs;  // 纳秒级时间戳
    size_t size;
};

class FileUtils {
public:
    /**
     * @brief 确保文件存在，如果不存在就创建
     * @param path 文件路径
     * @param minSize 文件最小大小（字节），不足时自动扩展
     */
    static void ensureFileExists(const std::string& path);
    static void ensureDicExists(const std::string& path);
    static std::string generateMmapFileName(const std::string& dir);
    static std::vector<FileInfo> listFiles(const std::string &dir);
    static void deleteFilesWithLimit(const std::vector<FileInfo> &files, size_t maxTotalSize);
};

