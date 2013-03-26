global.should = require('chai').should();
var dv = require('../lib/dv');
var fs = require('fs');
var util = require("util");

describe('TickReader', function(){
    before(function(){
        this.tickReader = new dv.TickReader(
                 { 'outerCheckedThreshold': 0.7,
                     'outerCheckedTrueMargin': 0.1,
                     'outerCheckedFalseMargin': 0.1,
                     'innerCheckedThreshold': 0.04,
                     'innerCheckedTrueMargin': 0.03,
                     'innerCheckedFalseMargin': 0.03});
    })
    it('should project horizontally', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var horizontalProjection = this.tickReader.getHorizontalProjection(checkboxImage);
        //console.log(horizontalProjection);
        var expectedResult =
                [ 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,
                 60, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2, 60,
                 0, 0, 0, 0, 0, 0, 0, 0, 0,  0];
        for (var i = 0; i < expectedResult.length; i++) {
            horizontalProjection.values[i].should.equal(expectedResult[i], i);
        }
    })
    it('should project vertically', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var verticalProjection = this.tickReader.getVerticalProjection(checkboxImage);
        //console.log(verticalProjection);
        var expectedResult =
                [ 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,
                 60, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2, 60,
                 0, 0, 0, 0, 0, 0, 0, 0, 0,  0];
        for (var i = 0; i < expectedResult.length; i++) {
            verticalProjection.values[i].should.equal(expectedResult[i], i);
        }
    })
    it('should project (asymmetric)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_asymmetric_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var horizontalProjection = this.tickReader.getHorizontalProjection(checkboxImage);
        var verticalProjection = this.tickReader.getVerticalProjection(checkboxImage);
        //console.log(horizontalProjection);
        //console.log(verticalProjection);
        var horizontalExpectedResult =
                [60, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,
                 2, 2, 2, 2, 2, 2, 2, 2, 2, 60,
                 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,
                 0, 0, 0, 0, 0, 0, 0, 0, 0,  0];
        var verticalExpectedResult =
                [0, 0, 0, 0,  0, 60, 2, 2, 2, 2,
                 2, 2, 2, 2,  2,  2, 2, 2, 2, 2,
                 2, 2, 2, 2,  2,  2, 2, 2, 2, 2,
                 2, 2, 2, 2,  2,  2, 2, 2, 2, 2,
                 2, 2, 2, 2,  2,  2, 2, 2, 2, 2,
                 2, 2, 2, 2,  2,  2, 2, 2, 2, 2,
                 2, 2, 2, 2, 60,  0, 0, 0, 0, 0,
                 0, 0, 0, 0,  0,  0, 0, 0, 0, 0];
        for (var i = 0; i < horizontalExpectedResult.length; i++) {
            horizontalProjection.values[i].should.equal(horizontalExpectedResult[i], 'horizontal ' + i);
        }
        for (var i = 0; i < verticalExpectedResult.length; i++) {
            verticalProjection.values[i].should.equal(verticalExpectedResult[i], 'vertical ' + i);
        }
    })
    it('should project checked', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_checked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var horizontalProjection = this.tickReader.getHorizontalProjection(checkboxImage);
        var verticalProjection = this.tickReader.getVerticalProjection(checkboxImage);
        //console.log(horizontalProjection);
        //console.log(verticalProjection);
        var horizontalExpectedResult =
                [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 0,  0,  0,  0,  0,  0,  0,  0,  0,  0];
        var verticalExpectedResult =
                [  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
                 0,  0,  0,  0,  0,  0,  0,  0,  0,  0];
        for (var i = 0; i < horizontalExpectedResult.length; i++) {
            horizontalProjection.values[i].should.equal(horizontalExpectedResult[i], 'horizontal ' + i);
        }
        for (var i = 0; i < verticalExpectedResult.length; i++) {
            verticalProjection.values[i].should.equal(verticalExpectedResult[i], 'vertical ' + i);
        }
    })
    /*it('should locate peaks at index 10 and 69', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var peaks = this.tickReader.locatePeaks(this.tickReader.getHorizontalProjection(checkboxImage), 15, 3);
        var expectedResult = [10, 69];
        for (var i = 0; i < expectedResult.length; i++) {
            peaks[i].should.equal(expectedResult[i], i);
        }
    })
    it('should locate peaks at index 10 and 69', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var peaks = this.tickReader.locatePeaks(this.tickReader.getVerticalProjection(checkboxImage), 15, 3);
        var expectedResult = [10, 69];
        for (var i = 0; i < expectedResult.length; i++) {
            peaks[i].should.equal(expectedResult[i], i);
        }
    })
    it('should locate peak and 69', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var peaks = this.tickReader.locatePeaks(this.tickReader.getHorizontalProjection(checkboxImage), 80, 3);
        var expectedResult = [69];
        for (var i = 0; i < expectedResult.length; i++) {
            peaks[i].should.equal(expectedResult[i], i);
        }
    })
    it('should calculate fill to 0', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fill = this.tickReader.getFill(checkboxImage, 11, 68, 11, 68);
        fill.should.equal(0);
    })
    it('should calculate fill to 236', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fill = this.tickReader.getFill(checkboxImage, 10, 69, 10, 69);
        fill.should.equal(236);
    })
    it('should calculate fill ratio to 0', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fillRatio = this.tickReader.getFillRatio(checkboxImage, 11, 68, 11, 68);
        fillRatio.should.equal(0.0);
    })
    it('should calculate fill ratio to 0', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fillRatios = this.tickReader.getFillRatios(checkboxImage, 15, 3);
        var expectedResult = [0.0];
        for (var i = 0; i < expectedResult.length; i++) {
            fillRatios[i].should.equal(expectedResult[i], i);
        }
    })
    it('should calculate fill ratio to 0 (asymmetric)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_asymmetric_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fillRatios = this.tickReader.getFillRatios(checkboxImage, 15, 3);
        var expectedResult = [0.0];
        for (var i = 0; i < expectedResult.length; i++) {
            fillRatios[i].should.equal(expectedResult[i], i);
        }
    })
    it('should calculate fill ratio to 0, without threshold', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var fillRatios = this.tickReader.getFillRatios(checkboxImage, 15, 3);
        var expectedResult = [0.0];
        for (var i = 0; i < expectedResult.length; i++) {
            fillRatios[i].should.equal(expectedResult[i], i);
        }
    })
    it('should calculate fill ratio to ?', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var peaks = this.tickReader.locatePeaks(this.tickReader.getHorizontalProjection(checkboxImage), 15, 3);
        //console.log(util.inspect(peaks, false, 5));
        var fillRatios = this.tickReader.getFillRatios(checkboxImage, 15, 3);
    })
    it('should calculate fill ratio to ?', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_unchecked_skewed.png'));
        checkboxImage = checkboxImage.toGray('max');
        var peaks = this.tickReader.locatePeaks(this.tickReader.getHorizontalProjection(checkboxImage), 15, 3);
        //console.log(util.inspect(peaks, false, 5));
        var fillRatios = this.tickReader.getFillRatios(checkboxImage, 15, 3);
    })*/
    it('should be unchecked (checkbox_perfect_unchecked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be unchecked (checkbox_perfect_asymmetric_unchecked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_asymmetric_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be unchecked (checkbox_scan_unchecked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_unchecked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be unchecked (checkbox_scan_unchecked_skewed.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_unchecked_skewed.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_perfect_checked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_checked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_perfect_checked2.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_perfect_checked2.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_checked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_checked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_checked.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_checked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_checked2.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_checked2.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_checked3.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_checked3.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig1.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig1.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig2.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig2.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig3.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig3.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig4.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig4.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig5.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig5.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be unchecked (checkbox_scan_orig6.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig6.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_orig7.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_orig7.png'));
        checkboxImage = checkboxImage.toGray('max');
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    /*it('should project horizontally without threshold', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_checked.png'));
        checkboxImage = checkboxImage.toGray('max');
        var horizontalProjection = this.tickReader.getHorizontalProjection(checkboxImage, 100);
        var verticalProjection = this.tickReader.getVerticalProjection(checkboxImage, 100);
        //console.log(util.inspect(horizontalProjection));
        //console.log(util.inspect(verticalProjection));
    })*/
    it('should be checked (checkbox_scan_gender1.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_gender1.png'));
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': true, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
    it('should be checked (checkbox_scan_gender2.png)', function(){
        var checkboxImage = new dv.Image('png', fs.readFileSync(__dirname + '/fixtures/checkbox_scan_gender2.png'));
        var boxes = this.tickReader.checkboxIsChecked(checkboxImage);
        var expectedResult = {'checked': false, 'confidence': 1.0};
        //console.log(util.inspect(boxes, false, 5));
        for (var i = 0; i < expectedResult.length; i++) {
            boxes.checked.should.equal(expectedResult.checked, i + ', checked');
            //boxes[i].confidence.should.equal(expectedResult[i].confidence, i + ', confidence');
        }
    })
})
