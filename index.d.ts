export enum DrawerErrorCodes {
  DRAWER_SUCCESS = 0,
  DRAWER_INVALID_ARGUMENT = 1000,
  DRAWER_PRINTER_OPEN_ERROR = 1001,
  DRAWER_START_DOC_ERROR = 1002,
  DRAWER_START_PAGE_ERROR = 1003,
  DRAWER_WRITE_ERROR = 1004,
  DRAWER_INCOMPLETE_WRITE = 1005,
  INVALID_PRINTER_NAME = 1006,
  OTHER_ERROR = 1007,
  /** Attempted to use a virtual printer (PDF, XPS, Fax, etc.) */
  DRAWER_VIRTUAL_PRINTER_ERROR = 1008,
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
  errorCode: DrawerErrorCodes;
}

export enum PrinterStatus {
  OK = "OK",
  IDLE = "IDLE",
  OFFLINE = "OFFLINE",
  UNKNOWN = "UNKNOWN",
}

export interface PrinterInfo {
  name: string | null;
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
 * Gets a list of available printers on the system (Windows only).
 * @returns An array of printer information objects.
 */
export declare function getAvailablePrinters(): PrinterInfo[];
