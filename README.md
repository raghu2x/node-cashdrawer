# @devraghu/cashdrawer

A simple Node.js library to open a cash drawer connected through a specified printer. Ideal for POS systems that need to trigger cash drawers programmatically.

## Installation

Install via npm:

```bash
npm install @devraghu/cashdrawer
```

## Platform Support

- Windows (x64, arm64)
- macOS (x64, arm64)
- Linux (x64, arm64)

## Usage

This package lets you open a cash drawer by specifying the printer's name associated with the cash drawer. The drawer will trigger based on the standard ESC/POS command sent to the printer.

### Import the Module

```javascript
// ESM
import { openCashDrawer, getAvailablePrinters, PrinterStatus, PrinterType } from '@devraghu/cashdrawer';

// CommonJS
const { openCashDrawer, getAvailablePrinters, PrinterStatus, PrinterType } = require('@devraghu/cashdrawer');
```

### Open the Cash Drawer

To open the cash drawer, call the `openCashDrawer` function and pass the name of the printer connected to your cash drawer. The function is asynchronous and returns a Promise.

```javascript
await openCashDrawer("your-printer-name");
```

### Example

```javascript
import { openCashDrawer } from '@devraghu/cashdrawer';

const printerName = 'EPSON_TM_T20III';

const result = await openCashDrawer(printerName);

if (!result.success) {
  console.log(result.errorMessage);
} else {
  console.log('Cash drawer opened successfully!');
}
```

### Using Options

You can customize the drawer command with optional parameters:

```javascript
import { openCashDrawer } from '@devraghu/cashdrawer';

const result = await openCashDrawer('EPSON_TM_T20III', {
  pin: 0,           // Drawer pin (0 or 1). Default: 0
  pulseOnTime: 50,  // Pulse on time (0-255). Default: 50 (~100ms)
  pulseOffTime: 250 // Pulse off time (0-255). Default: 250 (~500ms)
});
```

## API

### `openCashDrawer(printerName: string, options?: DrawerOptions): Promise<OpenCashDrawerResult>`

Opens the cash drawer connected to the specified printer.

**Parameters:**

- **printerName** (string) - The name of the printer connected to the cash drawer. This should match the exact name your system uses for the printer.
- **options** (DrawerOptions, optional) - Configuration for the drawer command:
  - `pin` (number) - Drawer pin (0 or 1). Default: 0
  - `pulseOnTime` (number) - Pulse on time (0-255). Default: 50 (~100ms)
  - `pulseOffTime` (number) - Pulse off time (0-255). Default: 250 (~500ms)

**Returns:** `Promise<OpenCashDrawerResult>` - A promise that resolves to an object with:
  - `success` (boolean): Indicates whether the cash drawer opened successfully.
  - `errorMessage` (string): A description of the error if the operation failed.
  - `errorCode` (PrinterErrorCodes): A specific error code representing the type of failure.

### `getAvailablePrinters(): Promise<PrinterInfo[]>`

Returns a list of printers available on the system. This is useful for identifying the exact name of the printer connected to your cash drawer.

```javascript
import { getAvailablePrinters, PrinterStatus, PrinterType } from '@devraghu/cashdrawer';

const printers = await getAvailablePrinters();
console.log('Available Printers:', printers);

// Each printer object contains:
// {
//   name: string,           // Printer name
//   default: boolean,       // Is this the default printer?
//   status: PrinterStatus,  // Current printer status
//   type: PrinterType,      // Connection type (USB, NETWORK, etc.)
//   ipAddress?: string,     // IP address (for network printers)
//   port?: number,          // Port number (for network printers)
//   bluetoothAddress?: string // Bluetooth MAC address (for Bluetooth printers)
// }

// Filter by connection type
const networkPrinters = printers.filter(p => p.type === PrinterType.NETWORK);
const usbPrinters = printers.filter(p => p.type === PrinterType.USB);

// Check printer status
const readyPrinters = printers.filter(p => p.status === PrinterStatus.IDLE);
```

### `PrinterStatus`

An enum representing printer status values:

```javascript
import { PrinterStatus } from '@devraghu/cashdrawer';

// Ready states
PrinterStatus.IDLE        // Printer is ready
PrinterStatus.PRINTING    // Currently printing
PrinterStatus.PROCESSING  // Processing a job
PrinterStatus.BUSY        // Printer is busy

// Offline/Error states
PrinterStatus.OFFLINE     // Printer is offline
PrinterStatus.ERROR       // General error
PrinterStatus.PAUSED      // Printer is paused

// Unknown
PrinterStatus.UNKNOWN     // Status cannot be determined
```

### `PrinterType`

An enum representing printer connection types:

```javascript
import { PrinterType } from '@devraghu/cashdrawer';

PrinterType.USB        // USB connected printer
PrinterType.NETWORK    // Network printer (TCP/IP, IPP, etc.)
PrinterType.BLUETOOTH  // Bluetooth printer
PrinterType.SERIAL     // Serial port (COM) printer
PrinterType.PARALLEL   // Parallel port (LPT) printer
PrinterType.VIRTUAL    // Virtual printer (PDF, XPS, etc.)
PrinterType.LOCAL      // Local printer (other)
PrinterType.UNKNOWN    // Connection type unknown
```

### `PrinterErrorCodes`

Error codes returned in `OpenCashDrawerResult.errorCode`:

```javascript
import { PrinterErrorCodes } from '@devraghu/cashdrawer';

PrinterErrorCodes.PRINTER_SUCCESS          // 0 - Success
PrinterErrorCodes.PRINTER_INVALID_ARGUMENT // 1000 - Invalid argument
PrinterErrorCodes.PRINTER_OPEN_ERROR       // 1001 - Failed to open printer
PrinterErrorCodes.PRINTER_START_DOC_ERROR  // 1002 - Failed to start document
PrinterErrorCodes.PRINTER_START_PAGE_ERROR // 1003 - Failed to start page
PrinterErrorCodes.PRINTER_WRITE_ERROR      // 1004 - Failed to write data
PrinterErrorCodes.PRINTER_INCOMPLETE_WRITE // 1005 - Incomplete write
PrinterErrorCodes.PRINTER_INVALID_NAME     // 1006 - Invalid printer name
PrinterErrorCodes.PRINTER_OTHER_ERROR      // 1007 - Other error
PrinterErrorCodes.PRINTER_VIRTUAL_BLOCKED  // 1008 - Virtual printer blocked
```

## Supported Printers

This package has been tested with printers that support ESC/POS commands, such as:

- EPSON TM Series
- Star TSP Series

## Contributing

Contributions are welcome! If you encounter a bug or have a feature request, please open an issue on GitHub.

## License

This project is licensed under the MIT License.
