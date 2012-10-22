var dv = require('../lib/dv');
var fs = require('fs');

var textPageImageData = fs.readFileSync(__dirname + '/fixtures/textpage300.png');
var textPageImage = new dv.Image("png", textPageImageData);

fs.writeFileSync(__dirname + '/out/textpage300.png', textPageImage.toBuffer("png"));
