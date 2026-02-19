// Will load a compiled build if present or a prebuild.
// If no build if found it will throw an exception
const addon = require('node-gyp-build')(__dirname);

module.exports = {
  openCashDrawer: addon.openCashDrawer
};