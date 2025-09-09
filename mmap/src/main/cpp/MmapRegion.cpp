#include "MmapRegion.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <dirent.h>
#include <algorithm>
#include <vector>
#include <ctime>
#include "Log.h"
#include "FileUtils.h"
#include "AsyncWriter.h"

constexpr uint32_t MAGIC = 0x58415548; // 文件头部固定标识符，判断文件有效

struct MmapHeader {
    uint32_t magic;       // 魔数标识
    size_t writeOffset; // 记录当前写入位置
};

MmapRegion::MmapRegion(const std::string &filePath, size_t size)
        : bufferFile(filePath + "/cache.mmap"), filesDir(filePath + "/files"), maxTotalSize(size) {
    FileUtils::ensureFileExists(bufferFile);
    FileUtils::ensureDicExists(filesDir);
    createMapping();
}

MmapRegion::~MmapRegion() {
    flush();
    destroyMapping();
}

bool MmapRegion::createMapping() {
    int fd = open(bufferFile.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        LOGE("open buffer file failed");
        return false;
    }

    // 扩展文件大小。如果文件原来小于 size → 会扩展文件，新增部分通常填 0；如果文件原来大于 size → 会截断文件，多余数据丢弃
    if (ftruncate(fd, static_cast<off_t>(maxCacheSize)) < 0) {
        LOGE("ftruncate failed");
        close(fd);
        return false;
    }

    mmapPtr = static_cast<char *>(
            mmap(nullptr, maxCacheSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    if (mmapPtr == MAP_FAILED) {
        LOGE("mmap failed");
        mmapPtr = nullptr;
        return false;
    }

    auto *header = reinterpret_cast<MmapHeader *>(mmapPtr);
    if (header->magic != MAGIC) {
        // 清空缓冲区
        std::memset(mmapPtr, 0, maxCacheSize);
        // 文件是新建的，初始化头部
        header->magic = MAGIC;
        header->writeOffset = sizeof(MmapHeader);
        msync(header, sizeof(MmapHeader), MS_SYNC);
    }

    writeOffset = header->writeOffset;
    return true;
}

void MmapRegion::destroyMapping() {
    if (mmapPtr) {
        munmap(mmapPtr, maxCacheSize);
        mmapPtr = nullptr;
    }
}

bool MmapRegion::writeSync(const void *data, size_t size) {
    if (!mmapPtr) {
        LOGE("mmapPtr is null");
        return false;
    }
    if (size > maxCacheSize) {
        LOGE("data size %zu exceeds max cache size %zu", size, maxCacheSize);
        return false;
    }
    if (writeOffset + size > maxCacheSize) {
        LOGD("writing out of mmap bounds, flushing first");
        if (!flushSync()) {
            LOGE("writing flush failed");
            return false;
        }
        writeOffset = sizeof(MmapHeader);
    }
    memcpy(mmapPtr + writeOffset, data, size);
    writeOffset += size;
    // 更新头部写入位置
    auto *header = reinterpret_cast<MmapHeader *>(mmapPtr);
    header->writeOffset = writeOffset;
    msync(header, sizeof(MmapHeader), MS_SYNC);
    return true;
}

bool MmapRegion::flushSync() {
    if (!mmapPtr || writeOffset == 0) return false;

    size_t dataSize = writeOffset - sizeof(MmapHeader);
    if (dataSize <= 0) {
        LOGW("no data to write");
        return false; // 没有数据可写
    }

    // 获取文件列表
    std::vector<FileInfo> files = FileUtils::listFiles(filesDir);

    // 找最新文件
    std::string latestFile;
    size_t latestSize = 0;
    if (!files.empty()) {
        latestFile = files.back().path;
        latestSize = files.back().size;
    }

    // 没有就新建文件
    if (latestFile.empty() || latestSize >= maxFileSize) {
        latestFile = FileUtils::generateMmapFileName(filesDir);
        LOGD("create new file %s %d", latestFile.c_str(), latestSize);
    }

    int fd = open(latestFile.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        LOGE("open final file");
        return false;
    }

    ssize_t written = ::write(fd, mmapPtr + sizeof(MmapHeader), dataSize);
    if (written < 0) {
        LOGE("write final file fail: %s", strerror(errno));
        close(fd);
        return false;
    }
    close(fd);

    // 清空缓冲区
    std::memset(mmapPtr, 0, maxCacheSize);
    writeOffset = sizeof(MmapHeader);
    // 重新初始化缓存文件头部
    auto *header = reinterpret_cast<MmapHeader *>(mmapPtr);
    header->magic = MAGIC;
    header->writeOffset = writeOffset;
    msync(header, sizeof(MmapHeader), MS_SYNC);

    // 删除超量文件
    FileUtils::deleteFilesWithLimit(files, maxTotalSize);

    return true;
}

static AsyncWriter asyncWriter; // 单例后台线程

void MmapRegion::write(const void *data, size_t size) {
    asyncWriter.postTask([this, data, size]() {
        this->writeSync(data, size);
    });
}

void MmapRegion::flush() {
    asyncWriter.postTask([this]() {
        this->flushSync();
    });
}
