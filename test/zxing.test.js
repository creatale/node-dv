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
    describe('#findCode()', function(){
        it('should find ITF (truncated)', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode1.png'));
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('301036108');
        })
        it('should find ITF', function(){
            this.zxing.image = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/barcode2.png'));
            var code = this.zxing.findCode();
            code.type.should.equal('ITF');
            code.data.should.equal('12345678901231');
        })
    })
})
