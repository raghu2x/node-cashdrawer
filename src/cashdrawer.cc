#include "common.h"

// ============================================================================
// Cash Drawer Configuration
// ============================================================================

// Default ESC/POS drawer configuration
static const unsigned char DEFAULT_DRAWER_PIN = 0x00;      // Pin 0 (some drawers use 0x01)
static const unsigned char DEFAULT_PULSE_ON_TIME = 0x32;   // ~100ms
static const unsigned char DEFAULT_PULSE_OFF_TIME = 0xFA;  // ~500ms

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

// ============================================================================
// Windows RAII Printer Handle
// ============================================================================

#ifdef _WIN32
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

    PrinterHandle(const PrinterHandle&) = delete;
    PrinterHandle& operator=(const PrinterHandle&) = delete;
};
#endif

// ============================================================================
// Core cash drawer operation
// ============================================================================

static OperationResult open_cash_drawer(const std::string& printerName, const DrawerConfig& config = DrawerConfig()) {
    OperationResult result;

    // Validate printer name
    if (printerName.empty()) {
        result.setError(PRINTER_INVALID_ARGUMENT, "Printer name cannot be empty");
        return result;
    }
    if (printerName.length() > MAX_PRINTER_NAME_LENGTH) {
        result.setError(
            PRINTER_INVALID_ARGUMENT,
            "Printer name too long. Maximum length is " + std::to_string(MAX_PRINTER_NAME_LENGTH) + " characters"
        );
        return result;
    }

    // Block virtual printers
    if (isBlockedVirtualPrinter(printerName)) {
        result.setError(
            PRINTER_VIRTUAL_BLOCKED,
            "Cannot open cash drawer on virtual printer '" + printerName + "'. Please use a physical receipt printer."
        );
        return result;
    }

    std::vector<unsigned char> escposCommand = config.buildCommand();

#ifdef _WIN32
    PrinterHandle printer;
    DWORD winError = 0;

    if (!printer.open(printerName, &winError)) {
        result.setError(
            PRINTER_OPEN_ERROR,
            "Failed to open printer '" + printerName + "'. Windows Error: " + std::to_string(winError) +
            ". Make sure the printer is installed and accessible."
        );
        return result;
    }

    if (!printer.startDoc("Open Cash Drawer", &winError)) {
        result.setError(
            PRINTER_START_DOC_ERROR,
            "Failed to start print job. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    if (!printer.startPage(&winError)) {
        result.setError(
            PRINTER_START_PAGE_ERROR,
            "Failed to start page. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    DWORD bytesWritten = 0;
    if (!printer.write(escposCommand, &bytesWritten, &winError)) {
        result.setError(
            PRINTER_WRITE_ERROR,
            "Failed to write to printer. Windows Error: " + std::to_string(winError)
        );
        return result;
    }

    if (bytesWritten != escposCommand.size()) {
        result.setError(
            PRINTER_INCOMPLETE_WRITE,
            "Not all bytes were written to printer. Expected: " + std::to_string(escposCommand.size()) +
            ", Written: " + std::to_string(bytesWritten)
        );
        return result;
    }

#else
    // macOS and Linux use CUPS
    cups_dest_t *dests = nullptr;
    int num_dests = cupsGetDests(&dests);
    cups_dest_t *dest = cupsGetDest(printerName.c_str(), NULL, num_dests, dests);

    if (!dest) {
        result.setError(
            PRINTER_OPEN_ERROR,
            "Printer not found: '" + printerName + "'. Check printer name and installation."
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    // Create a temporary file for the command
    char tempFile[] = "/tmp/drawer_cmd_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd < 0) {
        int savedErrno = errno;
        result.setError(
            PRINTER_WRITE_ERROR,
            "Failed to create temporary file: " + std::string(std::strerror(savedErrno))
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    ssize_t bytes_written = write(fd, escposCommand.data(), escposCommand.size());
    int writeErrno = errno;
    close(fd);

    if (bytes_written < 0 || static_cast<size_t>(bytes_written) != escposCommand.size()) {
        result.setError(
            PRINTER_WRITE_ERROR,
            "Failed to write command to temporary file: " + std::string(std::strerror(writeErrno))
        );
        unlink(tempFile);
        cupsFreeDests(num_dests, dests);
        return result;
    }

    int job_id = cupsPrintFile(dest->name, tempFile, "Open Cash Drawer", 0, NULL);

    unlink(tempFile);

    if (job_id == 0) {
        result.setError(
            PRINTER_START_DOC_ERROR,
            "Failed to send print job to '" + printerName + "': " + cupsLastErrorString()
        );
        cupsFreeDests(num_dests, dests);
        return result;
    }

    cupsFreeDests(num_dests, dests);
#endif

    return result;
}

// ============================================================================
// Async work for openCashDrawer
// ============================================================================

struct AsyncDrawerWork {
    napi_async_work work;
    napi_deferred deferred;
    std::string printerName;
    DrawerConfig config;
    OperationResult result;
};

static void ExecuteOpenDrawer(napi_env env, void* data) {
    AsyncDrawerWork* asyncWork = static_cast<AsyncDrawerWork*>(data);
    asyncWork->result = open_cash_drawer(asyncWork->printerName, asyncWork->config);
}

static void CompleteOpenDrawer(napi_env env, napi_status status, void* data) {
    AsyncDrawerWork* asyncWork = static_cast<AsyncDrawerWork*>(data);

    napi_value result_object;
    napi_create_object(env, &result_object);

    napi_value success_value;
    napi_get_boolean(env, asyncWork->result.success, &success_value);
    napi_set_named_property(env, result_object, "success", success_value);

    napi_value error_code_value;
    napi_create_int32(env, asyncWork->result.errorCode, &error_code_value);
    napi_set_named_property(env, result_object, "errorCode", error_code_value);

    napi_value error_message_value;
    napi_create_string_utf8(env, asyncWork->result.errorMessage.c_str(),
        NAPI_AUTO_LENGTH, &error_message_value);
    napi_set_named_property(env, result_object, "errorMessage", error_message_value);

    napi_resolve_deferred(env, asyncWork->deferred, result_object);

    napi_delete_async_work(env, asyncWork->work);
    delete asyncWork;
}

// Helper to parse DrawerConfig from JS options
static bool ParseDrawerConfig(napi_env env, napi_value options, DrawerConfig& config) {
    if (options == nullptr) return true;

    napi_valuetype type;
    napi_typeof(env, options, &type);
    if (type != napi_object) return true;

    napi_value pin_value, pulse_on_value, pulse_off_value;
    bool has_pin, has_pulse_on, has_pulse_off;

    napi_has_named_property(env, options, "pin", &has_pin);
    if (has_pin) {
        napi_get_named_property(env, options, "pin", &pin_value);
        int32_t pin;
        if (napi_get_value_int32(env, pin_value, &pin) == napi_ok) {
            if (pin < 0 || pin > 255) return false;
            config.pin = static_cast<unsigned char>(pin);
        }
    }

    napi_has_named_property(env, options, "pulseOnTime", &has_pulse_on);
    if (has_pulse_on) {
        napi_get_named_property(env, options, "pulseOnTime", &pulse_on_value);
        int32_t pulseOn;
        if (napi_get_value_int32(env, pulse_on_value, &pulseOn) == napi_ok) {
            if (pulseOn < 0 || pulseOn > 255) return false;
            config.pulseOnTime = static_cast<unsigned char>(pulseOn);
        }
    }

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

// ============================================================================
// Exported N-API function
// ============================================================================

napi_value OpenCashDrawer(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];

    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 1) {
        napi_throw_error(env, nullptr, "Expected at least 1 argument: printer name");
        return nullptr;
    }

    std::string printer_name;
    if (!GetPrinterNameFromArg(env, args[0], printer_name)) {
        napi_throw_error(env, nullptr, "First argument must be a string (printer name) with max 256 characters");
        return nullptr;
    }

    DrawerConfig config;
    if (argc >= 2) {
        if (!ParseDrawerConfig(env, args[1], config)) {
            napi_throw_error(env, nullptr, "Invalid options: pin, pulseOnTime, pulseOffTime must be 0-255");
            return nullptr;
        }
    }

    AsyncDrawerWork* asyncWork = new AsyncDrawerWork();
    asyncWork->printerName = printer_name;
    asyncWork->config = config;

    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &asyncWork->deferred, &promise));

    napi_value work_name;
    NAPI_CALL(env, napi_create_string_utf8(env, "OpenCashDrawerAsync", NAPI_AUTO_LENGTH, &work_name));

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
