# DocumentVision [![NPM version](https://badge.fury.io/js/dv.png)](http://badge.fury.io/js/dv) [![Build Status](https://travis-ci.org/creatale/node-dv.png)](https://travis-ci.org/creatale/node-dv) [![devDependency Status](https://david-dm.org/creatale/node-dv/dev-status.png?theme=shields.io)](https://david-dm.org/creatale/node-dv#info=devDependencies)

DocumentVision is a [node.js](http://nodejs.org) library for processing and understanding scanned documents.

## Features

- Image loading using [jpeg-compressor](http://code.google.com/p/jpeg-compressor/), [LodePNG](http://lodev.org/lodepng/) and pixel buffers
- Image manipulation using [Leptonica](http://www.leptonica.com/) (Version 1.69)
- OCR using [Tesseract](http://code.google.com/p/tesseract-ocr/) (Version 3.02, SVN r866)
- OMR for Barcodes using [ZXing](https://github.com/zxing/zxing) (Version 2.3.0)

## Installation

    $ npm install dv

## Quick Start

Once you've installed, download [that image](https://github.com/creatale/node-dv/blob/master/test/fixtures/textpage300.png). You can use any other image containing simple text at 300dpi or higher. Now run the following code snipped to recognize text from your image:

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
- [Tutorial: Preprocessing](https://github.com/creatale/node-dv/wiki/Tutorial-Preprocessing)
- [Tutorial: Recognition](https://github.com/creatale/node-dv/wiki/Tutorial-Recognition)
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

Licensed under the incredibly [permissive](http://en.wikipedia.org/wiki/Permissive_free_software_licence) [MIT License](http://creativecommons.org/licenses/MIT/). Copyright &copy; 2012 Christoph Schulz.
