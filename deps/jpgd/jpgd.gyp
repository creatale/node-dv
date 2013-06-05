{
  'includes': [ '../../common.gyp' ],
  'targets': [
    {
      'target_name': 'jpgd',
      'type': 'static_library',
      'include_dirs': [
        '.',
      ],
      'sources': [
        './jpgd.cpp',
      ],
    },
  ]
}
