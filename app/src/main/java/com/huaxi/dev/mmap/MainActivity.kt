package com.huaxi.dev.mmap

import android.os.Bundle
import android.os.Process
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.huaxi.dev.mmap.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    val mmapRegion by lazy { MmapRegion(getExternalFilesDir("main")?.absolutePath, 50 * 1024) }
//    val mmapRegion2 by lazy { MmapRegion(getExternalFilesDir("logcat")?.absolutePath, 50 * 1024) }

    private var testCount = 0

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.write.setOnClickListener {
            write()
        }
        binding.flush.setOnClickListener {
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
        mmapRegion.write("${Process.myPid()} $this log $testCount\n")
        Log.i(TAG, "write cost: ${System.currentTimeMillis() - startTime} ms")
//        mmapRegion2.write("${Process.myPid()} $this logcat $testCount\n")
        testCount++
    }

    private fun flush() {
        val startTime = System.currentTimeMillis()
        mmapRegion.flush()
        Log.i(TAG, "flush cost: ${System.currentTimeMillis() - startTime} ms")
    }

    companion object {
        private const val TAG = "MainActivity"
    }

}