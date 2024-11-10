# node-cashdrawer

A simple Node.js library to open a cash drawer connected through a specified printer. Ideal for POS systems that need to trigger cash drawers programmatically.

## Installation

Install via npm:

```bash
npm install node-cashdrawer
```

## Usage

This package lets you open a cash drawer by specifying the printer's name associated with the cash drawer. The drawer will trigger based on the standard escape command sent to the printer.

### Import the Module

```javascript
// ESM
import { openCashDrawer } from 'node-cashdrawer';

// Common JS
const { openCashDrawer } = require('node-cashdrawer');
```

### Open the Cash Drawer

To open the cash drawer, call the `openCashDrawer` function and pass the name of the printer connected to your cash drawer.

```javascript
openCashDrawer("your-printer-name");
```

### Example

```javascript
import { openCashDrawer } from 'node-cashdrawer';

const printerName = 'EPSON_TM_T20III';

const result = openCashDrawer(printerName)

if(!result.success) {
  console.log(result.errorMessage)
} else {
 console.log('Cash drawer opened successfully!')
}

```

## API

### `openCashDrawer(printerName: string): Promise<void>`

- **printerName** (string) - The name of the printer connected to the cash drawer. This should match the exact name your system uses for the printer.

- **Returns**: A promise that resolves when the cash drawer opens successfully. If the drawer cannot be opened, the promise will reject with an error.

## Error Handling

If the function fails to open the cash drawer (e.g., if the printer name is incorrect or the printer is not reachable), it will throw an error. Use `.catch` to handle errors gracefully.

## Supported Printers

This package has been tested with printers that support ESC/POS commands, such as:

- EPSON TM Series
- Star TSP Series

## Contributing

Contributions are welcome! If you encounter a bug or have a feature request, please open an issue on GitHub.

## License

This project is licensed under the MIT License.
