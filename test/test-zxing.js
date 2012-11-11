var dv = require('../lib/dv');
var fs = require('fs');

var barcodeImage1 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode1.png'));
var barcodeImage2 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode2.png'));

var zxing = new dv.ZXing(barcodeImage1);
console.log('ZXing image: ' + zxing.image);
console.log(zxing.findCode());

zxing.image = barcodeImage2
console.log(zxing.findCode());


