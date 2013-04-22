{
  "targets": [
    {
      'target_name': 'dvBinding',
      'dependencies': [
        'deps/jpgd/jpgd.gyp:jpgd',
        'deps/lodepng/lodepng.gyp:lodepng',
        'deps/tesseract/tesseract.gyp:libtesseract',
        'deps/zxing/zxing.gyp:libzxing',
      ],
      'include_dirs': [
        'deps/jpgd',
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
        'deps/zxing/core/src',
      ],
      'sources': [
        'src/image.cc',
        'src/tesseract.cc',
        'src/util.cc',
        'src/zxing.cc',
        'src/module.cc',
      ],
      'cflags!': ['-fno-exceptions'],
      'cflags_cc!': ['-fno-exceptions'],
      'conditions': [
        ['OS=="mac"',
          {
            'xcode_settings': {
              'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
            },
            'configurations': {
              'Debug': {
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'ExceptionHandling': '1',
                  },
                },
              },
              'Release': {
                'msvs_settings': {
                  'VCCLCompilerTool': {
                    'ExceptionHandling': '1',
                  },
                },
              },
            },
          }
        ],
      ],
    },
  ]
}
