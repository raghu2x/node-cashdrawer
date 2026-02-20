#include "common.h"
#include <regex>

// ============================================================================
// Helper functions to parse connection details
// ============================================================================

// Extract IP address from a string (e.g., "192.168.1.100" or "192.168.1.100:9100")
static bool extractIPv4(const std::string& str, std::string& ip, int& port) {
    // Pattern for IPv4 with optional port: xxx.xxx.xxx.xxx[:port]
    std::regex ipPattern(R"((\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})(?::(\d+))?)");
    std::smatch match;

    if (std::regex_search(str, match, ipPattern)) {
        ip = match[1].str();
        if (match[2].matched) {
            port = std::stoi(match[2].str());
        }
        return true;
    }
    return false;
}

// Extract Bluetooth address (e.g., "00:11:22:33:44:55" or "001122334455")
static bool extractBluetoothAddress(const std::string& str, std::string& btAddr) {
    // Pattern for Bluetooth address with colons or dashes
    std::regex btPattern(R"(([0-9A-Fa-f]{2}[:\-]){5}[0-9A-Fa-f]{2})");
    std::smatch match;

    if (std::regex_search(str, match, btPattern)) {
        btAddr = match[0].str();
        return true;
    }

    // Pattern for Bluetooth address without separators (12 hex chars)
    std::regex btPatternNoSep(R"(([0-9A-Fa-f]{12}))");
    if (std::regex_search(str, match, btPatternNoSep)) {
        // Format it with colons
        std::string raw = match[1].str();
        btAddr = raw.substr(0, 2) + ":" + raw.substr(2, 2) + ":" +
                 raw.substr(4, 2) + ":" + raw.substr(6, 2) + ":" +
                 raw.substr(8, 2) + ":" + raw.substr(10, 2);
        return true;
    }
    return false;
}

#ifdef _WIN32
// Detect connection type and extract connection details (Windows)
static void detectConnectionDetails(const char* portName, DWORD attributes, PrinterInfo& info) {
    if (!portName || strlen(portName) == 0) {
        info.type = "UNKNOWN";
        return;
    }

    std::string portStr = portName;
    std::string portLower = toLowercase(portStr);

    // Check for USB ports
    if (portLower.find("usb") != std::string::npos) {
        info.type = "USB";
        return;
    }

    // Check for Bluetooth
    if (portLower.find("bth") != std::string::npos ||
        portLower.find("bluetooth") != std::string::npos) {
        info.type = "BLUETOOTH";
        extractBluetoothAddress(portStr, info.bluetoothAddress);
        return;
    }

    // Check for network printers - try to extract IP
    if (extractIPv4(portStr, info.ipAddress, info.port)) {
        info.type = "NETWORK";
        if (info.port == 0) {
            info.port = 9100; // Default RAW printing port
        }
        return;
    }

    // Check for UNC path (\\server\printer)
    if (portLower.find("\\\\") == 0 || portLower.find("//") == 0) {
        info.type = "NETWORK";
        // Extract server name/IP from UNC path
        size_t start = 2;
        size_t end = portStr.find('\\', start);
        if (end == std::string::npos) {
            end = portStr.find('/', start);
        }
        if (end != std::string::npos) {
            std::string server = portStr.substr(start, end - start);
            // Check if server is an IP
            int tempPort = 0;
            if (extractIPv4(server, info.ipAddress, tempPort)) {
                if (tempPort > 0) info.port = tempPort;
            }
        }
        return;
    }

    // Check for WSD ports
    if (portLower.find("wsd-") != std::string::npos ||
        portLower.find("ws-") != std::string::npos) {
        info.type = "NETWORK";
        return;
    }

    // Check for serial/COM ports
    if (portLower.find("com") == 0 && portLower.length() <= 5) {
        info.type = "SERIAL";
        return;
    }

    // Check for parallel/LPT ports
    if (portLower.find("lpt") == 0) {
        info.type = "PARALLEL";
        return;
    }

    // Check for file/virtual ports
    if (portLower.find("file:") != std::string::npos ||
        portLower.find("nul") != std::string::npos ||
        portLower.find("portprompt") != std::string::npos) {
        info.type = "VIRTUAL";
        return;
    }

    // Check attributes for network printer
    if (attributes & PRINTER_ATTRIBUTE_NETWORK) {
        info.type = "NETWORK";
        return;
    }

    // Check attributes for local printer
    if (attributes & PRINTER_ATTRIBUTE_LOCAL) {
        info.type = "LOCAL";
        return;
    }

    info.type = "UNKNOWN";
}
#endif

// ============================================================================
// Platform-specific printer enumeration
// ============================================================================

