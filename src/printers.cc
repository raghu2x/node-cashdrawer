#include "common.h"

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

        // Map printer status
        DWORD status = pPrinterInfo[i].Status;
        if (status == 0) {
            info.status = "OK";
        } else if (status & PRINTER_STATUS_OFFLINE) {
            info.status = "OFFLINE";
        } else if (status & PRINTER_STATUS_PAUSED) {
            info.status = "IDLE";
        } else {
            info.status = "UNKNOWN";
        }

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
                info.status = "OK";
            } else if (stateVal == 5) {
                info.status = "OFFLINE";
            } else {
                info.status = "UNKNOWN";
            }
        } else {
            info.status = "OK";
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
