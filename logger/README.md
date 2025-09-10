# Logger 库

`Logger` 是一个基于 Android 和 mmap 的高性能日志库，支持异步写入、持久化存储以及 Logcat
输出，适合对日志存储和追踪有较高要求的应用。

## 功能特点

- **多级日志**：支持 `Verbose / Debug / Info / Warn / Error`。
- **双通道存储**：
    - `mainRegion`：存储应用业务日志。
    - `logcatRegion`：存储 Logcat 输出。
- **异步写入**：后台线程异步写入文件，避免阻塞主线程。
- **自动管理日志文件**：支持缓存文件大小限制与历史文件清理。
- **日志头信息**：包含应用包名、版本号、进程名、设备信息和启动时间。
- **Logcat 输出**：兼容系统 Logcat，同时避免重复记录。

## 使用示例

```java
// 初始化 Logger
Logger.init(context);

// 打印日志
Logger.d("MainActivity","Debug message");
Logger.i("Network","Request success");
Logger.e("Database","Insert failed");

// 手动刷新日志到文件
Logger.flush();

// 释放资源
Logger.release();
```

## 实现原理

- 通过 MmapRegion 将日志写入内存映射文件，提高写入效率。

- 异步写入由 AsyncWriter 负责，主线程调用 write 方法时不会阻塞。

- 每条日志记录时间戳和日志级别，并自动追加到日志文件。

- ZERO_WIDTH_SPACE 避免 Logcat 重复读取同一条日志。

- 日志文件会自动管理大小，超出限制时删除旧文件。

## 依赖

- Android 5.0+（API 21）

- MmapRegion 日志缓存模块

- C++ mmap 支持（用于底层文件映射）

## 注意事项

- 请在 Application 或主进程中调用 Logger.init 进行初始化。

- 异步写入可能存在延迟，关键日志建议同时调用 flush。

- 使用完成后务必调用 Logger.release 释放资源，防止内存泄漏。
