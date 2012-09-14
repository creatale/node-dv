# Document Vision

Document Vision is a [node.js](http://nodejs.org) library for processing and understanding scanned documents.

## Features

- Image manipulation using [Leptonica](http://www.leptonica.com/)
- OCR using [Tesseract](http://code.google.com/p/tesseract-ocr/)

## Installation

	[sudo] npm install [-g] dv

## Quick Start

Once you've installed, download [that image](https://github.com/schulzch/node-dv/blob/master/test/fixtures/textpage300.png). You can use any other image containing simple text at 300dpi or higher. Now run the following code snipped to recognize text from your image:

	var dv = require('../lib/dv');
	var tesseract = new dv.Tesseract('.');
	tesseract.image = new dv.Image('textpage300.png');
	console.log(tesseract.findText('plain'));

## Whats next?

Here are some quick links to help you get started:

- [Reference Manual](https://github.com/schulzch/node-dv/wiki/Reference)
- [Troubleshooting](https://github.com/schulzch/node-dv/wiki/Troubleshooting)
- [Bug Tracker](https://github.com/schulzch/node-dv/issues)

## License

Licensed under the incredibly [permissive](http://en.wikipedia.org/wiki/Permissive_free_software_licence) [MIT License](http://creativecommons.org/licenses/MIT/). Copyright &copy; 2012 Christoph Schulz.
