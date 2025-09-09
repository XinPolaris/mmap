#pragma once

#include <string>
#include <memory>
#include <cstddef>

class MmapRegion {
public:
    MmapRegion(const std::string &filePath, size_t maxSize = 50 * 1024 * 1024); // 默认 50MB
    ~MmapRegion();

    // 写入数据
    bool write(const void *data, size_t size);

    // flush 到文件
    bool flush();

private:
    bool createMapping(size_t size);

    void destroyMapping();

    size_t maxSize;
    std::string bufferFile;   // 缓存路径
    size_t maxCacheSize = 50 * 1024;     // 缓存最大大小
    std::string filesDir;     // 最终文件目录
    char *mmapPtr = nullptr;  // 内存映射指针
    size_t writeOffset = 0;   // 当前写入偏移
};
