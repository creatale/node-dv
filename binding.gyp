{
  "targets": [
    {
      'target_name': 'dvBinding',
      'dependencies': [
        'deps/lodepng/lodepng.gyp:lodepng',
        'deps/tesseract/tesseract.gyp:libtesseract',
        'deps/zbar/zbar.gyp:libzbar',
      ],
      'include_dirs': [
        'deps/lodepng',
        'deps/leptonica/src',
        'deps/tesseract/api',
        'deps/tesseract/ccmain',
        'deps/tesseract/ccstruct',
        'deps/tesseract/ccutil',
        'deps/tesseract/classify',
        'deps/tesseract/cutil',
        'deps/tesseract/dict',
        'deps/tesseract/image',
        'deps/tesseract/textord',
        'deps/tesseract/viewer',
        'deps/tesseract/wordrec',
        'deps/zbar',
      ],
      'sources': [
        'src/image.cc',
        'src/tesseract.cc',
        'src/zbar.cc',
        'src/module.cc',
      ],
    },
  ]
}
