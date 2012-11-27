global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

var writeImage = function(basename, image){
    fs.writeFileSync(__dirname + '/fixtures_out/' + basename, image.toBuffer('png'));
}

describe('Image', function(){
    before(function(){
        this.gray = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/dave.png'));
        this.rgba = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/rgba.png'));
    })
    it('should save using #toBuffer()', function(){
        writeImage('gray.png', this.gray);
        writeImage('rgba.png', this.rgba);
    })
    it('should #invert()', function(){
        writeImage('gray-invert.png', this.gray.invert());
    })
    it('should #or(), #and(), #xor() and #subtract()', function(){
        var a = this.gray.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1).image;
        var b = this.gray.erode(5, 5).otsuAdaptiveThreshold(16, 16, 0, 0, 0.1).image;
        writeImage('gray-boole-a.png', a);
        writeImage('gray-boole-b.png', b);
        writeImage('gray-boole-or.png', a.or(b));
        writeImage('gray-boole-and.png', a.and(b));
        writeImage('gray-boole-xor.png', a.xor(b));
        writeImage('gray-boole-subtract.png', a.subtract(b));
    })
    it('should #rotate()', function(){
        writeImage('gray-rotate.png', this.gray.rotate(-0.703125));
        writeImage('gray-rotate45.png', this.gray.rotate(45));
    })
    it('should #scale()', function(){
        writeImage('gray-scale2.png', this.gray.scale(2.0, 2.0));
        writeImage('gray-scale05.png', this.gray.scale(0.5, 0.5));
    })
    it('should #crop()', function(){
        writeImage('gray-crop.png', this.gray.crop(100, 100, 100, 100));
    })
    it('should #rankFilter()', function(){
        writeImage('gray-rankfilter-median.png', this.gray.rankFilter(3, 3, 0.5));
    })
    it('should #toGray()', function(){
        writeImage('rgba-gray-33.png', this.rgba.toGray(0.33, 0.33, 0.34));
        writeImage('rgba-gray-min.png', this.rgba.toGray('min'));
        writeImage('rgba-gray-max.png', this.rgba.toGray('max'));
        writeImage('gray-gray-max.png', this.gray.toGray('max'));
    })
    it('should #erode()', function(){
        writeImage('gray-erode.png', this.gray.erode(3, 3));
    })
    it('should #dilate()', function(){
        writeImage('gray-dilate.png', this.gray.dilate(3, 3));
    })
    it('should #otsuAdaptiveThreshold(), #findSkew()', function(){
        var threshold = this.gray.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1);
        writeImage('gray-threshold-values.png', threshold.thresholdValues);
        writeImage('gray-threshold-image.png', threshold.image);
        var skew = threshold.image.findSkew();
        skew.angle.should.equal(-0.703125);
        skew.confidence.should.equal(4.957831859588623);
    })
    it('should #connectedComponents()', function(){
        var textpage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
        var binaryImage = textpage.otsuAdaptiveThreshold(32, 32, 0, 0, 0.1).image;
        var boxes = binaryImage.connectedComponents(4);
        var canvas = new dv.Image(binaryImage);
        for (var i in boxes) {
            canvas.drawBox(boxes[i].x, boxes[i].y,
                           boxes[i].width, boxes[i].height,
                           2, 'set');
        }
        writeImage('textpage-components.png', canvas);
    })
    it('should #drawBox()', function(){
        var canvas = new dv.Image(this.gray)
        .drawBox(50, 50, 100, 100, 5)
        .drawBox(100, 100, 100, 100, 5, 'clear')
        .drawBox(150, 150, 100, 100, 5, 'flip');
        writeImage('gray-box.png', canvas);
    })
})
