/*
 * Copyright (c) 2012 Christoph Schulz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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

// Wrap and export Tesseract.
var Tesseract = exports.Tesseract = function(lang, image) {
    var tess;
    if (typeof lang !== "undefined" && lang !== null
            && typeof image !== "undefined" && image !== null) {
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

// Export others.
exports.Image = binding.Image;
exports.ZXing = binding.ZXing;
exports.TickReader = binding.TickReader;
