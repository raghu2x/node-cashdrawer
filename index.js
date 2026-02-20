const bindings = require("./binding.js");

// Import error codes from native layer (single source of truth)
const { PrinterErrorCodes } = bindings;

/**
 * Opens the cash drawer connected to the specified printer.
 * @param {string} printerName - The name of the printer connected to the cash drawer.
 * @param {Object} [options] - Optional configuration for the drawer command.
 * @param {number} [options.pin=0] - Drawer pin (0 or 1).
 * @param {number} [options.pulseOnTime=50] - Pulse on time (0-255).
 * @param {number} [options.pulseOffTime=250] - Pulse off time (0-255).
 * @returns {Promise<{success: boolean, errorCode: number, errorMessage: string}>}
 */
const openCashDrawer = async (printerName, options = {}) => {
  if (typeof printerName !== "string") {
    return {
      success: false,
      errorCode: PrinterErrorCodes.PRINTER_INVALID_NAME,
      errorMessage: "printerName must be a string.",
    };
  }

  try {
    const result = await bindings.openCashDrawer(printerName, options);
    return result;
  } catch (error) {
    return {
      success: false,
      errorCode: PrinterErrorCodes.PRINTER_OTHER_ERROR,
      errorMessage: error?.message ?? "Failed to open Cash Drawer.",
    };
  }
};

// Printer Status Constants
const PrinterStatus = {
  IDLE: "IDLE",
  OFFLINE: "OFFLINE",
  ERROR: "ERROR",
  PAUSED: "PAUSED",
  BUSY: "BUSY",
  PRINTING: "PRINTING",
  PROCESSING: "PROCESSING",
  UNKNOWN: "UNKNOWN",
};

// Printer Type Constants
const PrinterType = {
  USB: "USB",
  NETWORK: "NETWORK",
  BLUETOOTH: "BLUETOOTH",
  SERIAL: "SERIAL",
  PARALLEL: "PARALLEL",
  VIRTUAL: "VIRTUAL",
  LOCAL: "LOCAL",
  UNKNOWN: "UNKNOWN",
};

/**
 * Gets a list of available printers on the system.
 * Cross-platform: Works on Windows, macOS, and Linux.
 * @returns {Promise<Array<{name: string, default: boolean, status: string, type: string, ipAddress?: string, port?: number, bluetoothAddress?: string}>>}
 */
const getAvailablePrinters = async () => {
  try {
    const printers = await bindings.getAvailablePrinters();
    return printers
  } catch (error) {
    return [];
  }
};

module.exports = { openCashDrawer, getAvailablePrinters, PrinterStatus, PrinterType, PrinterErrorCodes };
   