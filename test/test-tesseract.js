var dv = require('../lib/dv');
var fs = require('fs');

var textPageImageData = fs.readFileSync(__dirname + '/fixtures/textpage300.png');
var textPageImage = new dv.Image("png", textPageImageData);
var textPageParagraph =
        'Mr do raising article general norland my hastily. Its companions say uncommonly pianoforte ' +
        'favourable. Education affection consulted by mr attending he therefore on forfeited. High way ' +
        'more far feet kind evil play led. Sometimes furnished collected add for resources attention. ' +
        'Norland an by minuter enquire it general on towards forming. Adapted mrs totally company ' +
        'two yet conduct men.';

function diffIndex(first, second) {
    if(first === second) {
        return -1;
    }
    first  = first.toString();
    second = second.toString();
    var minLen = Math.min(first.length, second.length);
    for(var i = 0; i < minLen; i++) {
        if(first.charAt(i) !== second.charAt(i)) {
            return i;
        }
    }
    return minLen;
}


var tesseract = new dv.Tesseract();
console.log('Tesseract version: ' + tesseract.version);
console.log('Tesseract image: ' + tesseract.image);
console.log('Tesseract rectangle: ' + tesseract.rectangle);
console.log('Tesseract page segmentation: ' + tesseract.pageSegMode);

// Test clearing.
tesseract.clear();
tesseract.clearAdaptiveClassifier();

// Test rectangle recognition.
tesseract.image = textPageImage;
console.log(tesseract.findRegions());
console.log(tesseract.findTextLines());
console.log(tesseract.findWords());
console.log(tesseract.findSymbols());

// Test text recognition.
var srcParagraph = textPageParagraph.replace(/\s/g, '');
var ocrText = tesseract.findText('plain').replace(/\s/g, '');
for (var i = 0; i < 6; ++i) {
    var paragraph = ocrText.substr(i * srcParagraph.length, srcParagraph.length);
    var index = diffIndex(srcParagraph, paragraph);
    if (index === -1) {
        console.log('Pass');
    } else {
        console.log('Fail at ' + index + ': '
                    + srcParagraph.substr(index - 10, 10) + ' ('
                    + srcParagraph.substr(index, 5) + ' | '
                    + paragraph.substr(index, 5) + ')');
    }
}
