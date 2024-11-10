enum DrawerErrorCodes {
  DRAWER_SUCCESS = 0,
  DRAWER_PRINTER_OPEN_ERROR = 1001,
  DRAWER_START_DOC_ERROR = 1002,
  DRAWER_START_PAGE_ERROR = 1003,
  DRAWER_WRITE_ERROR = 1004,
  DRAWER_INCOMPLETE_WRITE = 1005,
  INVALID_PRINTER_NAME = 1006,
  OTHER_ERROR = 1007,
}

interface OpenCashDrawerResult {
  success: boolean;
  errorMessage: string;
  errorCode: DrawerErrorCodes;
}

declare function openCashDrawer(printerName: string): OpenCashDrawerResult;

declare function getAvailablePrinters(): string[];

export { openCashDrawer, getAvailablePrinters };
