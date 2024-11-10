#include <node_api.h>
#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>

using namespace std;

// Define the NAPI_CALL macro
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


// Structure to hold the result and error information
struct DrawerResult {
    bool success;
    int errorCode;
    std::string errorMessage;
    
    DrawerResult() : success(true), errorCode(0) {}
    
    void setError(int code, const std::string& message) {
        success = false;
        errorCode = code;
        errorMessage = message;
    }
};

// Custom error codes
enum DrawerErrorCodes {
    DRAWER_SUCCESS = 0,
    DRAWER_PRINTER_OPEN_ERROR = 1001,
    DRAWER_START_DOC_ERROR = 1002,
    DRAWER_START_PAGE_ERROR = 1003,
    DRAWER_WRITE_ERROR = 1004,
    DRAWER_INCOMPLETE_WRITE = 1005
};

DrawerResult open_cash_drawer(const std::string& printerName) {
    DrawerResult result;
    HANDLE hPrinter;
    
    // Attempt to open the printer
    if (!OpenPrinterA(const_cast<char*>(printerName.c_str()), &hPrinter, NULL)) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "Failed to open printer. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    // Set up print job information
    DOC_INFO_1A docInfo;
    docInfo.pDocName = const_cast<LPSTR>("Open Cash Drawer");
    docInfo.pOutputFile = nullptr;
    docInfo.pDatatype = const_cast<LPSTR>("RAW");

    // Start a print job
    if (StartDocPrinterA(hPrinter, 1, reinterpret_cast<LPBYTE>(&docInfo)) == 0) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_START_DOC_ERROR,
            "Failed to start print job. Windows Error: " + std::to_string(winError)
        );
        ClosePrinter(hPrinter);
        return result;
    }

    // Start a page for printing
    if (StartPagePrinter(hPrinter) == 0) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_START_PAGE_ERROR,
            "Failed to start page. Windows Error: " + std::to_string(winError)
        );
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return result;
    }

    // ESC/POS command to open the cash drawer
    std::vector<unsigned char> escposCommand = { 0x1B, 0x70, 0x00, 0x32, 0xFA };
    DWORD bytesWritten = 0;

    // Send the command to the printer
    if (!WritePrinter(hPrinter, escposCommand.data(), static_cast<DWORD>(escposCommand.size()), &bytesWritten)) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to write to printer. Windows Error: " + std::to_string(winError)
        );
        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return result;
    }

    // Verify that the full command was sent
    if (bytesWritten != escposCommand.size()) {
        result.setError(
            DRAWER_INCOMPLETE_WRITE,
            "Not all bytes were written to printer. Bytes written: " + std::to_string(bytesWritten)
        );
        EndPagePrinter(hPrinter);
        EndDocPrinter(hPrinter);
        ClosePrinter(hPrinter);
        return result;
    }

    // End the page and the print job
    EndPagePrinter(hPrinter);
    EndDocPrinter(hPrinter);
    ClosePrinter(hPrinter);

    return result;
}

napi_value OpenCashDrawer(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value result_object;
    
    // Retrieve arguments
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    // Get the printer name string
    size_t str_size;
    size_t str_size_read;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &str_size));

    std::string printer_name(str_size, '\0');
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], &printer_name[0], str_size + 1, &str_size_read));

    // Call the function to open the cash drawer
    DrawerResult drawer_result = open_cash_drawer(printer_name);

    // Create result object
    NAPI_CALL(env, napi_create_object(env, &result_object));

    // Add success property
    napi_value success_value;
    NAPI_CALL(env, napi_get_boolean(env, drawer_result.success, &success_value));
    NAPI_CALL(env, napi_set_named_property(env, result_object, "success", success_value));

    // Add errorCode property
    napi_value error_code_value;
    NAPI_CALL(env, napi_create_int32(env, drawer_result.errorCode, &error_code_value));
    NAPI_CALL(env, napi_set_named_property(env, result_object, "errorCode", error_code_value));

    // Add errorMessage property
    napi_value error_message_value;
    NAPI_CALL(env, napi_create_string_utf8(env, drawer_result.errorMessage.c_str(), 
        NAPI_AUTO_LENGTH, &error_message_value));
    NAPI_CALL(env, napi_set_named_property(env, result_object, "errorMessage", error_message_value));

    return result_object;
}

napi_value init(napi_env env, napi_value exports) {
    napi_value open_cashdrawer;
    napi_create_function(env, nullptr, 0, OpenCashDrawer, nullptr, &open_cashdrawer);
    return open_cashdrawer;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)