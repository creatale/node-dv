/*
 * node-dv - Document Vision for node.js
 *
 * Copyright (c) 2012 Christoph Schulz
 * Copyright (c) 2013-2015 creatale GmbH, contributors listed under AUTHORS
 * 
 * MIT License <https://github.com/creatale/node-dv/blob/master/LICENSE>
 */
var fs = require('fs');
var path = require('path');
var binding = require(__dirname + '/dvBinding.node');

// Wrap and export Tesseract.
var Tesseract = exports.Tesseract = function(lang, image, tessdata) {
    tessdata = tessdata || require('dv.data').tessdata;
    var tess;
    if (typeof lang !== 'undefined' && lang !== null
            && typeof image !== 'undefined' && image !== null) {
        tess = new binding.Tesseract(tessdata, lang, image);
    } else if (typeof lang !== 'undefined' && lang !== null) {
        tess = new binding.Tesseract(tessdata, lang);
    } else {
        tess = new binding.Tesseract(tessdata);
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
