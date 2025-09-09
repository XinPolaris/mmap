//
// Created by huangxina on 2025/9/9.
//
#include "AsyncWriter.h"

AsyncWriter::AsyncWriter() : stopFlag(false) {
    worker = std::thread([this]() { this->run(); });
}

AsyncWriter::~AsyncWriter() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        stopFlag = true;
    }
    cv.notify_one();
    if (worker.joinable()) worker.join();
}

void AsyncWriter::postTask(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mtx);
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void AsyncWriter::run() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this]() { return stopFlag || !tasks.empty(); });
            if (stopFlag && tasks.empty()) break;
            task = std::move(tasks.front());
            tasks.pop();
        }
        task(); // 执行 write 或 flush
    }
}
