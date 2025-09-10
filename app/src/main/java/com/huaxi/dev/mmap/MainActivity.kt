package com.huaxi.dev.mmap

import android.os.Bundle
import android.os.Process
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.huaxi.dev.log.Logger
import com.huaxi.dev.mmap.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private var testCount = 0

    //行为日志
    private val trackRegion by lazy {
        MmapRegion(getExternalFilesDir("track")?.getAbsolutePath(), 10 * 1024 * 1024)
    }

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.write.setOnClickListener {
            track("user click write")
            write()
        }
        binding.flush.setOnClickListener {
            track("user click flush")
            flush()
        }
        Thread({
            while (true) {
                Thread.sleep(100)
                write()
            }
        }).start()
    }

    private fun write() {
        val startTime = System.currentTimeMillis()
        Logger.i(TAG, "${Process.myPid()} $this write: $testCount")
        Logger.i(TAG, "write cost: ${System.currentTimeMillis() - startTime} ms")
        Log.i(TAG, "${Process.myPid()} logcat: $testCount")
        testCount++
    }

    private fun flush() {
        val startTime = System.currentTimeMillis()
        trackRegion.flush()
        Logger.flush()
        Log.i(TAG, "flush cost: ${System.currentTimeMillis() - startTime} ms")
    }

    private fun track(msg: String) {
        trackRegion.write("${System.currentTimeMillis()}: $this $msg\n")
    }

    companion object {
        private const val TAG = "MainActivity"
    }

}