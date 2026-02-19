const { openCashDrawer } = require('./index.js');

console.log('Testing cash drawer module...');

// Test with a dummy printer name
const result = openCashDrawer('test-printer');
console.log('Test result:', result);

if (result.success) {
  console.log('✅ Cash drawer opened successfully!');
} else {
  console.log('❌ Expected error for non-existent printer:', result.errorMessage);
}

console.log('Test completed.');