package com.huaxi.dev.log;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Process;
import android.util.Log;

import com.huaxi.dev.mmap.MmapRegion;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Objects;

/**
 * Created by HuangXin on 2025/9/9.
 */
public class Logger {

    public static final String ZERO_WIDTH_SPACE = "\u200B";
    private static MmapRegion mainRegion;
    private static MmapRegion logcatRegion;

    // 初始化日志文件
    public static void init(Context context) {
        try {
            mainRegion = new MmapRegion(Objects.requireNonNull(context.getExternalFilesDir("main")).getAbsolutePath(), 50 * 1024 * 1024);
            logcatRegion = new MmapRegion(Objects.requireNonNull(context.getExternalFilesDir("logcat")).getAbsolutePath(), 50 * 1024 * 1024);
            LogcatLogger.INSTANCE.start(line -> {
                if (logcatRegion != null) {
                    logcatRegion.write(line);
                }
            });
            writeHeader(context);
        } catch (NullPointerException e) {
            //noinspection CallToPrintStackTrace
            e.printStackTrace();
        }
    }

    // Verbose
    public static void v(String tag, String msg) {
        write("V", tag, msg);
    }

    // Debug
    public static void d(String tag, String msg) {
        write("D", tag, msg);
    }

    // Info
    public static void i(String tag, String msg) {
        write("I", tag, msg);
    }

    // Warn
    public static void w(String tag, String msg) {
        write("W", tag, msg);
    }

    // Error
    public static void e(String tag, String msg) {
        write("E", tag, msg);
    }

    // Flush 方法
    public static void flush() {
        if (mainRegion != null) {
            mainRegion.flush();
        }
        if (logcatRegion != null) {
            logcatRegion.flush();
        }
    }

    // 关闭释放资源
    public static void release() {
        if (mainRegion != null) {
            mainRegion.release();
            mainRegion = null;
        }
        LogcatLogger.INSTANCE.stop();
        if (logcatRegion != null) {
            logcatRegion.release();
            logcatRegion = null;
        }
    }

    private static void writeHeader(Context context) {
        if (mainRegion == null) return;

        // 包名
        String packageName = context.getPackageName();

        // 应用版本号、版本名
        String versionName = "unknown";
        int versionCode = -1;
        try {
            PackageInfo pi = context.getPackageManager().getPackageInfo(packageName, 0);
            versionName = pi.versionName;
            versionCode = (int) (android.os.Build.VERSION.SDK_INT >= 28 ? pi.getLongVersionCode() : pi.versionCode);
        } catch (PackageManager.NameNotFoundException ignored) {
        }

        // 进程名
        String processName = null; // API 28+ 可用
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            processName = Application.getProcessName();
        }
        if (processName == null) processName = packageName;

        // 时间戳
        String time = dateFormat.format(new Date());

        // 设备信息
        String deviceInfo = Build.MANUFACTURER + " " + Build.MODEL + " / Android " + Build.VERSION.RELEASE;

        // 拼接头部
        String header =
                "---------------------------- PROCESS STARTED ----------------------------\n" +
                        "Time       : " + time + "\n" +
                        "PID        : " + Process.myPid() + "\n" +
                        "Process    : " + processName + "\n" +
                        "Package    : " + packageName + "\n" +
                        "Version    : " + versionName + " (" + versionCode + ")\n" +
                        "Device     : " + deviceInfo + "\n" +
                        "-------------------------------------------------------------------------\n";

        mainRegion.write(header);
    }


    private static final SimpleDateFormat dateFormat = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS", Locale.CHINA);

    // 内部写日志
    private static void write(String level, String tag, String msg) {
        if (mainRegion == null) return;
        String time = dateFormat.format(new Date());
        String log = time + " " + level + "/" + tag + ": " + msg + "\n";
        mainRegion.write(log);
        // 再输出到 logcat
        msg += ZERO_WIDTH_SPACE; // 避免[LogcatReader]重复存储
        switch (level) {
            case "V":
                Log.v(tag, msg);
                break;
            case "D":
                Log.d(tag, msg);
                break;
            case "I":
                Log.i(tag, msg);
                break;
            case "W":
                Log.w(tag, msg);
                break;
            case "E":
                Log.e(tag, msg);
                break;
            default:
                break;
        }
    }
}
