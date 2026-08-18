#include <stdio.h>
#include <string.h>

#include "file_utils.h"
#include "json.h"
#include "logger.h"
#include "platform_detector.h"

/* --smoke-test：CI 无头冒烟自检（日志/平台/JSON/SHA-1 全链路） */
static int smoke_test(void) {
    char buf[512];
    mkl_json_get_string("{\"a\":{\"b\":\"ok\"}}", "a.b", buf, sizeof(buf), "fail");
    if (strcmp(buf, "ok") != 0) {
        fprintf(stderr, "smoke: json check failed\n");
        return 1;
    }

    char tpath[512];
    char fpath[1024];
    char sha[41];
    mkl_file_temp_directory(tpath, sizeof(tpath));
    const char* parts[2] = {tpath, "mkl_smoke_abc.txt"};
    mkl_file_join_path(parts, 2, fpath, sizeof(fpath));
    if (mkl_file_write_all_text(fpath, "abc") != 0) {
        fprintf(stderr, "smoke: write failed\n");
        return 1;
    }
    if (mkl_file_sha1(fpath, sha) != 0 ||
        strcmp(sha, "a9993e364706816aba3e25717850c26c9cd0d89d") != 0) {
        fprintf(stderr, "smoke: sha1 check failed (%s)\n", sha);
        mkl_file_delete(fpath);
        return 1;
    }
    mkl_file_delete(fpath);

    printf("MKL_SMOKE_OK\n");
    return 0;
}

static const char* os_name(mkl_os_type os) {
    switch (os) {
        case MKL_OS_WINDOWS: return "Windows";
        case MKL_OS_MACOS: return "macOS";
        case MKL_OS_LINUX: return "Linux";
        case MKL_OS_ANDROID: return "Android";
        case MKL_OS_IOS: return "iOS";
        default: return "Unknown";
    }
}

int main(int argc, char** argv) {
    int smoke = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--smoke-test") == 0) {
            smoke = 1;
        }
    }

    /* 日志：写入用户数据目录 mkl/logs/MKL.log（失败则降级仅控制台） */
    char appdata[512];
    char logs_dir[1024];
    char log_path[1100];
    mkl_file_appdata_directory(appdata, sizeof(appdata));
    const char* dir_parts[3] = {appdata, "mkl", "logs"};
    mkl_file_join_path(dir_parts, 3, logs_dir, sizeof(logs_dir));
    mkl_file_create_directories(logs_dir);
    const char* file_parts[2] = {logs_dir, "MKL.log"};
    mkl_file_join_path(file_parts, 2, log_path, sizeof(log_path));
    mkl_logger_init(log_path);
    mkl_logger_set_console(1);

    MKL_INFO("MKL v0.0.2-alpha (R2) 启动");

    mkl_platform_info info;
    mkl_platform_detect(&info);
    MKL_INFO("平台: %s 版本=%s 架构=%s%s", os_name(info.os), info.os_version, info.arch,
             info.glibc_version[0] ? " glibc=" : "");
    if (info.glibc_version[0]) {
        MKL_INFO("glibc 版本: %s", info.glibc_version);
    }

    int rc = smoke ? smoke_test() : 0;
    if (smoke) {
        MKL_INFO("冒烟自检返回码 %d", rc);
    } else {
        MKL_INFO("R2 骨架 CLI 运行完成（无 UI，等待原生壳里程碑）");
    }
    mkl_logger_flush();
    mkl_logger_shutdown();
    return rc;
}
