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

constexpr uint32_t MAGIC = 0x4D4D4150; // 文件头部固定标识符，判断文件有效

struct FileInfo {
    std::string path;
    time_t mtime;
    size_t size;
};

struct MmapHeader {
    uint32_t magic;       // 魔数标识
    size_t writeOffset; // 当前写入到哪里
};


static std::vector<FileInfo> listFiles(const std::string &dir) {
    std::vector<FileInfo> files;
    DIR *dp = opendir(dir.c_str());
    if (!dp) return files;

    struct dirent *entry;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue; // 忽略隐藏文件
        std::string fullPath = dir + "/" + entry->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            files.push_back({fullPath, st.st_mtime, static_cast<size_t>(st.st_size)});
        }
    }
    closedir(dp);

    std::sort(files.begin(), files.end(),
              [](const FileInfo &a, const FileInfo &b) { return a.mtime < b.mtime; });
    return files;
}

MmapRegion::MmapRegion(const std::string &filePath, size_t maxSize)
        : bufferFile(filePath + "/cache.mmap"), filesDir(filePath + "/files"), maxSize(maxSize) {
    FileUtils::ensureFileExists(bufferFile);
    FileUtils::ensureDicExists(filesDir);
    createMapping(maxCacheSize);
}

MmapRegion::~MmapRegion() {
    flush();
    destroyMapping();
}


bool MmapRegion::createMapping(size_t size) {
    int fd = open(bufferFile.c_str(), O_RDWR | O_CREAT, 0666);
    if (fd < 0) {
        LOGE("open buffer file failed");
        return false;
    }

    // 扩展文件大小
    if (ftruncate(fd, size) < 0) {
        LOGE("ftruncate failed");
        close(fd);
        return false;
    }

    mmapPtr = static_cast<char *>(
            mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);

    if (mmapPtr == MAP_FAILED) {
        LOGE("mmap failed");
        mmapPtr = nullptr;
        return false;
    }

    auto *header = reinterpret_cast<MmapHeader *>(mmapPtr);
    if (header->magic != MAGIC) {
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
        mmapPtr = NULL;
    }
}

bool MmapRegion::write(const void *data, size_t size) {
    if (!mmapPtr) return false;

    if (writeOffset + size > maxCacheSize) {
        if (!flush()) return false;
        writeOffset = 0;
    }
    LOGD("write %d %s", writeOffset, data);
    memcpy(mmapPtr + writeOffset, data, size);
    writeOffset += size;

    // 更新头部写入位置
    auto *header = reinterpret_cast<MmapHeader *>(mmapPtr);
    header->writeOffset = writeOffset;
    msync(header, sizeof(MmapHeader), MS_SYNC);
    return true;
}

bool MmapRegion::flush() {
    if (!mmapPtr || writeOffset == 0) return true;

    const size_t maxFileSize = 10 * 1024 * 1024; // 10M
    const size_t maxTotalSize = 100 * 1024 * 1024; // 总容量 100M

    // 获取文件列表
    std::vector<FileInfo> files = listFiles(filesDir);

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
    }

    int fd = open(latestFile.c_str(), O_RDWR | O_CREAT | O_APPEND, 0666);
    if (fd < 0) {
        LOGE("open final file");
        return false;
    }

    size_t dataSize = writeOffset - sizeof(MmapHeader);
    if (dataSize <= 0) {
        close(fd);
        return true; // 没有数据可写
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
    size_t totalSize = 0;
    for (std::vector<FileInfo>::const_iterator it = files.begin(); it != files.end(); ++it) {
        totalSize += it->size;
    }

    for (std::vector<FileInfo>::const_iterator it = files.begin();
         it != files.end() && totalSize > maxTotalSize; ++it) {
        totalSize -= it->size;
        ::remove(it->path.c_str());
    }

    return true;
}
