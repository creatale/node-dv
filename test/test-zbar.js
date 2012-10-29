var dv = require('../lib/dv');
var fs = require('fs');

var barcodeImage = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcodes.png'));

var zbar = new dv.ZBar(barcodeImage);
console.log('ZBar image: ' + zbar.image);
console.log(zbar.findSymbols());