static std::vector<PrinterInfo> enumerate_printers() {
    std::vector<PrinterInfo> printers;

#ifdef _WIN32
    DWORD needed = 0;
    DWORD returned = 0;
    DWORD flags = PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS;

    // First call to get required buffer size
    EnumPrintersA(flags, NULL, 2, NULL, 0, &needed, &returned);

    if (needed == 0) {
        return printers;
    }

    // Allocate buffer and enumerate
    std::vector<BYTE> buffer(needed);
    if (!EnumPrintersA(flags, NULL, 2, buffer.data(), needed, &needed, &returned)) {
        return printers;
    }

    PRINTER_INFO_2A* pPrinterInfo = reinterpret_cast<PRINTER_INFO_2A*>(buffer.data());

    // Get default printer name
    char defaultPrinter[256] = {0};
    DWORD defaultSize = sizeof(defaultPrinter);
    GetDefaultPrinterA(defaultPrinter, &defaultSize);

    for (DWORD i = 0; i < returned; i++) {
        PrinterInfo info;
        info.name = pPrinterInfo[i].pPrinterName ? pPrinterInfo[i].pPrinterName : "";
        info.isDefault = (info.name == defaultPrinter);

        // Map printer status - check both Status and Attributes
        DWORD status = pPrinterInfo[i].Status;
        DWORD attributes = pPrinterInfo[i].Attributes;

        // Check if printer is set to work offline (in Attributes)
        bool isWorkOffline = (attributes & PRINTER_ATTRIBUTE_WORK_OFFLINE) != 0;

        if (isWorkOffline) {
            info.status = "OFFLINE";
        } else if (status & PRINTER_STATUS_OFFLINE) {
            info.status = "OFFLINE";
        } else if (status & PRINTER_STATUS_ERROR) {
            info.status = "ERROR";
        } else if (status & PRINTER_STATUS_PAPER_JAM) {
            info.status = "ERROR";
        } else if (status & PRINTER_STATUS_PAPER_OUT) {
            info.status = "ERROR";
        } else if (status & PRINTER_STATUS_NOT_AVAILABLE) {
            info.status = "OFFLINE";
        } else if (status & PRINTER_STATUS_PAUSED) {
            info.status = "PAUSED";
        } else if (status & PRINTER_STATUS_BUSY) {
            info.status = "BUSY";
        } else if (status & PRINTER_STATUS_PRINTING) {
            info.status = "PRINTING";
        } else if (status & PRINTER_STATUS_PROCESSING) {
            info.status = "PROCESSING";
        } else if (status == 0) {
            info.status = "IDLE";
        } else {
            info.status = "UNKNOWN";
        }

        // Detect connection type and extract connection details
        detectConnectionDetails(pPrinterInfo[i].pPortName, attributes, info);

        printers.push_back(info);
    }
#else
    // macOS/Linux: Use CUPS
    cups_dest_t* dests = nullptr;
    int num_dests = cupsGetDests(&dests);

    for (int i = 0; i < num_dests; i++) {
        PrinterInfo info;
        info.name = dests[i].name ? dests[i].name : "";
        info.isDefault = (dests[i].is_default != 0);

        // Get printer state from options
        const char* state = cupsGetOption("printer-state", dests[i].num_options, dests[i].options);
        if (state) {
            int stateVal = atoi(state);
            // IPP printer states: 3=idle, 4=processing, 5=stopped
            if (stateVal == 3) {
                info.status = "IDLE";
            } else if (stateVal == 4) {
                info.status = "PROCESSING";
            } else if (stateVal == 5) {
                info.status = "OFFLINE";
            } else {
                info.status = "UNKNOWN";
            }
        } else {
            info.status = "IDLE";
        }

        // Detect connection type and extract details from device-uri
        const char* deviceUri = cupsGetOption("device-uri", dests[i].num_options, dests[i].options);
        if (deviceUri) {
            std::string uri = deviceUri;
            std::string uriLower = toLowercase(uri);

            if (uriLower.find("usb://") == 0 || uriLower.find("usb:") == 0) {
                info.type = "USB";
            } else if (uriLower.find("socket://") == 0) {
                info.type = "NETWORK";
                // Extract IP and port from socket://ip:port
                extractIPv4(uri.substr(9), info.ipAddress, info.port);
                if (info.port == 0) info.port = 9100;
            } else if (uriLower.find("ipp://") == 0 || uriLower.find("ipps://") == 0) {
                info.type = "NETWORK";
                size_t start = uriLower.find("://") + 3;
                extractIPv4(uri.substr(start), info.ipAddress, info.port);
                if (info.port == 0) info.port = (uriLower.find("ipps://") == 0) ? 631 : 631;
            } else if (uriLower.find("http://") == 0 || uriLower.find("https://") == 0) {
                info.type = "NETWORK";
                size_t start = uriLower.find("://") + 3;
                extractIPv4(uri.substr(start), info.ipAddress, info.port);
                if (info.port == 0) info.port = (uriLower.find("https://") == 0) ? 443 : 80;
            } else if (uriLower.find("lpd://") == 0 || uriLower.find("smb://") == 0) {
                info.type = "NETWORK";
                size_t start = uriLower.find("://") + 3;
                extractIPv4(uri.substr(start), info.ipAddress, info.port);
            } else if (uriLower.find("bluetooth://") == 0 || uriLower.find("bth://") == 0) {
                info.type = "BLUETOOTH";
                extractBluetoothAddress(uri, info.bluetoothAddress);
            } else if (uriLower.find("serial://") == 0 || uriLower.find("/dev/tty") != std::string::npos) {
                info.type = "SERIAL";
            } else if (uriLower.find("parallel://") == 0 || uriLower.find("/dev/lp") != std::string::npos) {
                info.type = "PARALLEL";
            } else if (uriLower.find("file://") == 0 || uriLower.find("cups-pdf") != std::string::npos) {
                info.type = "VIRTUAL";
            } else {
                info.type = "UNKNOWN";
            }
        } else {
            info.type = "UNKNOWN";
        }

        printers.push_back(info);
    }

    cupsFreeDests(num_dests, dests);
#endif

    return printers;
}

