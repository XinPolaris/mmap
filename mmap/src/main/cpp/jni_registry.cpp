#include <jni.h>
#include <string>
#include <cstdio>
#include "MmapRegion.h"
#include "Log.h"

using namespace std;

/// =====================
/// 1. 具体的 native 实现
/// =====================

// nativeCreate
jlong nativeCreate(JNIEnv *env, jobject /*obj*/, jstring jpath, jlong size, jboolean saveDaily) {
    if (!jpath) return 0; // 防止传入空字符串

    const char *cpath = env->GetStringUTFChars(jpath, nullptr);
    if (!cpath) return 0; // 内存不足情况

    LOGD("mmap path: %s", cpath);

    auto *region = new(std::nothrow) MmapRegion(cpath, size);
    env->ReleaseStringUTFChars(jpath, cpath);

    if (!region) return 0; // new 失败
    return reinterpret_cast<jlong>(region);
}

// nativeDestroy
void nativeDestroy(JNIEnv /**env*/, jobject /*obj*/, jlong handle) {
    if (handle == 0) return; // 忽略无效 handle

    auto *region = reinterpret_cast<MmapRegion *>(handle);
    delete region;
}

// 内部工具：取对象里的 handle 字段
static inline MmapRegion *getRegionFromObj(JNIEnv *env, jobject obj) {
    if (!obj) return nullptr;

    jclass clazz = env->GetObjectClass(obj);
    if (!clazz) return nullptr;

    jfieldID fid = env->GetFieldID(clazz, "nativeHandle", "J");
    if (!fid) return nullptr;

    jlong handle = env->GetLongField(obj, fid);
    if (handle == 0) {
        LOGE("Invalid nativeHandle field: %lld", (long long) handle);
        return nullptr;
    }

    return reinterpret_cast<MmapRegion *>(handle);
}

// write
void nativeWrite(JNIEnv *env, jobject obj, jstring data) {
    MmapRegion *region = getRegionFromObj(env, obj);
    if (!region) return; // 防止空指针

    const char *cstr = data ? env->GetStringUTFChars(data, nullptr) : nullptr;
    if (cstr) {
        region->write(cstr, strlen(cstr));
        env->ReleaseStringUTFChars(data, cstr);
    }
}

// flush
void nativeFlush(JNIEnv *env, jobject obj) {
    MmapRegion *region = getRegionFromObj(env, obj);
    if (!region) return;
    region->flush();
}

/// =====================
/// 2. 注册表：每个类一个方法数组
/// =====================

static JNINativeMethod gJniMethods[] = {
        {"nativeCreate",  "(Ljava/lang/String;JZ)J", (void *) nativeCreate},
        {"nativeDestroy", "(J)V",                    (void *) nativeDestroy},
        {"write",         "(Ljava/lang/String;)V",   (void *) nativeWrite},
        {"flush",         "()V",                     (void *) nativeFlush},
};

/// =====================
/// 3. 集中注册逻辑
/// =====================

struct JniClassInfo {
    const char *className;
    JNINativeMethod *methods;
    int methodCount;
};

static JniClassInfo gClasses[] = {
        {"com/huaxi/dev/mmap/MmapRegion", gJniMethods,
         sizeof(gJniMethods) / sizeof(gJniMethods[0])},
};

static bool registerAllNatives(JNIEnv *env) {
    for (auto &cls: gClasses) {
        jclass clazz = env->FindClass(cls.className);
        if (!clazz) return false;

        if (env->RegisterNatives(clazz, cls.methods, cls.methodCount) < 0) {
            return false;
        }
    }
    return true;
}

/// =====================
/// 4. JNI_OnLoad
/// =====================

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }
    if (!registerAllNatives(env)) {
        return JNI_ERR;
    }
    return JNI_VERSION_1_6;
}
