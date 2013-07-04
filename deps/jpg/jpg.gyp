{
  'includes': [ '../common.gyp' ],
  'targets': [
    {
      'target_name': 'jpg',
      'type': 'static_library',
      'include_dirs': [
        '.',
      ],
      'sources': [
        './jpgd.cpp',
        './jpge.cpp',
      ],
    },
  ]
}
