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
    it('should #save()', function(){
        writeImage('gray.png', this.gray);
        writeImage('rgba.png', this.rgba);
    })
    it('should #toGray()', function(){
        writeImage('rgba-gray-33.png', this.rgba.toGray(0.33, 0.33, 0.34));
        writeImage('rgba-gray-min.png', this.rgba.toGray('min'));
        writeImage('rgba-gray-max.png', this.rgba.toGray('max'));
        writeImage('gray-gray-max.png', this.gray.toGray('max'));
    })
    it('should #otsuAdaptiveThreshold(), #findSkew()', function(){
        var threshold = this.gray.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1);
        writeImage('gray-threshold-values.png', threshold.thresholdValues);
        writeImage('gray-threshold-image.png', threshold.image);
        var skew = threshold.image.findSkew();
        skew.angle.should.equal(-0.703125);
        skew.confidence.should.equal(4.957831859588623);
    })
    it('should #rotate()', function(){
        writeImage('gray-rotate.png', this.gray.rotate(-0.703125));
    })
    it('should #crop()', function(){
        writeImage('gray-crop.png', this.gray.crop(100, 100, 100, 100));
    })
    it('should #erode()', function(){
        writeImage('gray-erode.png', this.gray.erode(3, 3));
    })
    it('should #dilate()', function(){
        writeImage('gray-dilate.png', this.gray.dilate(3, 3));
    })
    it('should #rankFilter()', function(){
        writeImage('gray-rankfilter-median.png', this.gray.rankFilter(3, 3, 0.5));
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
})
