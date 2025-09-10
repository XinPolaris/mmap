package com.huaxi.dev.log

import android.text.TextUtils
import java.io.BufferedReader
import java.io.InputStreamReader

/**
 *  Created by HuangXin on 2025/9/9.
 */
internal object LogcatLogger {
    private var readerThread: ReaderThread? = null

    fun start(callback: IReaderCallback) {
        readerThread?.quiteLog()
        readerThread = ReaderThread(callback)
        readerThread?.start()
    }

    fun stop() {
        readerThread?.quiteLog()
        readerThread = null
    }

    class ReaderThread(val callback: IReaderCallback) : Thread() {

        private var isQuit = false

        init {
            name = "LogcatLogger"
        }

        fun quiteLog() {
            isQuit = true
        }

        override fun run() {
            var process: Process? = null
            var reader: BufferedReader? = null
            try {
                val command = "logcat -v time *:I" // 指定格式
                process = Runtime.getRuntime().exec(command)
                reader = BufferedReader(InputStreamReader(process.inputStream))

                while (!isQuit) {
                    //readLine获取不到会一直阻塞，不耗资源
                    val line = reader.readLine() ?: break
                    if (!TextUtils.isEmpty(line)
                        && !line.endsWith(Logger.ZERO_WIDTH_SPACE)//剔除[Logger]打印的日志。不重复存储
                        && !line.contains("MMAP_REGION", false) //剔除mmap库打印的日志，防止死循环
                    ) {
                        callback.printLine(line + System.lineSeparator())
                    }
                }
            } catch (e: Exception) {
                e.printStackTrace()
            } finally {
                try {
                    reader?.close()
                } catch (ignored: Exception) {
                }
                process?.destroy()
            }
        }
    }

    interface IReaderCallback {
        fun printLine(line: String)
    }
}