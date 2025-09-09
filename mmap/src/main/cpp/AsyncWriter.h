//
// Created by huangxina on 2025/9/9.
//
#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <atomic>

class AsyncWriter {
public:
    AsyncWriter();
    ~AsyncWriter();

    // 提交一个任务
    void postTask(std::function<void()> task);

private:
    void run();

    std::thread worker;
    std::mutex mtx;
    std::condition_variable cv;
    std::queue<std::function<void()>> tasks;
    std::atomic<bool> stopFlag;
};
