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
import { openCashDrawer, getAvailablePrinters, PrinterStatus } from '@devraghu/cashdrawer';

// CommonJS
const { openCashDrawer, getAvailablePrinters, PrinterStatus } = require('@devraghu/cashdrawer');
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
  - `errorCode` (DrawerErrorCodes): A specific error code representing the type of failure.

### `getAvailablePrinters(): PrinterInfo[]`

Returns a list of printers available on the system. This is useful for identifying the exact name of the printer connected to your cash drawer.

```javascript
import { getAvailablePrinters, PrinterStatus } from '@devraghu/cashdrawer';

const printers = getAvailablePrinters();
console.log('Available Printers:', printers);

// Each printer object contains:
// {
//   name: string | null,
//   default: boolean,
//   status: PrinterStatus
// }
```

### `PrinterStatus`

An enum representing printer status values:

```javascript
import { PrinterStatus } from '@devraghu/cashdrawer';

PrinterStatus.OK       // "OK"
PrinterStatus.IDLE     // "IDLE"
PrinterStatus.OFFLINE  // "OFFLINE"
PrinterStatus.UNKNOWN  // "UNKNOWN"
```

## Supported Printers

This package has been tested with printers that support ESC/POS commands, such as:

- EPSON TM Series
- Star TSP Series

## Contributing

Contributions are welcome! If you encounter a bug or have a feature request, please open an issue on GitHub.

## License

This project is licensed under the MIT License.
