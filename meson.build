project('line-lmi-calibration', 'cpp', default_options : ['cpp_std=c++20'])
add_project_arguments('-Wno-deprecated-anon-enum-enum-conversion, -Werror=return-type', language: 'cpp')

opencv = dependency('opencv4', version : '>=4.0')
#eigen = dependency('eigen3', version : '>=3.0')

#executable('calibrate', ['src/calibration.cpp', 'src/calibrate.cpp'], dependencies : [opencv])
#executable('apply_calibration', ['src/calibration.cpp', 'src/apply_calibration.cpp'], dependencies : [opencv])

#executable('rescale', ['src/detect_lines.cpp', 'src/rescale.cpp', 'src/lines.cpp', 'src/test_rescale.cpp'], dependencies : [opencv])

executable('scasub', ['src/lines.cpp', 'src/scasub.cpp'], dependencies : [opencv])

executable('select_lines', ['src/select_lines.cpp', 'src/lines.cpp'], dependencies : [opencv])
