/**
 * Error codes for printer operations.
 * These values are defined in the native C++ layer as the single source of truth.
 */
export enum PrinterErrorCodes {
  PRINTER_SUCCESS = 0,
  PRINTER_INVALID_ARGUMENT = 1000,
  PRINTER_OPEN_ERROR = 1001,
  PRINTER_START_DOC_ERROR = 1002,
  PRINTER_START_PAGE_ERROR = 1003,
  PRINTER_WRITE_ERROR = 1004,
  PRINTER_INCOMPLETE_WRITE = 1005,
  /** JS validation: printerName must be a string */
  PRINTER_INVALID_NAME = 1006,
  /** JS catch-all error */
  PRINTER_OTHER_ERROR = 1007,
  /** Attempted to use a virtual printer (PDF, XPS, Fax, etc.) */
  PRINTER_VIRTUAL_BLOCKED = 1008,
}

export interface DrawerOptions {
  /** Drawer pin (0 or 1). Default: 0 */
  pin?: number;
  /** Pulse on time (0-255). Default: 50 (~100ms) */
  pulseOnTime?: number;
  /** Pulse off time (0-255). Default: 250 (~500ms) */
  pulseOffTime?: number;
}

export interface OpenCashDrawerResult {
  success: boolean;
  errorMessage: string;
  errorCode: PrinterErrorCodes;
}

export enum PrinterStatus {
  OK = "OK",
  IDLE = "IDLE",
  OFFLINE = "OFFLINE",
  UNKNOWN = "UNKNOWN",
}

export interface PrinterInfo {
  name: string;
  default: boolean;
  status: PrinterStatus;
}

/**
 * Opens the cash drawer connected to the specified printer.
 * @param printerName - The name of the printer connected to the cash drawer.
 * @param options - Optional configuration for the drawer command.
 * @returns A promise that resolves to the result of the operation.
 */
export declare function openCashDrawer(
  printerName: string,
  options?: DrawerOptions
): Promise<OpenCashDrawerResult>;

/**
 * Gets a list of available printers on the system.
 * Cross-platform: Works on Windows, macOS, and Linux.
 * @returns A promise that resolves to an array of printer information objects.
 */
export declare function getAvailablePrinters(): Promise<PrinterInfo[]>;
