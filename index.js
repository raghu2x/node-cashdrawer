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

// Printer Status Constants (uppercase for clarity)
const PrinterStatus = {
  OK: "OK",
  IDLE: "IDLE",
  OFFLINE: "OFFLINE",
  UNKNOWN: "UNKNOWN",
};

/**
 * Gets a list of available printers on the system.
 * Cross-platform: Works on Windows, macOS, and Linux.
 * @returns {Promise<Array<{name: string, default: boolean, status: string}>>}
 */
const getAvailablePrinters = async () => {
  try {
    const printers = await bindings.getAvailablePrinters();
    // Map status strings to PrinterStatus constants for consistency
    return printers.map((printer) => ({
      ...printer,
      status: PrinterStatus[printer.status] || PrinterStatus.UNKNOWN,
    }));
  } catch (error) {
    return [];
  }
};

module.exports = { openCashDrawer, getAvailablePrinters, PrinterStatus, PrinterErrorCodes };
   