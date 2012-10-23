var dv = require('../lib/dv');
var fs = require('fs');

console.log("Load/Save");
var textPage = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
var dave = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/dave.png'));
var rgba = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/rgba.png'));
fs.writeFileSync(__dirname + '/out/textpage300.png', textPage.toBuffer("png"));
fs.writeFileSync(__dirname + '/out/dave.png', dave.toBuffer("png"));
fs.writeFileSync(__dirname + '/out/rgba.png', rgba.toBuffer("png"));

console.log("ToGray");
var grayDave = dave.toGray(0.33, 0.33, 0.34)
var grayDaveMinMax = dave.toGray('max')
fs.writeFileSync(__dirname + '/out/dave-gray.png', grayDave.toBuffer("png"));
fs.writeFileSync(__dirname + '/out/dave-grayminmax.png', grayDaveMinMax.toBuffer("png"));

console.log("OtsuAdaptiveThreshold");
var threshold = grayDave.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1);
fs.writeFileSync(__dirname + '/out/dave-threshold-values.png', threshold.thresholdValues.toBuffer("png"));
fs.writeFileSync(__dirname + '/out/dave-threshold-image.png', threshold.image.toBuffer("png"));

console.log("Deskew (Rotate)");
var skew = threshold.image.findSkew();
console.log(skew);
var daveDeskewed = dave.rotate(skew.angle);
fs.writeFileSync(__dirname + '/out/dave-deskewed.png', daveDeskewed.toBuffer("png"));
