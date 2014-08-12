global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

describe('ZXing', function(){
    this.timeout(10000);
    before(function(){
        this.zxing = new dv.ZXing();
        this.textpage300 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
        this.barcode1 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode1.png'));
        this.barcode2 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode2.png'));
        this.barcode3 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode3.png'));
    })
    it('should have no #image set', function(){
        should.not.exist(this.zxing.image);
    })
    it('should set #image to null', function(){
        this.zxing.image = null;
        should.not.exist(this.zxing.image);
    })
    it('should have #formats set', function(){
        should.exist(this.zxing.formats);
    })
    it('should have #tryHarder', function(){
        should.exist(this.zxing.tryHarder);
    })
    describe('#findCode()', function(){
        it('should find nothing', function(){
            this.zxing.image = this.textpage300;
            var code = this.zxing.findCode();
            should.not.exist(code);
        })
        it('should find ITF-10', function(){
            this.zxing.image = this.barcode1;
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('1234567890');
            should.exist(code.points);
        })
        it('should find ITF-14', function(){
            this.zxing.image = this.barcode2;
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('12345678901231');
            should.exist(code.points);
        })
        it('should find PDF417', function(){
            this.zxing.image = this.barcode3;
            var code = this.zxing.findCode();
            code.type.should.equal('PDF_417');
            code.data.should.equal('This PDF417 barcode has error correction level 4');
            should.exist(code.points);
        })
    })
    describe('#findCode() with tryHarder', function(){
        before(function(){
            this.zxing.tryHarder = true;
        })
        it('should find nothing', function(){
            this.zxing.image = this.textpage300
            var code = this.zxing.findCode();
            should.not.exist(code);
        })
        it('should find ITF-10', function(){
            this.zxing.image = this.barcode1;
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('1234567890');
            should.exist(code.points);
        })
    })
})
