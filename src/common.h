#ifndef NODE_PRINTER_COMMON_H
#define NODE_PRINTER_COMMON_H

#include <node_api.h>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#else
#include <cups/cups.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// ============================================================================
// Constants
// ============================================================================

static const size_t MAX_PRINTER_NAME_LENGTH = 256;

// List of virtual printers that should be blocked (case-insensitive check)
static const std::vector<std::string> BLOCKED_VIRTUAL_PRINTERS = {
    "microsoft print to pdf",
    "microsoft xps document writer",
    "onenote",
    "fax",
    "send to onenote",
    "adobe pdf",
    "cute pdf",
    "cutepdf",
    "bullzip pdf",
    "foxit pdf",
    "pdf24",
    "dopdf",
    "pdfcreator"
};

// ============================================================================
// Error Codes - Single source of truth
// ============================================================================

enum PrinterErrorCodes {
    PRINTER_SUCCESS = 0,
    PRINTER_INVALID_ARGUMENT = 1000,
    PRINTER_OPEN_ERROR = 1001,
    PRINTER_START_DOC_ERROR = 1002,
    PRINTER_START_PAGE_ERROR = 1003,
    PRINTER_WRITE_ERROR = 1004,
    PRINTER_INCOMPLETE_WRITE = 1005,
    PRINTER_INVALID_NAME = 1006,
    PRINTER_OTHER_ERROR = 1007,
    PRINTER_VIRTUAL_BLOCKED = 1008
};

// ============================================================================
// NAPI Helper Macro
// ============================================================================

#define NAPI_CALL(env, call)                                      \
  do                                                              \
  {                                                               \
    napi_status status = (call);                                  \
    if (status != napi_ok)                                        \
    {                                                             \
      const napi_extended_error_info *error_info = NULL;          \
      napi_get_last_error_info((env), &error_info);               \
      bool is_pending;                                            \
      napi_is_exception_pending((env), &is_pending);              \
      if (!is_pending)                                            \
      {                                                           \
        const char *message = (error_info->error_message == NULL) \
                                  ? "empty error message"         \
                                  : error_info->error_message;    \
        napi_throw_error((env), NULL, message);                   \
        return NULL;                                              \
      }                                                           \
    }                                                             \
  } while (0)

// ============================================================================
// Utility Functions
// ============================================================================

inline std::string toLowercase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

inline bool isBlockedVirtualPrinter(const std::string& printerName) {
    std::string lowerName = toLowercase(printerName);
    for (const auto& blocked : BLOCKED_VIRTUAL_PRINTERS) {
        if (lowerName.find(blocked) != std::string::npos) {
            return true;
        }
    }
    return false;
}

// Helper function to extract printer name from JS argument
inline bool GetPrinterNameFromArg(napi_env env, napi_value arg, std::string& printer_name) {
    napi_valuetype valuetype;
    napi_typeof(env, arg, &valuetype);

    if (valuetype != napi_string) {
        return false;
    }

    size_t str_size;
    napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);

    if (str_size > MAX_PRINTER_NAME_LENGTH) {
        return false;
    }

    printer_name.resize(str_size);
    size_t str_size_read;
    napi_get_value_string_utf8(env, arg, &printer_name[0], str_size + 1, &str_size_read);

    if (!printer_name.empty() && printer_name.back() == '\0') {
        printer_name.pop_back();
    }

    return true;
}

// ============================================================================
// Shared Data Structures
// ============================================================================

struct PrinterInfo {
    std::string name;
    bool isDefault;
    std::string status;
};

struct OperationResult {
    bool success;
    int errorCode;
    std::string errorMessage;

    OperationResult() : success(true), errorCode(0) {}

    void setError(int code, const std::string& message) {
        success = false;
        errorCode = code;
        errorMessage = message;
    }
};

// ============================================================================
// Function Declarations (implemented in separate files)
// ============================================================================

// printers.cc
napi_value GetAvailablePrinters(napi_env env, napi_callback_info info);

// cashdrawer.cc
napi_value OpenCashDrawer(napi_env env, napi_callback_info info);

// Export error codes as JS object
napi_value GetErrorCodes(napi_env env);

#endif // NODE_PRINTER_COMMON_H
