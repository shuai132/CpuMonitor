
option(FLATBUFFERS_CODE_COVERAGE "Enable the code coverage build option."
        OFF)
option(FLATBUFFERS_BUILD_TESTS "Enable the build of tests and samples."
        OFF)
option(FLATBUFFERS_INSTALL "Enable the installation of targets."
        OFF)
option(FLATBUFFERS_BUILD_FLATLIB "Enable the build of the flatbuffers library"
        ON)
option(FLATBUFFERS_BUILD_FLATC "Enable the build of the flatbuffers compiler"
        ON)
option(FLATBUFFERS_STATIC_FLATC "Build flatbuffers compiler with -static flag"
        OFF)
option(FLATBUFFERS_BUILD_FLATHASH "Enable the build of flathash"
        OFF)
option(FLATBUFFERS_BUILD_BENCHMARKS "Enable the build of flatbenchmark."
        OFF)
option(FLATBUFFERS_BUILD_GRPCTEST "Enable the build of grpctest"
        OFF)
option(FLATBUFFERS_BUILD_SHAREDLIB
        "Enable the build of the flatbuffers shared library"
        OFF)
option(FLATBUFFERS_LIBCXX_WITH_CLANG "Force libc++ when using Clang"
        ON)
# NOTE: Sanitizer check only works on Linux & OSX (gcc & llvm).
option(FLATBUFFERS_CODE_SANITIZE
        "Add '-fsanitize' flags to 'flattests' and 'flatc' targets."
        OFF)
option(FLATBUFFERS_PACKAGE_REDHAT
        "Build an rpm using the 'package' target."
        OFF)
option(FLATBUFFERS_PACKAGE_DEBIAN
        "Build an deb using the 'package' target."
        OFF)
option(FLATBUFFERS_BUILD_CPP17
        "Enable the build of c++17 test target. \"
       Requirements: Clang6, GCC7, MSVC2017 (_MSC_VER >= 1914)  or higher."
        OFF)
option(FLATBUFFERS_BUILD_LEGACY
        "Run C++ code generator with '--cpp-std c++0x' switch."
        OFF)
option(FLATBUFFERS_ENABLE_PCH
        "Enable precompile headers support for 'flatbuffers' and 'flatc'. \"
        Only work if CMake supports 'target_precompile_headers'. \"
        This can speed up compilation time."
        OFF)
option(FLATBUFFERS_SKIP_MONSTER_EXTRA
        "Skip generating monster_extra.fbs that contains non-supported numerical\"
      types." OFF)
option(FLATBUFFERS_STRICT_MODE
        "Build flatbuffers with all warnings as errors (-Werror or /WX)."
        OFF)
