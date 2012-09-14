{
  "targets": [
    {
      'target_name': 'dvBinding',
      'dependencies': [
        'deps/tesseract/tesseract.gyp:libtesseract'
      ],
      'include_dirs': [
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
      ],
      'sources': [
        'src/image.cc',
        'src/module.cc',
        'src/tesseract.cc',
      ],
    },
  ]
}
