project('line-lmi-calibration', 'cpp', default_options : ['cpp_std=c++20'])

opencv = dependency('opencv4', version : '>=4.0')

executable('calibrate', ['src/calibration.cpp', 'src/calibrate.cpp'], dependencies : [opencv])
