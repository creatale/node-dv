global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

var console = {
    'log': function() {}
}

var writeImage = function(basename, image){
    fs.writeFileSync(__dirname + '/fixtures_out/' + basename, image.toBuffer('png'));
}

var checkboxToScore = function(checkboxImage) {
    var closedCheckbox = checkboxImage.dilate(31, 31).erode(31, 31);
    var markedArea = closedCheckbox.distanceFunction(8).and(checkboxImage.invert().toGray());
    var histogram = markedArea.histogram();
    var score = 0;
    for (var value in histogram) {
        score += histogram[value] * value;
    }
    return score;
}

describe('Examples', function(){
    it('should print some text', function(){
        var image = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
        var tesseract = new dv.Tesseract('eng', image);
        console.log(tesseract.findText('plain'));
    })
    it('should isolate barcode canidates', function(){
        var barcodes = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/formpage300.png'));
        var open = barcodes.thin('bg', 8, 5).dilate(3, 3);
        var openMap = open.distanceFunction(8);
        var openMask = openMap.threshold(10).erode(11*2, 11*2);
        writeImage('barcodes-open.png', open);
        writeImage('barcodes-openMap.png', openMap.maxDynamicRange('log'));
        writeImage('barcodes-openMask.png', openMask);
        var boxes = openMask.invert().connectedComponents(8);
        for (var i in boxes) {
            barcodes.drawBox(boxes[i].x, boxes[i].y,
                             boxes[i].width, boxes[i].height,
                             2, 'flip');
        }
        writeImage('barcodes-isolated.png', barcodes);
    })
    it('should score checkboxes', function(){
        var checkboxes = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkboxes.png'));
        var checkboxesFuzzy = checkboxes.dilate(3, 3).erode(3, 3);
        writeImage('checkboxes-fuzzy.png', checkboxesFuzzy);
        var boxes = checkboxesFuzzy.threshold(128).connectedComponents(8);
        for (var i in boxes) {
            var checkboxImage = checkboxes.crop(boxes[i].x, boxes[i].y,
                                                boxes[i].width, boxes[i].height);
            console.log("Checkbox", i, checkboxToScore(checkboxImage));
        }
    })
})
