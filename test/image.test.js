global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

describe('Image', function(){
    before(function(){
        this.dave = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/dave.png'));
    })
    it('should #load() and #save()', function(){
        var textPage = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
        var rgba = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/rgba.png'));
        fs.writeFileSync(__dirname + '/fixtures_out/textpage300.png', textPage.toBuffer("png"));
        fs.writeFileSync(__dirname + '/fixtures_out/dave.png', this.dave.toBuffer("png"));
        fs.writeFileSync(__dirname + '/fixtures_out/rgba.png', rgba.toBuffer("png"));
    })
    it('should #toGray(), #otsuAdaptiveThreshold(), #deskew() and #rotate()', function(){
        var grayDave = this.dave.toGray(0.33, 0.33, 0.34)
        var grayDaveMinMax = this.dave.toGray('max')
        fs.writeFileSync(__dirname + '/fixtures_out/dave-gray.png', grayDave.toBuffer("png"));
        fs.writeFileSync(__dirname + '/fixtures_out/dave-grayminmax.png', grayDaveMinMax.toBuffer("png"));
        var threshold = grayDave.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1);
        fs.writeFileSync(__dirname + '/fixtures_out/dave-threshold-values.png', threshold.thresholdValues.toBuffer("png"));
        fs.writeFileSync(__dirname + '/fixtures_out/dave-threshold-image.png', threshold.image.toBuffer("png"));
        var skew = threshold.image.findSkew();
        skew.angle.should.equal(-0.703125);
        skew.confidence.should.equal(4.957831859588623);
        var daveDeskewed = this.dave.rotate(skew.angle);
        fs.writeFileSync(__dirname + '/fixtures_out/dave-deskewed.png', daveDeskewed.toBuffer("png"));
    })
})
