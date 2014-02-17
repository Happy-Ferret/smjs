{
  'variables': {
    'library%': 'static_library',    # allow override to 'shared_library' for DLL/.so builds
    'component%': 'static_library',  # NB. these names match with what V8 expects
  },

  'target_defaults': {
    'default_configuration': 'Release',

    'sources': [
      'common.gypi',
    ],

    'configurations': {
      'Debug': {
        'defines': [ 'DEBUG', '_DEBUG' ],
        'cflags': [ '-g', '-O0' ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 1, # static debug
            'Optimization': 0, # /Od, no optimization
            'MinimalRebuild': 'false',
            'OmitFramePointers': 'false',
            'BasicRuntimeChecks': 3, # /RTC1
          },
          'VCLinkerTool': {
            'LinkIncremental': 2, # enable incremental linking
          },
        },
        'xcode_settings': {
          'GCC_OPTIMIZATION_LEVEL': '0', # stop gyp from defaulting to -Os
        },
      },

      'Release': {
        'defines': [ 'NDEBUG', '_NDEBUG' ],
        'cflags': [ '-O3', '-fstrict-aliasing', '-ffunction-sections', '-fdata-sections' ],
        'conditions': [
          ['OS != "mac" and OS != "win"', {
            'cflags': [ '-fno-omit-frame-pointer' ],
          }],
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': 0, # static release
            'Optimization': 3, # /Ox, full optimization
            'FavorSizeOrSpeed': 1, # /Ot, favour speed over size
            'InlineFunctionExpansion': 2, # /Ob2, inline anything eligible
            'WholeProgramOptimization': 'true', # /GL, whole program optimization, needed for LTCG
            'OmitFramePointers': 'true',
            'EnableFunctionLevelLinking': 'true',
            'EnableIntrinsicFunctions': 'true',
            'RuntimeTypeInfo': 'false',
          },
          'VCLibrarianTool': {
            'AdditionalOptions': [
              '/LTCG', # link time code generation
            ],
          },
          'VCLinkerTool': {
            'LinkTimeCodeGeneration': 1, # link-time code generation
            'OptimizeReferences': 2, # /OPT:REF
            'EnableCOMDATFolding': 2, # /OPT:ICF
            'LinkIncremental': 1, # disable incremental linking
          },
        },
      }
    },

    'conditions': [
      ['OS == "win"', {
        'defines': [
          'WIN32',
          # VC++ doesn't need to warn us about how dangerous C functions are.
          '_CRT_SECURE_NO_DEPRECATE',
          # ... or that C implementations shouldn't use posix names
          '_CRT_NONSTDC_NO_DEPRECATE',
          # ... or that we need to validate inputs.
          '_SCL_SECURE_NO_WARNINGS'
        ],
        'msvs_cygwin_shell': 0, # prevent actions from trying to use cygwin
        'msvs_settings': {
          'VCCLCompilerTool': {
            'BufferSecurityCheck': 'true',
            'DebugInformationFormat': 3, # Generate a PDB
            'ExceptionHandling': 1, # /EHsc
            'StringPooling': 'true', # pool string literals
            'SuppressStartupBanner': 'true',
            'WarnAsError': 'false',
            'WarningLevel': 3,
            'AdditionalOptions': [
              '/MP', # compile across multiple CPUs
            ],
          },
          'VCLibrarianTool': {
          },
          'VCLinkerTool': {
            'conditions': [
              ['target_arch=="x64"', {
                'TargetMachine' : 17 # /MACHINE:X64
              }],
            ],
            'GenerateDebugInformation': 'true',
            'RandomizedBaseAddress': 2, # enable ASLR
            'DataExecutionPrevention': 2, # enable DEP
            'AllowIsolation': 'true',
            'SuppressStartupBanner': 'true',
            'target_conditions': [
              ['_type=="executable"', {
                'SubSystem': 1, # console executable
              }],
            ],
          },
        },
        'conditions': [
          ['target_arch=="x64"', {
            'msvs_configuration_platform': 'x64',
          }],
        ],
      }],

      ['OS == "mac"', {
        'defines': ['_DARWIN_USE_64_BIT_INODE=1'],
        'xcode_settings': {
          'ALWAYS_SEARCH_USER_PATHS': 'NO',
          'GCC_CW_ASM_SYNTAX': 'NO',                # No -fasm-blocks
          'GCC_DYNAMIC_NO_PIC': 'NO',               # No -mdynamic-no-pic
                                                    # (Equivalent to -fPIC)
          'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',        # -fno-exceptions
          'GCC_ENABLE_CPP_RTTI': 'NO',              # -fno-rtti
          'GCC_ENABLE_PASCAL_STRINGS': 'NO',        # No -mpascal-strings
          'GCC_THREADSAFE_STATICS': 'NO',           # -fno-threadsafe-statics
          'PREBINDING': 'NO',                       # No -Wl,-prebind
          'MACOSX_DEPLOYMENT_TARGET': '10.5',       # -mmacosx-version-min=10.5
          'USE_HEADERMAP': 'NO',
          'OTHER_CFLAGS': [
            '-fno-strict-aliasing',
          ],
          'WARNING_CFLAGS': [
            '-Wall',
            '-Wendif-labels',
            '-W',
            '-Wno-unused-parameter',
          ],
        },
        'target_conditions': [
          ['_type!="static_library"', {
            'xcode_settings': {'OTHER_LDFLAGS': ['-Wl,-search_paths_first']},
          }],
        ],
        'conditions': [
          ['target_arch=="ia32"', {
            'xcode_settings': {'ARCHS': ['i386']},
          }],
          ['target_arch=="x64"', {
            'xcode_settings': {'ARCHS': ['x86_64']},
          }],
        ],
      }],

      ['OS != "win" and OS != "mac"', {
        'cflags': [ '-Wall', '-Wextra', '-Wno-unused-parameter', '-pthread', ],
        'cflags_cc': [ '-fno-rtti', '-fno-exceptions' ],
        'ldflags': [ '-pthread', '-rdynamic' ],
        'target_conditions': [
          ['_type=="static_library"', {
            'standalone_static_library': 1, # disable thin archive which needs binutils >= 2.19
          }],
        ],
        'conditions': [
          ['target_arch == "ia32"', {
            'cflags': [ '-m32' ],
            'ldflags': [ '-m32' ],
          }],
          ['target_arch == "x64"', {
            'cflags': [ '-m64' ],
            'ldflags': [ '-m64' ],
          }],
        ],
      }],
    ],
  }
}
