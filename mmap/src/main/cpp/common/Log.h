//
// Created by huangxina on 2025/9/5.
//
/**
 * 头文件保护（include guard） 的简写，作用是：
✅ 保证头文件只会被编译器包含（include）一次，避免重复定义引起的编译错误。
 传统写法：
     #ifndef LOG_H
    #define LOG_H

    // 头文件内容

    #endif // LOG_H
 */
#pragma once

#include <android/log.h>

// 全局统一 TAG，可以按模块修改
#ifndef LOG_TAG
#define LOG_TAG "MmapRegion"
#endif

// 日志宏
#define LOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG,   LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,    LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN,    LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,   LOG_TAG, __VA_ARGS__)
