const { openCashDrawer, getAvailablePrinters, PrinterErrorCodes } = require('./index.js');

// Use a non-existent printer for safe testing (won't create files)
const TEST_PRINTER_NAME = 'test-printer-does-not-exist';

async function runTests() {
  console.log('Testing cash drawer module...\n');

  // Show available printers (now async)
  const printers = await getAvailablePrinters();
  console.log('Available printers:', printers);
  console.log('');

  // Show exported error codes
  console.log('Exported PrinterErrorCodes:', PrinterErrorCodes);
  console.log('');

  // Test with non-existent printer (expected to fail gracefully)
  console.log(`Test 1: Opening drawer on "${TEST_PRINTER_NAME}"...`);
  const result = await openCashDrawer(TEST_PRINTER_NAME);
  console.log('Result:', result);
  console.log('');

  // Test with options
  console.log(`Test 2: Opening drawer with custom options...`);
  const resultWithOptions = await openCashDrawer(TEST_PRINTER_NAME, {
    pin: 1,
    pulseOnTime: 100,
    pulseOffTime: 200
  });
  console.log('Result:', resultWithOptions);
  console.log('');

  // Test validation - empty printer name
  console.log('Test 3: Empty printer name validation...');
  const emptyResult = await openCashDrawer('');
  console.log('Result:', emptyResult);
  console.log('');

  // Test virtual printer blocking
  console.log('Test 4: Virtual printer blocking (Microsoft Print to PDF)...');
  const virtualResult = await openCashDrawer('Microsoft Print to PDF');
  console.log('Result:', virtualResult);
  console.log('Expected: errorCode 1008 (virtual printer blocked)');
  console.log('');

  console.log('All tests completed.');
}

runTests().catch(console.error);