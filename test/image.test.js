global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

String.prototype.endsWith = function(suffix) {
    return this.indexOf(suffix, this.length - suffix.length) !== -1;
};

var writeImage = function(basename, image){
    if (basename.endsWith('jpg')) {
        fs.writeFileSync(__dirname + '/fixtures_out/' + basename, image.toBuffer('jpg'));
    } else {
        fs.writeFileSync(__dirname + '/fixtures_out/' + basename, image.toBuffer('png'));
    }
}

describe('Image', function(){
    this.timeout(3000);
    this.slow(250);
    before(function(){
        this.gray = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/dave.png'));
        this.rgb = new dv.Image('jpg', fs.readFileSync(__dirname + '/fixtures/rgb.jpg'));
        this.rgba = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/rgba.png'));
        this.rgbaBuffer = new Buffer(128 * 256 * 4);
        for (var y = 0; y < 256; y++) {
            for (var x = 0; x < 128; x++) {
                // Fill a buffer with red, green and blue stripes.
                for (var c = 0; c < 3; c++) {
                    this.rgbaBuffer[y * 128 * 4 + x * 4 + c] = x % 4 == c ? 255 : 0;
                }
                this.rgbaBuffer[y * 128 * 4 + x * 4 + c + 3] = 128;
            }
        }
        this.rgbBuffer = new Buffer(128 * 256 * 3);
        for (var y = 0; y < 256; y++) {
            for (var x = 0; x < 128; x++) {
                // Fill a buffer with red, green and blue stripes.
                for (var c = 0; c < 3; c++) {
                    this.rgbBuffer[y * 128 * 3 + x * 3 + c] = x % 3 == c ? 255 : 0;
                }
            }
        }
        this.textpage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
    })
    it('should save using #toBuffer()', function(){
        writeImage('gray.jpg', this.gray);
        writeImage('rgb.jpg', this.rgb);
        writeImage('gray.png', this.gray);
        writeImage('rgb.png', this.rgb);
        writeImage('rgba.png', this.rgba);
        writeImage('rgbaBuffer.png', new dv.Image('rgba', this.rgbaBuffer, 128, 256));
        writeImage('rgbBuffer.png', new dv.Image('rgb', this.rgbBuffer, 128, 256));
        writeImage('whd.png', new dv.Image(128, 128, 8));
        writeImage('composed.png', new dv.Image(this.gray, this.textpage, this.textpage));
    })
    it('should return raw image data using #toBuffer()', function(){
        var buf = new dv.Image('rgb', this.rgbBuffer, 128, 256).toBuffer();
        buf.length.should.equal(this.rgbBuffer.length);
        for (var i = 0; i < 1000; i++) {
            buf[i].should.equal(this.rgbBuffer[i]);
        }
    })
    it('should #invert()', function(){
        writeImage('gray-invert.png', this.gray.invert());
    })
    it('should #or(), #and(), #xor(), #add() and #subtract()', function(){
        var a = this.gray.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1).image;
        var b = this.gray.erode(5, 5).otsuAdaptiveThreshold(16, 16, 0, 0, 0.1).image;
        writeImage('gray-boole-a.png', a);
        writeImage('gray-boole-b.png', b);
        writeImage('gray-boole-or.png', a.or(b));
        writeImage('gray-boole-and.png', a.and(b));
        writeImage('gray-boole-xor.png', a.xor(b));
        writeImage('gray-boole-add.png', a.add(b));
        writeImage('gray-boole-subtract.png', a.subtract(b));
    })
    it('should #add() and #subtract() arithmetically', function(){
        var red = this.rgba.toGray(1, 0, 0);
        var cyan = this.rgba.toGray(0, 0.5, 0.5);
        this.rgba.subtract(new dv.Image(this.rgba)).toBuffer()[0].should.equal(0);
        writeImage('gray-arith-add.png', red.add(cyan));
        writeImage('gray-arith-subtract.png', red.subtract(cyan));
        writeImage('color-arith-add.png', this.rgb.add(this.rgb));
    })
    it('should #convolve()', function(){
        writeImage('gray-convolve.png', this.gray.convolve(15, 15));
    })
    it('should #unsharp()', function(){
        writeImage('gray-unsharp.png', this.gray.unsharpMasking(6, 2.5));
    })
    it('should #rotate()', function(){
        writeImage('gray-rotate.png', this.gray.rotate(-0.703125));
        writeImage('gray-rotate45.png', this.gray.rotate(45));
    })
    it('should #resize()', function(){
        writeImage('gray-resize50100.png', this.gray.scale(50, 100));
        writeImage('gray-resize20.png', this.gray.scale(20));
    })
    it('should #scale()', function(){
        writeImage('gray-scale2.png', this.gray.scale(2.0, 2.0));
        writeImage('gray-scale05.png', this.gray.scale(0.5));
    })
    it('should #crop()', function(){
        writeImage('gray-crop.png', this.gray.crop(100, 100, 100, 100));
    })
    it('should #inRange()', function(){
        writeImage('rgb-inrange.png', this.rgb.inRange(0, 0, 0, 255, 10, 10));
    })
    it('should #rankFilter()', function(){
        writeImage('gray-rankfilter-median.png', this.gray.rankFilter(3, 3, 0.5));
    })
    it('should #toGray()', function(){
        writeImage('rgba-gray.png', this.rgba.toGray());
        writeImage('rgba-gray-33.png', this.rgba.toGray(0.33, 0.33, 0.34));
        writeImage('rgba-gray-min.png', this.rgba.toGray('min'));
        writeImage('rgba-gray-max.png', this.rgba.toGray('max'));
        writeImage('gray-gray-max.png', this.gray.toGray('max'));
    })
    it('should #toHSV() and #toRGB()', function(){
        var hsv = this.rgb.toHSV();
        writeImage('rgb-hsv.png', hsv);
        writeImage('rgb-hsv-rgb.png', hsv.toRGB());
    })
    it('should #erode()', function(){
        writeImage('gray-erode.png', this.gray.erode(3, 3));
    })
    it('should #dilate()', function(){
        writeImage('gray-dilate.png', this.gray.dilate(3, 3));
    })
    it('should #open()', function(){
        writeImage('gray-dilate.png', this.gray.open(3, 3));
    })
    it('should #close()', function(){
        writeImage('gray-dilate.png', this.gray.close(3, 3));
    })
    it('should #thin()', function(){
        writeImage('gray-thin.png', this.gray.thin('fg', 4, 3));
    })
    it('should #otsuAdaptiveThreshold(), #findSkew()', function(){
        var threshold = this.gray.otsuAdaptiveThreshold(16, 16, 0, 0, 0.1);
        writeImage('gray-threshold-values.png', threshold.thresholdValues);
        writeImage('gray-threshold-image.png', threshold.image);
        var skew = threshold.image.findSkew();
        skew.angle.should.equal(-0.703125);
        skew.confidence.should.equal(4.957831859588623);
    })
    it('should #lineSegments(), #drawLine()', function(){
        var segments = this.gray.lineSegments(3, 0, false);
        var canvas = this.gray.toColor();
        for (var i in segments) {
            var segment = segments[i];
            var t = i / segments.length;
            var u = t < 0.5 ? t * 2 : 0;
            var v = t >= 0.5 ? t * 2 : 0;
            canvas.drawLine(segment.p1, segment.p2, 2, 255,
                            Math.round(255 * u), Math.round(255 * v));
        }
        writeImage('gray-line-segments.png', canvas);
    })
    it('should #connectedComponents()', function(){
        var binaryImage = this.textpage.otsuAdaptiveThreshold(32, 32, 0, 0, 0.1).image;
        var boxes = binaryImage.connectedComponents(4);
        var canvas = new dv.Image(binaryImage);
        for (var i in boxes) {
            canvas.drawBox(boxes[i], 2, 'set');
        }
        writeImage('textpage-components.png', canvas);
    })
    it('should #distanceFunction() and #maxDynamicRange()', function(){
        var distanceMap = this.rgb.toGray().distanceFunction(4);
        writeImage('distance-map.png', distanceMap.maxDynamicRange('log'));
    })
	it('should #fillBox()', function(){
        var canvas = new dv.Image(this.gray)
        .fillBox(100, 100, 100, 100, 192)
        .fillBox(150, 150, 100, 100, 64)
        writeImage('gray-box2.png', canvas);
        var colorCanvas = canvas.toColor()
        .fillBox(25, 25, 100, 100, 255, 0, 0)
        .fillBox(75, 75, 100, 100, 0, 0, 255, 0.5);
        writeImage('gray-box2-color.png', colorCanvas);
    })
    it('should #drawBox()', function(){
        var canvas = new dv.Image(this.gray)
        .drawBox(50, 50, 100, 100, 5, 'set')
        .drawBox(100, 100, 100, 100, 5, 'clear')
        .drawBox(150, 150, 100, 100, 5, 'flip')
        writeImage('gray-box.png', canvas);
        var colorCanvas = canvas.toColor()
        .drawBox(25, 25, 100, 100, 5, 255, 0, 0)
        .drawBox(75, 75, 100, 100, 5, 0, 0, 255, 0.5);
        writeImage('gray-box-color.png', colorCanvas);
    })
    it('should #drawImage()', function(){
        var canvas = new dv.Image(512, 512, 8);
        canvas.drawImage(this.gray, 128, 128, 256, 256);
        writeImage('whd-drawImage.png', canvas);   
    })
    it('should #octreeColorQuant()', function() {
        writeImage('rgb-octreeColorQuant-16.png', this.rgb.medianCutQuant(16));
    })
    it('should #medianCutQuant()', function() {
        writeImage('rgb-medianCutQuant-16.png', this.rgb.medianCutQuant(16));
    })
    it('should #threshold()', function() {
        writeImage('gray-threshold-64.png', this.gray.threshold(64));
        writeImage('gray-threshold-196.png', this.gray.threshold(196));
    })
    it('should #histogram()', function() {
        var result = this.gray.histogram()
        result.length.should.equal(256);
        result[0].should.equal(0);
        result[128].should.be.within(0.00, 0.01);
        result[255].should.be.within(0.44, 0.45);
    })
	it('should #projection()', function() {
        var horizontal = this.gray.threshold(254).projection('horizontal');
        horizontal.length.should.equal(400);
        var vertical = this.gray.threshold(254).projection('vertical');
        vertical.length.should.equal(669);
    })
    it('should #applyCurve() and #setMasked()', function() {
        var curve = new Array(256);
        for (var i = 0; i < 256; i++)
            curve[i] = i / 2;
        curve[255] = 200;
        var mask = new dv.Image(this.gray).threshold(48).invert().clearBox(50, 50, 200, 100);
        writeImage('gray-curve.png', new dv.Image(this.gray).applyCurve(curve, mask));
        writeImage('gray-setmasked.png', new dv.Image(this.gray).setMasked(mask, 255));
    })
})
