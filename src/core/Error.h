#pragma once

#include <string>
#include <utility>

namespace mkl {

// 错误码（规格 2.5：错误必须包含上下文：文件、函数、行号、系统错误码）
enum class ErrorCode {
    OK = 0,
    INVALID_OPTIONS,
    NOT_FOUND,
    IO_ERROR,
    NETWORK_ERROR,
    DOWNLOAD_FAILED,
    VERIFICATION_FAILED,
    JAVA_NOT_FOUND,
    PROCESS_FAILED,
    AUTH_FAILED,
    STORAGE_ERROR,
    PLUGIN_ERROR,
    UPDATE_FAILED,
    UNSUPPORTED,
    UNKNOWN
};

struct Error {
    ErrorCode code = ErrorCode::UNKNOWN;
    std::string message;
    std::string file;
    std::string function;
    int line = 0;
    int systemErrorCode = 0;
};

// Result<T>：数据层/服务层所有可能失败操作的标准返回类型
template <typename T>
class Result {
public:
    static Result ok(T value) {
        Result r;
        r.m_ok = true;
        r.m_value = std::move(value);
        return r;
    }
    static Result fail(const Error& error) {
        Result r;
        r.m_ok = false;
        r.m_error = error;
        return r;
    }
    bool isOk() const { return m_ok; }
    bool isError() const { return !m_ok; }
    const T& value() const { return m_value; }
    const Error& error() const { return m_error; }

private:
    Result() = default;
    bool m_ok = false;
    T m_value{};
    Error m_error;
};

} // namespace mkl

// 便捷错误构造（自动填充文件/函数/行号）
#define MKL_ERR(code, msg) \
    ::mkl::Error{code, msg, __FILE__, __FUNCTION__, __LINE__, 0}
