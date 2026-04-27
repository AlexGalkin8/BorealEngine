#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <wrl.h>
#include <wchar.h>


static inline void assertImpl(const char* expr, const char* file, int line, const char* func, const char* fmt, ...)
{
    char msgbuf[1024];
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        vsnprintf_s(msgbuf, sizeof(msgbuf), _TRUNCATE, fmt, ap);
        va_end(ap);
    }
    else
    {
        msgbuf[0] = '\0';
    }

    char out[2048];
    if (msgbuf[0] != '\0')
    {
        sprintf_s(out, sizeof(out), "Assertion failed: %s\nMessage: %s\nAt: %s:%d (%s)\n", expr, msgbuf, file, line, func);
    }
    else
    {
        sprintf_s(out, sizeof(out), "Assertion failed: %s\nAt: %s:%d (%s)\n", expr, file, line, func);
    }

    OutputDebugStringA(out);

    DebugBreak();
}

static inline void assertImpl(const char* expr, const char* file, int line, const char* func, const wchar_t* fmt, ...)
{
    wchar_t msgbuf[1024];
    if (fmt)
    {
        va_list ap;
        va_start(ap, fmt);
        _vsnwprintf_s(msgbuf, sizeof(msgbuf) / sizeof(msgbuf[0]), _TRUNCATE, fmt, ap);
        va_end(ap);
    }
    else
    {
        msgbuf[0] = L'\0';
    }

    wchar_t out[2048];
    if (msgbuf[0] != L'\0')
    {
        swprintf_s(out, sizeof(out) / sizeof(out[0]), L"Assertion failed: %hs\nMessage: %ls\nAt: %hs:%d (%hs)\n", expr, msgbuf, file, line, func);
    }
    else
    {
        swprintf_s(out, sizeof(out) / sizeof(out[0]), L"Assertion failed: %hs\nAt: %hs:%d (%hs)\n", expr, file, line, func);
    }

    OutputDebugStringW(out);

    DebugBreak();
}

#ifdef _DEBUG

#define ASSERT(cond) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (const char*)nullptr); } } while (0)

#define ASSERT_MSG(cond, msg, ...) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); } } while (0)

#define ASSERT_FAIL(msg, ...) \
    do { assertImpl("", __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); } while (0)

#define ASSERT_MSG_RETURN(cond, retValue, msg, ...) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); return (retValue); } } while (0)

#define ASSERT_MSG_RETURN_VOID(cond, msg, ...) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); return; } } while (0)

#define ASSERT_MSG_RETURN_NULL(cond, msg, ...) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); return nullptr; } } while (0)

#define ASSERT_MSG_RETURN_EMPTY(cond, msg, ...) \
    do { if (!(cond)) { assertImpl(#cond, __FILE__, __LINE__, __func__, (msg), ##__VA_ARGS__); return {}; } } while (0)

#else // !_DEBUG

#define ASSERT(cond) (void(0))
#define ASSERT_MSG(cond, msg, ...) (void(0))
#define ASSERT_MSG_RETURN(cond, retValue, msg, ...) \
    do { if (!(cond)) { return (retValue); } } while (0)
#define ASSERT_MSG_RETURN_VOID(cond, msg, ...) \
    do { if (!(cond)) { return; } } while (0)
#define ASSERT_MSG_RETURN_NULL(cond, msg, ...) \
    do { if (!(cond)) { return nullptr; } } while (0)
#define ASSERT_MSG_RETURN_EMPTY(cond, msg, ...) \
    do { if (!(cond)) { return {}; } } while (0)
#endif // _DEBUG