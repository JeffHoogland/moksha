cmd = [ edje_cmd,
        '-id', join_paths(meson.source_root(), 'src' , 'modules', m, 'data'),
        '@INPUT@', '@OUTPUT@'
      ]
edc = 'shotedit'
custom_target(edc + '.edj',
              input        : edc + '.edc',
              output       : edc + '.edj',
              command      : cmd,
              install_dir  : _dir,
              install_mode : 'rw-r--r--',
              install      : true
              )
out = join_paths(_dir, edc + '.edj')
install_data(['Puk-Regular.ttf',
              'Yantiq.ttf',
              'intuitive.ttf',
              'IndieFlower.ttf',
              'shots.desktop'],
             install_dir: _dir)
