{
  'target_defaults': {
    'default_configuration': 'Release',

    # XXX this should probably be specified in common.gypi
    'target_conditions': [
      ['target_arch == "x64"', {
        'msvs_configuration_platform': 'x64'
      }]
    ],

    'configurations': {
      'Debug': {
        'defines': ['DEBUG', 'JS_DEBUG', 'JS_GC_ZEAL'],
        'cflags': ['-g', '-O0'],
      },
      'Release': {
        'defines': ['NDEBUG'],
        'cflags': ['-O3', '-fno-strict-aliasing'],
      },
    },

    # XXX not everything needs to be exported in direct_dependent_settings
    'direct_dependent_settings': {
      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
        'include',
        'src',
        'src/js',
        'src/js/assembler',
        'src/js/jit',
        'src/mozilla/double-conversion',
      ],

      'defines': [
        # Platform dependent
        'HAVE_VA_LIST_AS_ARRAY=1',
        '__STDC_LIMIT_MACROS=1',

        # Configuration and cool features
        'ENABLE_YARR_JIT=1',
        'JS_DEFAULT_JITREPORT_GRANULARITY=3',
        'JS_ION=1',
        'JS_THREADSAFE=1',
        'JSGC_GENERATIONAL=1',
        'JSGC_INCREMENTAL=1',
        'JSGC_USE_EXACT_ROOTING=1',

        # Library output
        'STATIC_EXPORTABLE_JS_API=1',  # Re-exportable static library
        'IMPL_MFBT=1',                 # Statically link and re-export mfbt
      ],

      # XXX I have frankly no idea what JS_NUNBOX32 and JS_PUNBOX64 *really* do
      'conditions': [
        ['target_arch == "x64"', {
          'defines': [
            'JS_BITS_PER_WORD_LOG2=6',
            'JS_BYTES_PER_WORD=8',
            'JS_PUNBOX64=1',
            'JS_CPU_X64=1',
          ],
        }],
        ['target_arch == "ia32"', {
          'defines': [
            'JS_BITS_PER_WORD_LOG2=5',
            'JS_BYTES_PER_WORD=4',
            'JS_NUNBOX32=1',
            'JS_CPU_X86=1',
          ],
        }],
        ['target_arch == "arm"', {
          'defines': [
            'JS_BITS_PER_WORD_LOG2=5',
            'JS_BYTES_PER_WORD=4',
            'JS_NUNBOX32=1',
            'JS_CPU_ARM=1',
          ],
        }],
        ['OS == "linux"', {
          'defines': [
            'JS_HAVE_ENDIAN_H=1',
            'XP_UNIX=1',
          ],
          'cflags': [
              '-pthread',
              '-std=c++0x',
              '-Wno-invalid-offsetof'
          ],
          'libraries': [
            '-pthread'
          ],
        }],
        ['OS == "mac"', {
          'defines': [
            'JS_HAVE_MACHINE_ENDIAN_H=1',
            'XP_MACOSX=1',
            'XP_UNIX=1',
            'DARWIN=1',
          ],
          'conditions': [
            ['target_arch == "x64"', {
              'xcode_settings': {'ARCHS': ['x86_64']},
            }],
            ['target_arch == "ia32"', {
              'xcode_settings': {'ARCHS': ['i386']},
            }],
          ],
          'xcode_settings': {
            'CLANG_CXX_LANGUAGE_STANDARD': 'c++0x',
            'CLANG_CXX_LIBRARY': 'libc++',
            'GCC_WARN_ABOUT_INVALID_OFFSETOF_MACRO' : 'NO',
          },
        }],
        ['OS == "win"', {
          'defines': [
            'WIN32=1',
            'XP_WIN=1',
            'WIN32_LEAN_AND_MEAN=1',
            'WTF_COMPILER_MSVC=1'
          ],
          'conditions': [
            ['target_arch == "ia32"', {
              'defines': [
                '_X86_=1'
              ],
            }],
            ['target_arch == "x64"', {
              'defines': [
                '_AMD64_=1',
                '_M_AMD64_=1'
              ],
            }],
          ],
          'msvs_cygwin_shell': 0, # don't use bash
          'msvs_disabled_warnings': [
            '4291', # warning C4291: 'declaration' : no matching operator
                    # delete found; memory will not be freed if initialization
                    # throws an exception
            '4305', # warning C4305: 'identifier' : truncation from 'type1' to
                    # 'type2'
            '4351', # warning C4351: new behavior: elements of array 'array'
                    # will be default initialized
            '4355', # warning C4355: 'this' : used in base member initializer
                    # list
            '4624', # warning C4624: 'derived class' : destructor could not be
                    # generated because a base class destructor is inaccessible
            '4661', # warning C4661: 'identifier' : no suitable definition
                    # provided for explicit template instantiation request
            '4804', # warning C4804: 'operation' : unsafe use of type 'bool'
                    # in operation
            '4805', # warning C4805: 'operation' : unsafe mix of type 'bool'
                    # and type 'type' in operation
          ],
          'msvs_settings': {
            'VCCLCompilerTool': {
              'AdditionalOptions': [
                '/MP', # compile across multiple CPUs
              ],
            },
          }
        }],
      ],
    },
  },

  'targets': [
    {
      'target_name': 'shell',
      'type': 'executable',
      'dependencies': ['smjs'],
      'include_dirs': [
        'src/js/shell',
        'src/js/perf',
      ],
      'conditions': [
        ['OS == "win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ForcedIncludeFiles': [ 'mozilla/Char16.h' ],
            },
          },
        }],
      ],
      'sources': [
        'src/js/shell/js.cpp',
        'src/js/shell/jsheaptools.cpp',
        'src/js/shell/jsoptparse.cpp',
      ],
    },

    {
      'target_name': 'jsapi-tests',
      'type': 'executable',
      'dependencies': ['smjs'],
      'conditions': [
        ['OS == "win"', {
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ForcedIncludeFiles': [ 'mozilla/Char16.h' ],
            },
          },
        }],
      ],
      'sources': [
        'src/js/jsapi-tests/selfTest.cpp',
        'src/js/jsapi-tests/testAddPropertyPropcache.cpp',
        'src/js/jsapi-tests/testArgumentsObject.cpp',
        'src/js/jsapi-tests/testArrayBuffer.cpp',
        'src/js/jsapi-tests/testBindCallable.cpp',
        'src/js/jsapi-tests/testBug604087.cpp',
        'src/js/jsapi-tests/testCallNonGenericMethodOnProxy.cpp',
        'src/js/jsapi-tests/testChromeBuffer.cpp',
        'src/js/jsapi-tests/testClassGetter.cpp',
        'src/js/jsapi-tests/testCloneScript.cpp',
        'src/js/jsapi-tests/testConservativeGC.cpp',
        'src/js/jsapi-tests/testContexts.cpp',
        'src/js/jsapi-tests/testCustomIterator.cpp',
        'src/js/jsapi-tests/testDebugger.cpp',
        'src/js/jsapi-tests/testDeepFreeze.cpp',
        'src/js/jsapi-tests/testDefineGetterSetterNonEnumerable.cpp',
        'src/js/jsapi-tests/testDefineProperty.cpp',
        'src/js/jsapi-tests/testEnclosingFunction.cpp',
        'src/js/jsapi-tests/testErrorCopying.cpp',
        'src/js/jsapi-tests/testException.cpp',
        'src/js/jsapi-tests/testExternalStrings.cpp',
        'src/js/jsapi-tests/testFindSCCs.cpp',
        'src/js/jsapi-tests/testFuncCallback.cpp',
        'src/js/jsapi-tests/testFunctionProperties.cpp',
        'src/js/jsapi-tests/testGCExactRooting.cpp',
        'src/js/jsapi-tests/testGCFinalizeCallback.cpp',
        'src/js/jsapi-tests/testGCOutOfMemory.cpp',
        'src/js/jsapi-tests/testGCStoreBufferRemoval.cpp',
        'src/js/jsapi-tests/testHashTable.cpp',
        'src/js/jsapi-tests/testHashTableInit.cpp',
        'src/js/jsapi-tests/testIndexToString.cpp',
        'src/js/jsapi-tests/testIntern.cpp',
        'src/js/jsapi-tests/testIntString.cpp',
        'src/js/jsapi-tests/testIntTypesABI.cpp',
        'src/js/jsapi-tests/testJSEvaluateScript.cpp',
        'src/js/jsapi-tests/testLookup.cpp',
        'src/js/jsapi-tests/testLooselyEqual.cpp',
        'src/js/jsapi-tests/testNewObject.cpp',
        'src/js/jsapi-tests/testNullRoot.cpp',
        'src/js/jsapi-tests/testObjectEmulatingUndefined.cpp',
        'src/js/jsapi-tests/testOOM.cpp',
        'src/js/jsapi-tests/testOps.cpp',
        'src/js/jsapi-tests/testOriginPrincipals.cpp',
        'src/js/jsapi-tests/testParseJSON.cpp',
        'src/js/jsapi-tests/testPersistentRooted.cpp',
        'src/js/jsapi-tests/testProfileStrings.cpp',
        'src/js/jsapi-tests/testPropCache.cpp',
        'src/js/jsapi-tests/testRegExp.cpp',
        'src/js/jsapi-tests/testResolveRecursion.cpp',
        'src/js/jsapi-tests/tests.cpp',
        'src/js/jsapi-tests/testSameValue.cpp',
        'src/js/jsapi-tests/testScriptInfo.cpp',
        'src/js/jsapi-tests/testScriptObject.cpp',
        'src/js/jsapi-tests/testSetProperty.cpp',
        'src/js/jsapi-tests/testSourcePolicy.cpp',
        'src/js/jsapi-tests/testStringBuffer.cpp',
        'src/js/jsapi-tests/testStructuredClone.cpp',
        'src/js/jsapi-tests/testToIntWidth.cpp',
        'src/js/jsapi-tests/testTrap.cpp',
        'src/js/jsapi-tests/testTypedArrays.cpp',
        'src/js/jsapi-tests/testUTF8.cpp',
        'src/js/jsapi-tests/testXDR.cpp',
      ],
    },

    {
      'target_name': 'jskwgen',
      'type': 'executable',
      'sources': ['src//js/jskwgen.cpp'],
    },

    {
      'target_name': 'jsoplengen',
      'type': 'executable',
      'sources': ['src/js/jsoplengen.cpp'],
    },

    {
      'target_name': 'smjs',
      'type': 'static_library',

      'dependencies': [
        'jskwgen',
        'jsoplengen',
      ],

      'actions': [
        {
          'action_name': 'jskwgen',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)jskwgen<(EXECUTABLE_SUFFIX)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jsautokw.h',
          ],
          'action': [
            '<@(_inputs)',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'jsoplengen',
          'inputs': [
            '<(PRODUCT_DIR)/<(EXECUTABLE_PREFIX)jsoplengen<(EXECUTABLE_SUFFIX)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/jsautooplen.h',
          ],
          'action': [
            '<@(_inputs)',
            '<@(_outputs)',
          ],
        },
        {
          'action_name': 'selfhosted',
          'variables': {
            'embedjs': 'src/js/builtin/embedjs.py',
            'js_msg': 'src/js/js.msg',
            'selfhosted_defines': [
              # Todo: make sure this is filled out with defines.
            ],
            'selfhosted_srcs': [
              'src/js/builtin/Utilities.js',
              'src/js/builtin/Array.js',
              'src/js/builtin/Date.js',
              'src/js/builtin/Intl.js',
              'src/js/builtin/IntlData.js',
              'src/js/builtin/Iterator.js',
              'src/js/builtin/Map.js',
              'src/js/builtin/Number.js',
              'src/js/builtin/String.js',
              'src/js/builtin/Set.js',
              'src/js/builtin/TypedObject.js',
            ],
          },
          'conditions': [
            ['OS == "linux"', {
              'variables': { 'cpp': 'cpp' }
            }],
            ['OS == "mac"', {
              'variables': { 'cpp': '"gcc -xc -E"' }
            }],
            ['OS == "win"', {
              'variables': { 'cpp': '"cl -EP"' }
            }],
          ],
          'inputs': [
            '<@(embedjs)',
            '<@(js_msg)',
            '<@(selfhosted_srcs)',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/selfhosted.out.h',
          ],
          'action': [
            'python',
            '<@(embedjs)',
            '<@(selfhosted_defines)',
            '-p',
            '<@(cpp)',
            '-m',
            '<@(js_msg)',
            '-o',
            '<@(_outputs)',
            '<@(selfhosted_srcs)'
          ]
        },
      ],

      'include_dirs': [
        '<(SHARED_INTERMEDIATE_DIR)',
        'include',
        'src',
        'src/js',
        'src/js/assembler',
        'src/js/jit',
        'src/mozilla/double-conversion',
      ],

      'defines': [
      ],

      'conditions': [
        ['OS == "win"', {
          'defines': [
            '_CRT_RAND_S=1',
            'HAVE_SNPRINTF=1',
            'HW_THREADS=1',
            'STDC_HEADERS=1',
            'WIN32=1',
            'WIN32_LEAN_AND_MEAN=1',
            'XP_WIN32=1',
            'XP_WIN=1',
            '_WINDOWS=1',
          ],
          'link_settings': {
            'libraries': [ '-lwinmm.lib', '-lpsapi.lib' ],
          },
          'msvs_settings': {
            'VCCLCompilerTool': {
              'ForcedIncludeFiles': [ 'mozilla/Char16.h' ],
            },
          },
          'sources': [
            'src/js/assembler/jit/ExecutableAllocatorWin.cpp',
            'src/js/yarr/OSAllocatorWin.cpp',
            # JS_THREADSAFE
            'src/js/threading/windows/ConditionVariable.cpp',
            'src/js/threading/windows/Mutex.cpp',
            'src/js/threading/windows/Thread.cpp',
          ],
        }, {
          'sources': [
            'src/js/assembler/jit/ExecutableAllocatorPosix.cpp',
            'src/js/yarr/OSAllocatorPosix.cpp',
            # JS_THREADSAFE
            'src/js/threading/posix/ConditionVariable.cpp',
            'src/js/threading/posix/Mutex.cpp',
            'src/js/threading/posix/Thread.cpp',
          ],
        }],

        ['OS == "linux"', {
          'sources': ['src/js/perf/pm_linux.cpp'],
        }, {
          'sources': ['src/js/perf/pm_stub.cpp'],
        }],

        ['target_arch == "ia32" or target_arch == "x64"', {
          'sources': [
            # ENABLE_ION or ENABLE_YARR_JIT
            'src/js/assembler/assembler/MacroAssemblerX86Common.cpp',
            # ENABLE_ION
            'src/js/jit/shared/Assembler-x86-shared.cpp',
            'src/js/jit/shared/BaselineCompiler-x86-shared.cpp',
            'src/js/jit/shared/BaselineIC-x86-shared.cpp',
            'src/js/jit/shared/CodeGenerator-x86-shared.cpp',
            'src/js/jit/shared/Lowering-x86-shared.cpp',
            'src/js/jit/shared/MoveEmitter-x86-shared.cpp',
          ],
        }],

        ['target_arch == "ia32"', {
          'sources': [
            # ENABLE_ION
            'src/js/jit/x86/Assembler-x86.cpp',
            'src/js/jit/x86/Bailouts-x86.cpp',
            'src/js/jit/x86/BaselineCompiler-x86.cpp',
            'src/js/jit/x86/BaselineIC-x86.cpp',
            'src/js/jit/x86/CodeGenerator-x86.cpp',
            'src/js/jit/x86/Lowering-x86.cpp',
            'src/js/jit/x86/MacroAssembler-x86.cpp',
            'src/js/jit/x86/Trampoline-x86.cpp',
          ],
        }],

        ['target_arch == "x64"', {
          'sources': [
            # ENABLE_ION
            'src/js/jit/x64/Assembler-x64.cpp',
            'src/js/jit/x64/Bailouts-x64.cpp',
            'src/js/jit/x64/BaselineCompiler-x64.cpp',
            'src/js/jit/x64/BaselineIC-x64.cpp',
            'src/js/jit/x64/CodeGenerator-x64.cpp',
            'src/js/jit/x64/Lowering-x64.cpp',
            'src/js/jit/x64/MacroAssembler-x64.cpp',
            'src/js/jit/x64/Trampoline-x64.cpp',
          ],
        }],

        ['target_arch == "arm"', {
          'sources': [
            # ENABLE_ION or ENABLE_YARR_JIT
            'src/js/assembler/assembler/ARMAssembler.cpp',
            'src/js/assembler/assembler/MacroAssemblerARM.cpp',
            # ENABLE_ION
            'src/js/jit/arm/Architecture-arm.cpp',
            'src/js/jit/arm/Assembler-arm.cpp',
            'src/js/jit/arm/Bailouts-arm.cpp',
            'src/js/jit/arm/BaselineCompiler-arm.cpp',
            'src/js/jit/arm/BaselineIC-arm.cpp',
            'src/js/jit/arm/CodeGenerator-arm.cpp',
            'src/js/jit/arm/IonFrames-arm.cpp',
            'src/js/jit/arm/Lowering-arm.cpp',
            'src/js/jit/arm/MacroAssembler-arm.cpp',
            'src/js/jit/arm/MoveEmitter-arm.cpp',
            'src/js/jit/arm/Trampoline-arm.cpp',
          ],
        }]
      ],

      'sources': [
        'src/js/assembler/jit/ExecutableAllocator.cpp',
        'src/js/builtin/Eval.cpp',
        'src/js/builtin/Intl.cpp',
        'src/js/builtin/MapObject.cpp',
        'src/js/builtin/Object.cpp',
        'src/js/builtin/Profilers.cpp',
        'src/js/builtin/RegExp.cpp',
        'src/js/builtin/SIMD.cpp',
        'src/js/builtin/TestingFunctions.cpp',
        'src/js/builtin/TypeRepresentation.cpp',
        'src/js/builtin/TypedObject.cpp',
        'src/js/devtools/sharkctl.cpp',
        'src/js/ds/LifoAlloc.cpp',
        'src/js/frontend/BytecodeCompiler.cpp',
        'src/js/frontend/BytecodeEmitter.cpp',
        'src/js/frontend/FoldConstants.cpp',
        'src/js/frontend/NameFunctions.cpp',
        'src/js/frontend/ParseMaps.cpp',
        'src/js/frontend/ParseNode.cpp',
        'src/js/frontend/Parser.cpp',
        'src/js/frontend/TokenStream.cpp',
        'src/js/gc/Barrier.cpp',
        'src/js/gc/Iteration.cpp',
        'src/js/gc/Marking.cpp',
        'src/js/gc/Memory.cpp',
        'src/js/gc/Nursery.cpp',
        'src/js/gc/RootMarking.cpp',
        'src/js/gc/Statistics.cpp',
        'src/js/gc/StoreBuffer.cpp',
        'src/js/gc/Tracer.cpp',
        'src/js/gc/Verifier.cpp',
        'src/js/gc/Zone.cpp',
        'src/js/jsalloc.cpp',
        'src/js/jsanalyze.cpp',
        'src/js/jsapi.cpp',
        'src/js/jsarray.cpp',
        'src/js/jsatom.cpp',
        'src/js/jsbool.cpp',
        'src/js/jscntxt.cpp',
        'src/js/jscompartment.cpp',
        'src/js/jscrashreport.cpp',
        'src/js/jsdate.cpp',
        'src/js/jsdtoa.cpp',
        'src/js/jsexn.cpp',
        'src/js/jsfriendapi.cpp',
        'src/js/jsfun.cpp',
        'src/js/jsgc.cpp',
        'src/js/jsinfer.cpp',
        'src/js/jsiter.cpp',
        'src/js/jsmath.cpp',
        'src/js/jsnativestack.cpp',
        'src/js/jsnum.cpp',
        'src/js/jsobj.cpp',
        'src/js/json.cpp',
        'src/js/jsonparser.cpp',
        'src/js/jsopcode.cpp',
        'src/js/jsprf.cpp',
        'src/js/jspropertytree.cpp',
        'src/js/jsproxy.cpp',
        'src/js/jsreflect.cpp',
        'src/js/jsscript.cpp',
        'src/js/jsstr.cpp',
        'src/js/jsutil.cpp',
        'src/js/jswatchpoint.cpp',
        'src/js/jsweakmap.cpp',
        'src/js/jsworkers.cpp',
        'src/js/jswrapper.cpp',
        'src/js/perf/jsperf.cpp',
        'src/js/prmjtime.cpp',
        'src/js/vm/ArgumentsObject.cpp',
        'src/js/vm/CallNonGenericMethod.cpp',
        'src/js/vm/CharacterEncoding.cpp',
        'src/js/vm/DateTime.cpp',
        'src/js/vm/Debugger.cpp',
        'src/js/vm/ErrorObject.cpp',
        'src/js/vm/ForkJoin.cpp',
        'src/js/vm/GlobalObject.cpp',
        'src/js/vm/Id.cpp',
        'src/js/vm/Interpreter.cpp',
        'src/js/vm/MemoryMetrics.cpp',
        'src/js/vm/Monitor.cpp',
        'src/js/vm/ObjectImpl.cpp',
        'src/js/vm/OldDebugAPI.cpp',
        'src/js/vm/Probes.cpp',
        'src/js/vm/PropertyKey.cpp',
        'src/js/vm/ProxyObject.cpp',
        'src/js/vm/RegExpObject.cpp',
        'src/js/vm/RegExpStatics.cpp',
        'src/js/vm/Runtime.cpp',
        'src/js/vm/SPSProfiler.cpp',
        'src/js/vm/ScopeObject.cpp',
        'src/js/vm/SelfHosting.cpp',
        'src/js/vm/Shape.cpp',
        'src/js/vm/Stack.cpp',
        'src/js/vm/String.cpp',
        'src/js/vm/StringBuffer.cpp',
        'src/js/vm/StructuredClone.cpp',
        'src/js/vm/ThreadPool.cpp',
        'src/js/vm/TypedArrayObject.cpp',
        'src/js/vm/Unicode.cpp',
        'src/js/vm/Value.cpp',
        'src/js/vm/Xdr.cpp',
        'src/js/yarr/PageBlock.cpp',
        'src/js/yarr/YarrCanonicalizeUCS2.cpp',
        'src/js/yarr/YarrInterpreter.cpp',
        'src/js/yarr/YarrPattern.cpp',
        'src/js/yarr/YarrSyntaxChecker.cpp',

        # JS_THREADSAFE
        'src/js/threading/Once.cpp',

        # ENABLE_ION
        'src/js/jit/AliasAnalysis.cpp',
        'src/js/jit/AsmJS.cpp',
        'src/js/jit/AsmJSLink.cpp',
        'src/js/jit/AsmJSModule.cpp',
        'src/js/jit/AsmJSSignalHandlers.cpp',
        'src/js/jit/BacktrackingAllocator.cpp',
        'src/js/jit/Bailouts.cpp',
        'src/js/jit/BaselineBailouts.cpp',
        'src/js/jit/BaselineCompiler.cpp',
        'src/js/jit/BaselineFrame.cpp',
        'src/js/jit/BaselineFrameInfo.cpp',
        'src/js/jit/BaselineIC.cpp',
        'src/js/jit/BaselineInspector.cpp',
        'src/js/jit/BaselineJIT.cpp',
        'src/js/jit/BitSet.cpp',
        'src/js/jit/BytecodeAnalysis.cpp',
        'src/js/jit/C1Spewer.cpp',
        'src/js/jit/CodeGenerator.cpp',
        'src/js/jit/CompileWrappers.cpp',
        'src/js/jit/EdgeCaseAnalysis.cpp',
        'src/js/jit/EffectiveAddressAnalysis.cpp',
        'src/js/jit/Ion.cpp',
        'src/js/jit/IonAnalysis.cpp',
        'src/js/jit/IonBuilder.cpp',
        'src/js/jit/IonCaches.cpp',
        'src/js/jit/IonFrames.cpp',
        'src/js/jit/IonMacroAssembler.cpp',
        'src/js/jit/IonOptimizationLevels.cpp',
        'src/js/jit/IonSpewer.cpp',
        'src/js/jit/JitOptions.cpp',
        'src/js/jit/JSONSpewer.cpp',
        'src/js/jit/LICM.cpp',
        'src/js/jit/LIR.cpp',
        'src/js/jit/LinearScan.cpp',
        'src/js/jit/LiveRangeAllocator.cpp',
        'src/js/jit/Lowering.cpp',
        'src/js/jit/MCallOptimize.cpp',
        'src/js/jit/MIR.cpp',
        'src/js/jit/MIRGraph.cpp',
        'src/js/jit/MoveResolver.cpp',
        'src/js/jit/ParallelFunctions.cpp',
        'src/js/jit/ParallelSafetyAnalysis.cpp',
        'src/js/jit/PerfSpewer.cpp',
        'src/js/jit/RangeAnalysis.cpp',
        'src/js/jit/RegisterAllocator.cpp',
        'src/js/jit/Safepoints.cpp',
        'src/js/jit/Snapshots.cpp',
        'src/js/jit/StupidAllocator.cpp',
        'src/js/jit/TypePolicy.cpp',
        'src/js/jit/TypeRepresentationSet.cpp',
        'src/js/jit/UnreachableCodeElimination.cpp',
        'src/js/jit/VMFunctions.cpp',
        'src/js/jit/ValueNumbering.cpp',
        'src/js/jit/shared/BaselineCompiler-shared.cpp',
        'src/js/jit/shared/CodeGenerator-shared.cpp',
        'src/js/jit/shared/Lowering-shared.cpp',

        # ENABLE_YARR_JIT
        'src/js/yarr/YarrJIT.cpp',

        # mfbt
        'src/mozilla/Compression.cpp',
        'src/mozilla/FloatingPoint.cpp',
        'src/mozilla/HashFunctions.cpp',
        'src/mozilla/Poison.cpp',
        'src/mozilla/SHA1.cpp',
        'src/mozilla/decimal/Decimal.cpp',
        'src/mozilla/double-conversion/bignum-dtoa.cc',
        'src/mozilla/double-conversion/bignum.cc',
        'src/mozilla/double-conversion/cached-powers.cc',
        'src/mozilla/double-conversion/diy-fp.cc',
        'src/mozilla/double-conversion/double-conversion.cc',
        'src/mozilla/double-conversion/fast-dtoa.cc',
        'src/mozilla/double-conversion/fixed-dtoa.cc',
        'src/mozilla/double-conversion/strtod.cc',
      ],
    },
  ],
}
