package com.huaxi.dev.mmap;

public class MmapRegion {

    // Used to load the 'mmap' library on application startup.
    static {
        System.loadLibrary("mmap");
    }

    private long nativeHandle;

    public MmapRegion(String path, long size) {
        this(path, size, false);
    }

    public MmapRegion(String path, long size, boolean saveDaily) {
        nativeHandle = nativeCreate(path, size, saveDaily);
    }

    public void release() {
        nativeDestroy(nativeHandle);
        nativeHandle = 0;
    }

    private native long nativeCreate(String path, long size, boolean saveDaily);

    private native void nativeDestroy(long handle);

    public native void write(String data);

    public native void flush();
}