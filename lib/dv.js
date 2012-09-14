var fs = require('fs');
var path = require('path');

var binding;
var debugBinding = path.normalize(__dirname + '/../build/Debug/dvBinding.node');
var releaseBinding = path.normalize(__dirname + '/../build/Release/dvBinding.node');
// Test for debug binding and prefer it.
if (fs.existsSync(debugBinding)) {
    binding = require(debugBinding);
} else {
    binding = require(releaseBinding);
}

// Exports.
exports.Image = binding.Image;
exports.Tesseract = binding.Tesseract;
