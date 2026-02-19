#include "common.h"

// ============================================================================
// Export error codes as a JavaScript object
// ============================================================================

napi_value GetErrorCodes(napi_env env) {
    napi_value codes;
    napi_create_object(env, &codes);

    napi_value val;

    napi_create_int32(env, PRINTER_SUCCESS, &val);
    napi_set_named_property(env, codes, "PRINTER_SUCCESS", val);

    napi_create_int32(env, PRINTER_INVALID_ARGUMENT, &val);
    napi_set_named_property(env, codes, "PRINTER_INVALID_ARGUMENT", val);

    napi_create_int32(env, PRINTER_OPEN_ERROR, &val);
    napi_set_named_property(env, codes, "PRINTER_OPEN_ERROR", val);

    napi_create_int32(env, PRINTER_START_DOC_ERROR, &val);
    napi_set_named_property(env, codes, "PRINTER_START_DOC_ERROR", val);

    napi_create_int32(env, PRINTER_START_PAGE_ERROR, &val);
    napi_set_named_property(env, codes, "PRINTER_START_PAGE_ERROR", val);

    napi_create_int32(env, PRINTER_WRITE_ERROR, &val);
    napi_set_named_property(env, codes, "PRINTER_WRITE_ERROR", val);

    napi_create_int32(env, PRINTER_INCOMPLETE_WRITE, &val);
    napi_set_named_property(env, codes, "PRINTER_INCOMPLETE_WRITE", val);

    napi_create_int32(env, PRINTER_INVALID_NAME, &val);
    napi_set_named_property(env, codes, "PRINTER_INVALID_NAME", val);

    napi_create_int32(env, PRINTER_OTHER_ERROR, &val);
    napi_set_named_property(env, codes, "PRINTER_OTHER_ERROR", val);

    napi_create_int32(env, PRINTER_VIRTUAL_BLOCKED, &val);
    napi_set_named_property(env, codes, "PRINTER_VIRTUAL_BLOCKED", val);

    return codes;
}

// ============================================================================
// Module initialization
// ============================================================================

napi_value init(napi_env env, napi_value exports) {
    // Export openCashDrawer
    napi_value open_cashdrawer;
    NAPI_CALL(env, napi_create_function(env, nullptr, 0, OpenCashDrawer, nullptr, &open_cashdrawer));
    NAPI_CALL(env, napi_set_named_property(env, exports, "openCashDrawer", open_cashdrawer));

    // Export getAvailablePrinters
    napi_value get_printers;
    NAPI_CALL(env, napi_create_function(env, nullptr, 0, GetAvailablePrinters, nullptr, &get_printers));
    NAPI_CALL(env, napi_set_named_property(env, exports, "getAvailablePrinters", get_printers));

    // Export error codes
    napi_value error_codes = GetErrorCodes(env);
    NAPI_CALL(env, napi_set_named_property(env, exports, "PrinterErrorCodes", error_codes));

    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init)
