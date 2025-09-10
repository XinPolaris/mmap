package com.huaxi.dev.mmap

import com.huaxi.dev.log.Logger

/**
 *  Created by HuangXin on 2025/9/9.
 */
class App : android.app.Application() {
    override fun onCreate() {
        super.onCreate()
        Logger.init(this)
    }
}