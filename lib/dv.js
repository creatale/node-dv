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

// Export Image.
exports.Image = binding.Image;

// Wrap and export Tesseract.
var Tesseract = exports.Tesseract = function(lang, image) {
    var tess;
    if (typeof lang !== "undefined" && lang !== null &&
        typeof image !== "undefined" && image !== null) {
        tess = new binding.Tesseract(__dirname + '/../', lang, image);
    } else if (typeof lang !== "undefined" && lang !== null) {
        tess = new binding.Tesseract(__dirname + '/../', lang);
    } else {
        tess = new binding.Tesseract(__dirname + '/../');
    }
    tess.__proto__ = Tesseract.prototype;
    return tess;
};
Tesseract.prototype = {
    __proto__: binding.Tesseract.prototype,
    constructor: Tesseract,
};