// ============================================================================
// Async work for getAvailablePrinters
// ============================================================================

struct AsyncPrintersWork {
    napi_async_work work;
    napi_deferred deferred;
    std::vector<PrinterInfo> printers;
};

static void ExecuteGetPrinters(napi_env env, void* data) {
    AsyncPrintersWork* asyncWork = static_cast<AsyncPrintersWork*>(data);
    asyncWork->printers = enumerate_printers();
}

static void CompleteGetPrinters(napi_env env, napi_status status, void* data) {
    AsyncPrintersWork* asyncWork = static_cast<AsyncPrintersWork*>(data);

    napi_value result_array;
    napi_create_array_with_length(env, asyncWork->printers.size(), &result_array);

    for (size_t i = 0; i < asyncWork->printers.size(); i++) {
        const PrinterInfo& printer = asyncWork->printers[i];

        napi_value printer_obj;
        napi_create_object(env, &printer_obj);

        // name
        napi_value name_val;
        napi_create_string_utf8(env, printer.name.c_str(), NAPI_AUTO_LENGTH, &name_val);
        napi_set_named_property(env, printer_obj, "name", name_val);

        // default
        napi_value default_val;
        napi_get_boolean(env, printer.isDefault, &default_val);
        napi_set_named_property(env, printer_obj, "default", default_val);

        // status
        napi_value status_val;
        napi_create_string_utf8(env, printer.status.c_str(), NAPI_AUTO_LENGTH, &status_val);
        napi_set_named_property(env, printer_obj, "status", status_val);

        // type
        napi_value connection_val;
        napi_create_string_utf8(env, printer.type.c_str(), NAPI_AUTO_LENGTH, &connection_val);
        napi_set_named_property(env, printer_obj, "type", connection_val);

        // ipAddress (only if not empty)
        if (!printer.ipAddress.empty()) {
            napi_value ip_val;
            napi_create_string_utf8(env, printer.ipAddress.c_str(), NAPI_AUTO_LENGTH, &ip_val);
            napi_set_named_property(env, printer_obj, "ipAddress", ip_val);
        }

        // port (only if > 0)
        if (printer.port > 0) {
            napi_value port_val;
            napi_create_int32(env, printer.port, &port_val);
            napi_set_named_property(env, printer_obj, "port", port_val);
        }

        // bluetoothAddress (only if not empty)
        if (!printer.bluetoothAddress.empty()) {
            napi_value bt_val;
            napi_create_string_utf8(env, printer.bluetoothAddress.c_str(), NAPI_AUTO_LENGTH, &bt_val);
            napi_set_named_property(env, printer_obj, "bluetoothAddress", bt_val);
        }

        napi_set_element(env, result_array, static_cast<uint32_t>(i), printer_obj);
    }

    napi_resolve_deferred(env, asyncWork->deferred, result_array);

    napi_delete_async_work(env, asyncWork->work);
    delete asyncWork;
}

// ============================================================================
// Exported N-API function
// ============================================================================

napi_value GetAvailablePrinters(napi_env env, napi_callback_info info) {
    AsyncPrintersWork* asyncWork = new AsyncPrintersWork();

    napi_value promise;
    NAPI_CALL(env, napi_create_promise(env, &asyncWork->deferred, &promise));

    napi_value work_name;
    NAPI_CALL(env, napi_create_string_utf8(env, "GetAvailablePrintersAsync", NAPI_AUTO_LENGTH, &work_name));

    NAPI_CALL(env, napi_create_async_work(
        env,
        nullptr,
        work_name,
        ExecuteGetPrinters,
        CompleteGetPrinters,
        asyncWork,
        &asyncWork->work
    ));

    NAPI_CALL(env, napi_queue_async_work(env, asyncWork->work));

    return promise;
}
