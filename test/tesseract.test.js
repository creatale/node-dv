global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');
var util = require('util')

var textParagraph =
        ('Mr do raising article general norland my hastily. Its companions say uncommonly pianoforte ' +
         'favourable. Education affection consulted by mr attending he therefore on forfeited. High way ' +
         'more far feet kind evil play led. Sometimes furnished collected add for resources attention. ' +
         'Norland an by minuter enquire it general on towards forming. Adapted mrs totally company ' +
         'two yet conduct men.').replace(/\s/g, '').toLowerCase();

var compareTextParagraph = function(text){
    var plainText =  text.replace(/\s/g, '').toLowerCase();
    for (var i = 0; i < 6; ++i) {
        var paragraph = plainText.substr(i * textParagraph.length, textParagraph.length);
        paragraph.should.equal(textParagraph, 'Paragraph ' + i);
    }
}

var writeImageBoxes = function(basename, image, array){
    //console.log(util.inspect(array, false, 100));
    var canvas = new dv.Image(image);
    for (var i in array) {
        canvas.drawBox(array[i].box.x, array[i].box.y,
                       array[i].box.width, array[i].box.height,
                       2, 'flip')
    }
    fs.writeFileSync(__dirname + '/fixtures_out/' + basename, canvas.toBuffer('png'));
}

describe('Tesseract', function(){
    before(function(){
        this.textPage300 = new dv.Image("png", fs.readFileSync(__dirname + '/fixtures/textpage300.png'));
        this.tesseract = new dv.Tesseract();
        fs.writeFileSync(__dirname + '/fixtures_out/textpage300.png', this.textPage300.toBuffer('png'));
    })
    it('should #clear()', function(){
        this.tesseract.clearAdaptiveClassifier();
    })
    it('should #clearAdaptiveClassifier()', function(){
        this.tesseract.clearAdaptiveClassifier();
    })
    it('should set #image', function(){
        this.tesseract.image = this.textPage300;
    })
    it('should set/get #symbolWhitelist', function(){
        this.tesseract.symbolWhitelist = '0123456789';
        this.tesseract.symbolWhitelist.should.equal('0123456789');
        this.tesseract.symbolWhitelist = '';
    })
    it('should #findRegions()', function(){
        writeImageBoxes('textpage300-regions.png', this.textPage300, this.tesseract.findRegions());
    })
    it('should #findRegions(false)', function(){
        writeImageBoxes('textpage300-regions.png', this.textPage300, this.tesseract.findRegions(false));
    })
    it('should #findTextLines()', function(){
        writeImageBoxes('textpage300-lines.png', this.textPage300, this.tesseract.findTextLines());
    })
    it('should #findTextLines(false)', function(){
        writeImageBoxes('textpage300-lines.png', this.textPage300, this.tesseract.findTextLines(false));
    })
    it('should #findParagraphs()', function(){
        writeImageBoxes('textpage300-para.png', this.textPage300, this.tesseract.findParagraphs());
    })
    it('should #findParagraphs(false)', function(){
        writeImageBoxes('textpage300-para.png', this.textPage300, this.tesseract.findParagraphs(false));
    })
    it('should #findWords()', function(){
        writeImageBoxes('textpage300-words.png', this.textPage300, this.tesseract.findWords());
    })
    it('should #findWords(false)', function(){
        writeImageBoxes('textpage300-words.png', this.textPage300, this.tesseract.findWords(false));
    })
    it('should #findSymbols()', function(){
        writeImageBoxes('textpage300-symbols.png', this.textPage300, this.tesseract.findSymbols());
    })
    it('should #findSymbols(false)', function(){
        writeImageBoxes('textpage300-symbols.png', this.textPage300, this.tesseract.findSymbols(false));
    })
    it('should #findText(\'plain\')', function(){
        this.tesseract.image = this.textPage300;
        compareTextParagraph(this.tesseract.findText('plain'));
    })
    it('should #findText(\'plain\', true)', function(){
        this.tesseract.image = this.textPage300;
        var result = this.tesseract.findText('plain', true);
        compareTextParagraph(result.text);
        result.confidence.should.be.above(90);
    })
})
