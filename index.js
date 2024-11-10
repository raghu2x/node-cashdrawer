const bindings = require("./binding.js");
const { execSync } = require("child_process");

const openCashDrawer = (printerName) => {
  if (typeof printerName !== "string") {
    return {
      success: false,
      errorCode: 1006,
      errorMessage: "printerName must be a string.",
    };
  }

  try {
    const result = bindings(printerName);

    return result;
  } catch (error) {
    return {
      success: false,
      errorCode: 1007,
      errorMessage: "Failed to open Cash Drawer.",
    };
  }
};

const getAvailablePrinters = () => {
  const printers = execSync("wmic printer get name")
    .toString()
    .split("\n")
    .map((line) => line.trim())
    .filter((line) => line && line !== "Name");

  return printers;
};

module.exports = { openCashDrawer, getAvailablePrinters };
