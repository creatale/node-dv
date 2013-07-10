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
