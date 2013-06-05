{
  'includes': [ '../../common.gyp' ],
  'targets': [
    {
      'target_name': 'lodepng',
      'type': 'static_library',
      'include_dirs': [
        '.',
      ],
      'sources': [
        './lodepng.cpp',
      ],
    },
  ]
}
