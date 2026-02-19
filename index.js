const bindings = require("./binding.js");
const { execSync } = require("child_process");

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
      errorCode: 1006,
      errorMessage: "printerName must be a string.",
    };
  }

  try {
    const result = await bindings.openCashDrawer(printerName, options);
    return result;
  } catch (error) {
    console.log(error);
    return {
      success: false,
      errorCode: 1007,
      errorMessage: error?.message ?? "Failed to open Cash Drawer.",
    };
  }
};

// Printer Status Constants (uppercase for clarity)
const PrinterStatus = {
  OK: "OK",
  IDLE: "IDLE",
  OFFLINE: "OFFLINE",
  UNKNOWN: "UNKNOWN", // For unmapped or missing statuses
};

const getAvailablePrinters = () => {
  try {
    const rawOutput = execSync(
      "wmic printer get name, default, status"
    ).toString();

    const lines = rawOutput
      .split("\n")
      .map((line) => line.trim())
      .filter((line) => line); // Remove empty lines

    // Extract headers and data
    const [headersLine, ...dataLines] = lines;
    const headers = headersLine.split(/\s{2,}/).map((header) => header.trim());

    const printers = dataLines.map((line) => {
      const values = line.split(/\s{2,}/).map((value) => value.trim());
      return headers.reduce((printer, header, index) => {
        const key = header.toLowerCase();
        let value = values[index] || null;

        if (key === "default") {
          value = value === "TRUE"; // Convert 'TRUE'/'FALSE' to boolean
        } else if (key === "status") {
          value = PrinterStatus[value?.toUpperCase()] || PrinterStatus.UNKNOWN; // Ensure case-insensitivity and map to constants
        }

        printer[key] = value;
        return printer;
      }, {});
    });

    return printers;
  } catch (error) {
    console.error("Error fetching printer list:", error.message);
    return [];
  }
};

module.exports = { openCashDrawer, getAvailablePrinters, PrinterStatus };
   