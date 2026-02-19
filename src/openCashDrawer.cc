#include <node_api.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cerrno>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
#include <winspool.h>
#else
// Both macOS and Linux use CUPS
#include <cups/cups.h>
#include <unistd.h>
#include <fcntl.h>
#endif

// Constants
static const size_t MAX_PRINTER_NAME_LENGTH = 256;

// Default ESC/POS drawer configuration
static const unsigned char DEFAULT_DRAWER_PIN = 0x00;      // Pin 0 (some drawers use 0x01)
static const unsigned char DEFAULT_PULSE_ON_TIME = 0x32;   // ~100ms
static const unsigned char DEFAULT_PULSE_OFF_TIME = 0xFA;  // ~500ms

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

// Helper function for case-insensitive string comparison
static std::string toLowercase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return result;
}

// Check if printer is a blocked virtual printer
static bool isBlockedVirtualPrinter(const std::string& printerName) {
    std::string lowerName = toLowercase(printerName);
    for (const auto& blocked : BLOCKED_VIRTUAL_PRINTERS) {
        if (lowerName.find(blocked) != std::string::npos) {
            return true;
        }
    }
    return false;
}

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

// Structure to hold drawer configuration
struct DrawerConfig {
    unsigned char pin;
    unsigned char pulseOnTime;
    unsigned char pulseOffTime;

    DrawerConfig()
        : pin(DEFAULT_DRAWER_PIN)
        , pulseOnTime(DEFAULT_PULSE_ON_TIME)
        , pulseOffTime(DEFAULT_PULSE_OFF_TIME) {}

    DrawerConfig(unsigned char p, unsigned char onTime, unsigned char offTime)
        : pin(p), pulseOnTime(onTime), pulseOffTime(offTime) {}

    // Build the ESC/POS command for opening the drawer
    std::vector<unsigned char> buildCommand() const {
        return { 0x1B, 0x70, pin, pulseOnTime, pulseOffTime };
    }
};

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
    DRAWER_INVALID_ARGUMENT = 1000,
    DRAWER_PRINTER_OPEN_ERROR = 1001,
    DRAWER_START_DOC_ERROR = 1002,
    DRAWER_START_PAGE_ERROR = 1003,
    DRAWER_WRITE_ERROR = 1004,
    DRAWER_INCOMPLETE_WRITE = 1005,
    DRAWER_PLATFORM_ERROR = 1006,
    DRAWER_VIRTUAL_PRINTER_ERROR = 1008
};

#ifdef _WIN32
// RAII wrapper for Windows printer handle
class PrinterHandle {
public:
    PrinterHandle() : handle_(NULL), docStarted_(false), pageStarted_(false) {}

    ~PrinterHandle() {
        close();
    }

    bool open(const std::string& printerName, DWORD* errorCode) {
        if (!OpenPrinterA(const_cast<char*>(printerName.c_str()), &handle_, NULL)) {
            if (errorCode) *errorCode = GetLastError();
            handle_ = NULL;
            return false;
        }
        return true;
    }

    bool startDoc(const char* docName, DWORD* errorCode) {
        DOC_INFO_1A docInfo;
        docInfo.pDocName = const_cast<LPSTR>(docName);
        docInfo.pOutputFile = nullptr;
        docInfo.pDatatype = const_cast<LPSTR>("RAW");

        DWORD jobId = StartDocPrinterA(handle_, 1, reinterpret_cast<LPBYTE>(&docInfo));
        if (jobId == 0) {
            if (errorCode) *errorCode = GetLastError();
            return false;
        }
        docStarted_ = true;
        return true;
    }

    bool startPage(DWORD* errorCode) {
        if (!StartPagePrinter(handle_)) {
            if (errorCode) *errorCode = GetLastError();
            return false;
        }
        pageStarted_ = true;
        return true;
    }

    bool write(const std::vector<unsigned char>& data, DWORD* bytesWritten, DWORD* errorCode) {
        if (!WritePrinter(handle_, const_cast<unsigned char*>(data.data()),
                          static_cast<DWORD>(data.size()), bytesWritten)) {
            if (errorCode) *errorCode = GetLastError();
            return false;
        }
        return true;
    }

    void close() {
        if (pageStarted_) {
            EndPagePrinter(handle_);
            pageStarted_ = false;
        }
        if (docStarted_) {
            EndDocPrinter(handle_);
            docStarted_ = false;
        }
        if (handle_ != NULL) {
            ClosePrinter(handle_);
            handle_ = NULL;
        }
    }

    bool isValid() const { return handle_ != NULL; }

private:
    HANDLE handle_;
    bool docStarted_;
    bool pageStarted_;

    // Prevent copying
    PrinterHandle(const PrinterHandle&) = delete;
    PrinterHandle& operator=(const PrinterHandle&) = delete;
};
#endif

