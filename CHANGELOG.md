## v1.7.2 (May 27, 2014)
- Fix encoding issues for Tesseract variables

## v1.7.1 (March 29, 2014)
- Fix broken .npmignore

## v1.7.0 (March 28, 2014)
- Added Tesseract#&lt;variable&gt; accessor
- Fixed assert (see [Tesseract r878](http://code.google.com/p/tesseract-ocr/source/detail?r=878))
- Fixed wrong error message for  Tesseract#findText() 

Thanks to: Huang Xin

## v1.6.2 (March 25, 2014)
- `npm install` removes the build directy after install to free up space.

## v1.6.1 (January 8, 2014)
- Fixed reporting too little memory consumption to V8.

## v1.6.0 (December 17, 2013)
- Added Image#projection() and Image#fillBox.
- Update ZXing to 2.3.0.
- Update LodePNG to 20131115.
- Fixed several issues of unfreeable memory/leaks.

## v1.5.1 and v1.5.2 (October 24, 2013)

- Fixed Tesseract crashes related to rectangles and best first search.
- Added `sparse_text` and `sparse_text_osd` to error messages.

## v1.5.0 (September 5, 2013)

- Updated ZXing (bugfixes, performance)
- Updated TesseractOCR (sparse text mode)

## v1.4.0 (August 14, 2013)

- Added Image#open() and Image#close()
- Added Image#toHSV() and Image#toRGB()
- Added Image#inRange() and Image() constructor for channel composition
- Bugfixes

## v1.3.0 (July 10, 2013)
- **Moved tesseract data** to dv.data package
- Added Image#drawImage() and Image#Image(width, height, depth)
- Removed "ignore quite zone" hack for ITF barcodes, as its pure evil to do so.

## v1.2.1 (July 10, 2013)
- Image#add() and Image#subtract() no longer fail when both operants are identical
- Fixed ZXing#formats and ZXing#tryHarder not working sometimes

## v1.2.0 (July 9, 2013)
- Added `Image#add()`
- Deprecated `Image#unsharpMasking()` in favor of `Image#unsharp()`
- Added OSD training data (`tesseract.pageSegMode = 'auto_osd'`)

## v1.1.0 (July 4, 2013)
- Added `Image#toBuffer("png")`

## v1.0.0 (July 3, 2013)
- **Initial stable release**
