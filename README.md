# @devraghu/cashdrawer

A simple Node.js library to open a cash drawer connected through a specified printer. Ideal for POS systems that need to trigger cash drawers programmatically.

## Installation

Install via npm:

```bash
npm install @devraghu/cashdrawer
```

## Usage

This package lets you open a cash drawer by specifying the printer's name associated with the cash drawer. The drawer will trigger based on the standard escape command sent to the printer.

### Import the Module

```javascript
// ESM
import { openCashDrawer } from '@devraghu/cashdrawer';

// Common JS
const { openCashDrawer } = require('@devraghu/cashdrawer');
```

### Open the Cash Drawer

To open the cash drawer, call the `openCashDrawer` function and pass the name of the printer connected to your cash drawer.

```javascript
openCashDrawer("your-printer-name");
```

### Example

```javascript
import { openCashDrawer } from '@devraghu/cashdrawer';

const printerName = 'EPSON_TM_T20III';

const result = openCashDrawer(printerName)

if(!result.success) {
  console.log(result.errorMessage)
} else {
 console.log('Cash drawer opened successfully!')
}

```

## API

### `openCashDrawer(printerName: string): OpenCashDrawerResult`

- **printerName** (string) - The name of the printer connected to the cash drawer. This should match the exact name your system uses for the printer.

- **Returns**: an object with the following properties:
  - success (boolean): Indicates whether the cash drawer opened successfully.
  - errorMessage (string): A description of the error if the operation failed.
  - errorCode (DrawerErrorCodes): A specific error code representing the type of failure.


### Get Available Printers
The `getAvailablePrinters` function returns a list of printers available on the system. This is useful for identifying the exact name of the printer connected to your cash drawer.

```javascript
import { getAvailablePrinters } from '@devraghu/cashdrawer';

const printers = getAvailablePrinters();
console.log('Available Printers:', printers);
```
## Supported Printers

This package has been tested with printers that support ESC/POS commands, such as:

- EPSON TM Series
- Star TSP Series

## Contributing

Contributions are welcome! If you encounter a bug or have a feature request, please open an issue on GitHub.

## License

This project is licensed under the MIT License.
