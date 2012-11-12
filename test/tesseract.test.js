global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');

var textParagraph =
        ('Mr do raising article general norland my hastily. Its companions say uncommonly pianoforte ' +
         'favourable. Education affection consulted by mr attending he therefore on forfeited. High way ' +
         'more far feet kind evil play led. Sometimes furnished collected add for resources attention. ' +
         'Norland an by minuter enquire it general on towards forming. Adapted mrs totally company ' +
         'two yet conduct men.').replace(/\s/g, '');

describe('Tesseract', function(){
    before(function(){
        this.tesseract = new dv.Tesseract();
        this.textPage300 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
    })
    it('should have defaults', function(){
        should.not.exist(this.tesseract.image);
        should.not.exist(this.tesseract.rectangle);
        this.tesseract.pageSegMode.should.equal('single_block');
    })
    it('should not crash', function(){
        //XXX: tests could be better.
        this.tesseract.image = this.textPage300;
        this.tesseract.findRegions();
        this.tesseract.findTextLines();
        this.tesseract.findWords();
        this.tesseract.findSymbols();
        this.tesseract.clear();
        this.tesseract.clearAdaptiveClassifier();
    })
    describe('#findText()', function(){
        it('should find plain text', function(done){
            this.tesseract.image = this.textPage300;
            var plainText = this.tesseract.findText('plain').replace(/\s/g, '');
            for (var i = 0; i < 6; ++i) {
                var paragraph = plainText.substr(i * textParagraph.length, textParagraph.length);
                paragraph.should.equal(textParagraph, 'Paragraph ' + i);
            }
        })
    })
})
