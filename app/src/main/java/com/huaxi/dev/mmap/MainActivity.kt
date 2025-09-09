package com.huaxi.dev.mmap

import android.os.Bundle
import androidx.appcompat.app.AppCompatActivity
import com.huaxi.dev.mmap.databinding.ActivityMainBinding

class MainActivity : AppCompatActivity() {

    private var testCount = 0

    private lateinit var binding: ActivityMainBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val mmapRegion =
            MmapRegion(getExternalFilesDir("main")?.absolutePath, 10 * 1024, true)

        binding.write.setOnClickListener {
            mmapRegion.write("$this log $testCount\n")
            testCount++
        }
        binding.flush.setOnClickListener {
            mmapRegion.flush()
        }
    }

}