DrawerResult open_cash_drawer(const std::string& printerName, const DrawerConfig& config = DrawerConfig()) {
    DrawerResult result;

    // Validate printer name length
    if (printerName.empty()) {
        result.setError(DRAWER_INVALID_ARGUMENT, "Printer name cannot be empty");
        return result;
    }
    if (printerName.length() > MAX_PRINTER_NAME_LENGTH) {
        result.setError(
            DRAWER_INVALID_ARGUMENT,
            "Printer name too long. Maximum length is " + std::to_string(MAX_PRINTER_NAME_LENGTH) + " characters"
        );
        return result;
    }

    // Block virtual printers (PDF writers, fax, etc.)
    if (isBlockedVirtualPrinter(printerName)) {
        result.setError(
            DRAWER_VIRTUAL_PRINTER_ERROR,
            "Cannot open cash drawer on virtual printer '" + printerName + "'. Please use a physical receipt printer."
        );
        return result;
    }

    // Build the ESC/POS command using configuration
    std::vector<unsigned char> escposCommand = config.buildCommand();

#ifdef _WIN32
    PrinterHandle printer;
    DWORD winError = 0;

    // Open the printer using RAII wrapper
    if (!printer.open(printerName, &winError)) {
        result.setError(
            DRAWER_PRINTER_OPEN_ERROR,
            "Failed to open printer '" + printerName + "'. Windows Error: " + std::to_string(winError) +
            ". Make sure the printer is installed and accessible."
        );
        return result;
    }

    // Start a print job
    if (!printer.startDoc("Open Cash Drawer", &winError)) {
        result.setError(
            DRAWER_START_DOC_ERROR,
            "Failed to start print job. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    // Start a page for printing
    if (!printer.startPage(&winError)) {
        result.setError(
            DRAWER_START_PAGE_ERROR,
            "Failed to start page. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    // Send the command to the printer
    DWORD bytesWritten = 0;
    if (!printer.write(escposCommand, &bytesWritten, &winError)) {
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to write to printer. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    // Verify that the full command was sent
    if (bytesWritten != escposCommand.size()) {
        result.setError(
            DRAWER_INCOMPLETE_WRITE,
            "Not all bytes were written to printer. Expected: " + std::to_string(escposCommand.size()) +
            ", Written: " + std::to_string(bytesWritten)
        );
        return result;
    }

    // RAII handles cleanup automatically

#else
    // macOS and Linux both use CUPS
    cups_dest_t *dests = nullptr;
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

    // Create a temporary file for the command
    char tempFile[] = "/tmp/drawer_cmd_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd < 0) {
        int savedErrno = errno;  // Capture errno immediately
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to create temporary file: " + std::string(std::strerror(savedErrno))
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // Write command to temp file
    ssize_t bytes_written = write(fd, escposCommand.data(), escposCommand.size());
    int writeErrno = errno;  // Capture errno immediately after write
    close(fd);

    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != escposCommand.size()) {
        result.setError(
            DRAWER_WRITE_ERROR,
            "Failed to write command to temporary file: " + std::string(std::strerror(writeErrno))
        );
        unlink(tempFile);
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // Use cupsPrintFile to send the command
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
#endif

    return result;
}

// Async work data structure
struct AsyncDrawerWork {
    napi_async_work work;
    napi_deferred deferred;
    std::string printerName;
    DrawerConfig config;
    DrawerResult result;
};

// Execute function runs on worker thread
void ExecuteOpenDrawer(napi_env env, void* data) {
    AsyncDrawerWork* asyncWork = static_cast<AsyncDrawerWork*>(data);
    asyncWork->result = open_cash_drawer(asyncWork->printerName, asyncWork->config);
}

// Complete function runs on main thread
void CompleteOpenDrawer(napi_env env, napi_status status, void* data) {
    AsyncDrawerWork* asyncWork = static_cast<AsyncDrawerWork*>(data);

    napi_value result_object;
    napi_create_object(env, &result_object);

    // Add success property
    napi_value success_value;
    napi_get_boolean(env, asyncWork->result.success, &success_value);
    napi_set_named_property(env, result_object, "success", success_value);

    // Add errorCode property
    napi_value error_code_value;
    napi_create_int32(env, asyncWork->result.errorCode, &error_code_value);
    napi_set_named_property(env, result_object, "errorCode", error_code_value);

    // Add errorMessage property
    napi_value error_message_value;
    napi_create_string_utf8(env, asyncWork->result.errorMessage.c_str(),
        NAPI_AUTO_LENGTH, &error_message_value);
    napi_set_named_property(env, result_object, "errorMessage", error_message_value);

    // Resolve the promise
    napi_resolve_deferred(env, asyncWork->deferred, result_object);

    // Cleanup
    napi_delete_async_work(env, asyncWork->work);
    delete asyncWork;
}

// Helper function to parse DrawerConfig from JS options object
bool ParseDrawerConfig(napi_env env, napi_value options, DrawerConfig& config) {
    if (options == nullptr) return true;

    napi_valuetype type;
    napi_typeof(env, options, &type);
    if (type != napi_object) return true;

    napi_value pin_value, pulse_on_value, pulse_off_value;
    bool has_pin, has_pulse_on, has_pulse_off;

    // Check for 'pin' property
    napi_has_named_property(env, options, "pin", &has_pin);
    if (has_pin) {
        napi_get_named_property(env, options, "pin", &pin_value);
        int32_t pin;
        if (napi_get_value_int32(env, pin_value, &pin) == napi_ok) {
            if (pin < 0 || pin > 255) return false;
            config.pin = static_cast<unsigned char>(pin);
        }
    }

    // Check for 'pulseOnTime' property
    napi_has_named_property(env, options, "pulseOnTime", &has_pulse_on);
    if (has_pulse_on) {
        napi_get_named_property(env, options, "pulseOnTime", &pulse_on_value);
        int32_t pulseOn;
        if (napi_get_value_int32(env, pulse_on_value, &pulseOn) == napi_ok) {
            if (pulseOn < 0 || pulseOn > 255) return false;
            config.pulseOnTime = static_cast<unsigned char>(pulseOn);
        }
    }

    // Check for 'pulseOffTime' property
    napi_has_named_property(env, options, "pulseOffTime", &has_pulse_off);
    if (has_pulse_off) {
        napi_get_named_property(env, options, "pulseOffTime", &pulse_off_value);
        int32_t pulseOff;
        if (napi_get_value_int32(env, pulse_off_value, &pulseOff) == napi_ok) {
            if (pulseOff < 0 || pulseOff > 255) return false;
            config.pulseOffTime = static_cast<unsigned char>(pulseOff);
        }
    }

    return true;
}

// Helper function to extract printer name from JS argument
bool GetPrinterName(napi_env env, napi_value arg, std::string& printer_name) {
    napi_valuetype valuetype;
    napi_typeof(env, arg, &valuetype);

    if (valuetype != napi_string) {
        return false;
    }

    size_t str_size;
    napi_get_value_string_utf8(env, arg, nullptr, 0, &str_size);

    // Validate length before allocation
    if (str_size > MAX_PRINTER_NAME_LENGTH) {
        return false;
    }

    printer_name.resize(str_size);
    size_t str_size_read;
    napi_get_value_string_utf8(env, arg, &printer_name[0], str_size + 1, &str_size_read);

    // Remove null terminator if present
    if (!printer_name.empty() && printer_name.back() == '\0') {
        printer_name.pop_back();
    }

    return true;
}

// Asynchronous version: openCashDrawer(printerName, options?) -> Promise
napi_value OpenCashDrawer(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];

    // Get arguments
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 1) {
        napi_throw_error(env, nullptr, "Expected at least 1 argument: printer name");
        return nullptr;
    }

    // Get printer name
    std::string printer_name;
    if (!GetPrinterName(env, args[0], printer_name)) {
        napi_throw_error(env, nullptr, "First argument must be a string (printer name) with max 256 characters");
        return nullptr;
    }

    // Parse optional config
    DrawerConfig config;
    if (argc >= 2) {
        if (!ParseDrawerConfig(env, args[1], config)) {
            napi_throw_error(env, nullptr, "Invalid options: pin, pulseOnTime, pulseOffTime must be 0-255");
            return nullptr;
        }
    }

    // Create async work data
    AsyncDrawerWork* asyncWork = new AsyncDrawerWork();
    asyncWork->printerName = printer_name;
    asyncWork->config = config;

    // Create promise
    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &asyncWork->deferred, &promise));

    // Create async work name
    napi_value work_name;
    NAPI_CALL(env, napi_create_string_utf8(env, "OpenCashDrawerAsync", NAPI_AUTO_LENGTH, &work_name));

    // Create and queue async work
    NAPI_CALL(env, napi_create_async_work(
        env,
        nullptr,
        work_name,
        ExecuteOpenDrawer,
        CompleteOpenDrawer,
        asyncWork,
        &asyncWork->work
    ));

    NAPI_CALL(env, napi_queue_async_work(env, asyncWork->work));

    return promise;
}

napi_value init(napi_env env, napi_value exports) {
    napi_value open_cashdrawer;
    NAPI_CALL(env, napi_create_function(env, nullptr, 0, OpenCashDrawer, nullptr, &open_cashdrawer));
    NAPI_CALL(env, napi_set_named_property(env, exports, "openCashDrawer", open_cashdrawer));
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)