# DocumentVision [![NPM version](https://badge.fury.io/js/dv.png)](http://badge.fury.io/js/dv) [![Build Status](https://travis-ci.org/creatale/node-dv.png)](https://travis-ci.org/creatale/node-dv)
[![Windows Build status](https://ci.appveyor.com/api/projects/status/4c67hpay2xsrhbrv?svg=true)](https://ci.appveyor.com/project/rashfael/node-dv) [![devDependency Status](https://david-dm.org/creatale/node-dv/dev-status.png)](https://david-dm.org/creatale/node-dv#info=devDependencies)

DocumentVision is a [node.js](http://nodejs.org) library for processing and understanding scanned documents.

## Features

- Image loading using [jpeg-compressor](http://code.google.com/p/jpeg-compressor/), [LodePNG](http://lodev.org/lodepng/) and pixel buffers
- Image manipulation using [Leptonica](http://www.leptonica.com/) (Version 1.69) and [OpenCV](http://opencv.org/) (Version 2.4.9)
- Line Segment Detection using [LSWMS](http://sourceforge.net/projects/lswms/)
- OCR using [Tesseract](http://code.google.com/p/tesseract-ocr/) (Version 3.03)
- OMR for Barcodes using [ZXing](https://github.com/zxing/zxing) (Version 2.3.0)

## Installation

    $ npm install dv

## Quick Start

Once you've installed, download [that image](https://github.com/creatale/node-dv/blob/master/test/fixtures/textpage300.png). You can use any other image containing simple text at 300dpi or higher. Now run the following code snippet to recognize text from your image:

```javascript
var dv = require('dv');
var fs = require('fs');
var image = new dv.Image('png', fs.readFileSync('textpage300.png'));
var tesseract = new dv.Tesseract('eng', image);
console.log(tesseract.findText('plain'));
```

## What's next?

Here are some quick links to help you get started:

- [Introduction](https://github.com/creatale/node-dv/wiki/Introduction)
- [Tutorial](https://github.com/creatale/node-dv/wiki/Tutorial)
- [API Reference](https://github.com/creatale/node-dv/wiki/API)
- [Bug Tracker](https://github.com/creatale/node-dv/issues)
- [Changelog](CHANGELOG.md)

## Versioning

DocumentVision is maintained under the [Semantic Versioning](http://semver.org/) guidelines as much as possible:

- Version number format is `<major>.<minor>.<patch>`
- Breaking backward compatibility bumps the major (resetting minor and patch)
- New additions without breaking backward compatibility bumps the minor (resetting patch)
- Bug fixes and other changes bumps the patch

## License

Licensed under the [MIT License](http://creativecommons.org/licenses/MIT/). See [LICENSE](https://github.com/creatale/node-dv/blob/master/LICENSE).

External libraries are licensed under their respective licenses.
