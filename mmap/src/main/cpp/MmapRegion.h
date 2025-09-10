#pragma once

#include <string>
#include <memory>
#include <cstddef>
#include <mutex>

class MmapRegion {
public:
    MmapRegion(const std::string &filePath, size_t size);

    ~MmapRegion();

    // 写入数据
    void write(const void *data, size_t size);

    // flush 到文件
    void flush();

private:
    bool createMapping();

    void destroyMapping();

    bool writeSync(const void *data, size_t size);

    bool flushSync();

    size_t maxFileSize = 10 * 1024 * 1024; // 单个文件大小10M
    const size_t maxCacheSize = 50 * 1024;     // 缓存大小50KB

    size_t maxTotalSize; // 总容量
    std::string bufferFile;   // 缓存路径
    std::string filesDir;     // 最终文件目录
    char *mmapPtr = nullptr;  // 内存映射指针
    size_t writeOffset = 0;   // 当前写入偏移
};
