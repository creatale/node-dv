global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

describe('ZXing', function(){
    before(function(){
        this.zxing = new dv.ZXing();
    })
    it('should have defaults', function(){
        should.not.exist(this.zxing.image);
    })
    it('should have formats set', function(){
        should.exist(this.zxing.formats);
    })
    describe('#findCode()', function(){
        it('should find nothing', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
            var code = this.zxing.findCode();
            should.not.exist(code);
        })
        it('should find ITF-10', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode1.png'));
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('1234567890');
            should.exist(code.points);
        })
        it('should find ITF-14', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode2.png'));
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('12345678901231');
            should.exist(code.points);
        })
        it('should find PDF417', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode3.png'));
            var code = this.zxing.findCode();
            code.type.should.equal('PDF417');
            code.data.should.equal('This PDF417 barcode has error correction level 4');
            should.exist(code.points);
        })
        it('should find multiple barcodes', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcodes.png'));
            var codes = this.zxing.findCodes();
            codes[0].data.should.equal('Hello World!');
            codes[1].data.should.equal('Hello World');
        })
    })
})
