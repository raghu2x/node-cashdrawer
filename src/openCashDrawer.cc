#include <node_api.h>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#elif defined(__APPLE__)
#include <cups/cups.h>
#include <unistd.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#endif

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
    DRAWER_INCOMPLETE_WRITE = 1005,
    DRAWER_PLATFORM_ERROR = 1006
};

DrawerResult open_cash_drawer(const std::string& printerName) {
    DrawerResult result;

#ifdef _WIN32
    HANDLE hPrinter;
    
    // Convert string to wide string for Unicode support
    std::wstring widePrinterName(printerName.begin(), printerName.end());
    
    // Attempt to open the printer (try both ANSI and Unicode versions)
    if (!OpenPrinterA(const_cast<char*>(printerName.c_str()), &hPrinter, NULL)) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "Failed to open printer '" + printerName + "'. Windows Error: " + std::to_string(winError) +
            ". Make sure the printer is installed and accessible."
        );
        return result;
    }

    // Set up print job information
    DOC_INFO_1A docInfo;
    docInfo.pDocName = const_cast<LPSTR>("Open Cash Drawer");
    docInfo.pOutputFile = nullptr;
    docInfo.pDatatype = const_cast<LPSTR>("RAW");

    // Start a print job
    DWORD jobId = StartDocPrinterA(hPrinter, 1, reinterpret_cast<LPBYTE>(&docInfo));
    if (jobId == 0) {
        DWORD winError = GetLastError();
        result.setError(
            DRAWER_START_DOC_ERROR,
            "Failed to start print job. Windows Error: " + std::to_string(winError)
        );
        ClosePrinter(hPrinter);
        return result;
    }

    // Start a page for printing
    if (!StartPagePrinter(hPrinter)) {
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
            "Not all bytes were written to printer. Expected: " + std::to_string(escposCommand.size()) +
            ", Written: " + std::to_string(bytesWritten)
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

#elif defined(__APPLE__)
    // Simplified macOS implementation using CUPS
    cups_dest_t *dests;
    int num_dests = cupsGetDests(&dests);
    cups_dest_t *dest = cupsGetDest(printerName.c_str(), NULL, num_dests, dests);
    
    if (!dest) {
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "Printer not found: '" + printerName + "'. Check printer name and installation."
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // ESC/POS command to open the cash drawer
    std::vector<unsigned char> escposCommand = { 0x1B, 0x70, 0x00, 0x32, 0xFA };
    
    // Create a temporary file for the command
    char tempFile[] = "/tmp/drawer_cmd_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd < 0) {
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to create temporary file: " + std::string(strerror(errno))
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // Write command to temp file
    ssize_t bytes_written = write(fd, escposCommand.data(), escposCommand.size());
    close(fd);
    
    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != escposCommand.size()) {
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to write command to temporary file"
        );
        unlink(tempFile);
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // Use cupsPrintFile which is simpler and more reliable
    int job_id = cupsPrintFile(dest->name, tempFile, "Open Cash Drawer", 0, NULL);
    
    // Clean up temp file
    unlink(tempFile);
    
    if (job_id == 0) {
        result.setError(
            DRAWER_START_DOC_ERROR,
            "Failed to send print job to '" + printerName + "': " + cupsLastErrorString()
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    cupsFreeDests(num_dests, dests);

#else
    // Linux/Unix fallback - try to write directly to device or use lpr
    std::string devicePath = "/dev/usb/lp0"; // Common USB printer path
    
    // Try common device paths
    std::vector<std::string> possiblePaths = {
        "/dev/usb/lp0", "/dev/usb/lp1", "/dev/lp0", "/dev/lp1"
    };
    
    bool deviceFound = false;
    for (const auto& path : possiblePaths) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            devicePath = path;
            deviceFound = true;
            break;
        }
    }
    
    if (!deviceFound) {
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "No printer device found. Tried common paths like /dev/usb/lp0, /dev/lp0"
        );
        return result;
    }
    
    // Open device for writing
    int fd = open(devicePath.c_str(), O_WRONLY);
    if (fd < 0) {
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "Failed to open printer device '" + devicePath + "': " + std::string(strerror(errno))
        );
        return result;
    }
    
    // ESC/POS command to open the cash drawer
    std::vector<unsigned char> escposCommand = { 0x1B, 0x70, 0x00, 0x32, 0xFA };
    
    ssize_t bytes_written = write(fd, escposCommand.data(), escposCommand.size());
    close(fd);
    
    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != escposCommand.size()) {
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to write command to printer device"
        );
        return result;
    }
#endif

    return result;
}

napi_value OpenCashDrawer(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_value result_object;
    
    // Check argument count
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));
    
    if (argc < 1) {
        napi_throw_error(env, nullptr, "Expected 1 argument: printer name");
        return nullptr;
    }

    // Check if argument is a string
    napi_valuetype valuetype;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype));
    
    if (valuetype != napi_string) {
        napi_throw_error(env, nullptr, "Argument must be a string");
        return nullptr;
    }

    // Get the printer name string
    size_t str_size;
    size_t str_size_read;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &str_size));

    std::string printer_name(str_size, '\0');
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], &printer_name[0], str_size + 1, &str_size_read));

    // Remove null terminator if present
    if (!printer_name.empty() && printer_name.back() == '\0') {
        printer_name.pop_back();
    }

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
    NAPI_CALL(env, napi_create_function(env, nullptr, 0, OpenCashDrawer, nullptr, &open_cashdrawer));
    NAPI_CALL(env, napi_set_named_property(env, exports, "openCashDrawer", open_cashdrawer));
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)