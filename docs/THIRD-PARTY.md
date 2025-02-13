# Optional third-party library benchmarks

SYCL-DNN provides a number of benchmarks using third party libraries in order
to compare the performance of our kernels with these well known alternatives.
In order to build these benchmarks, the third party libraries must be built
separately and provided to SYCL-DNN at configure time.

## ARM Compute Library

ARM has written a library of vision and NN operations called the ARM Compute
Library, available [on GitHub](https://github.com/ARM-software/ComputeLibrary).
The ARM Compute Library can run on both ARM and x86 platforms, though only
supports NEON kernels on ARM.

### Getting ARM Compute Library

It can be found [on GitHub](https://github.com/ARM-software/ComputeLibrary),
with both source and prebuilt binaries available.

### Building ARM Compute Library

ARM Compute Library uses the `scons` build system, which should be available
through your system's package manager. In order to cross compile for ARM
architectures you need a compiler that can target that architecture, such as
GCC or Clang. In this case, the compiler triple should be `aarch64-gnu-linux`.
`scons` expects that the cross-compiler will be available in `$PATH`. Either
installing this from your package manager or downloading a standalone cross-
compiler that you add to the path should work. Full instructions for how to
use the Compute Library are [provided by
ARM](https://arm-software.github.io/ComputeLibrary/v18.11/index.xhtml), but
a build sufficient for our purposes can be created with the following command:
```bash
$ scons arch=arm64-v8a examples=no opencl=yes neon=yes -j8
```
If you do this successfully, scons will create a folder called `build` in the
root of the ARM Compute Library folder, which in turn holds the ARM Compute
Library shared objects.

### Using ARM Compute Library with SYCL-DNN

SYCL-DNN's CMake has support for prebuilt ARM Compute Library shared objects.
To build the benchmarks that use the ARM Compute Library, add the following
commands to your CMake command line:
```bash
$ cmake -DSNN_BENCH_ARM_COMPUTE=ON -DARMCompute_DIR=/path/to/acl/dir
```
The ARM Compute Library binaries are all prefixed with `arm_`, to allow users
to filter the list of targets to build and run. The binaries are otherwise
standard Google Benchmark binaries and behave similarly to the SYCL-DNN
variants.

## Intel MKL-DNN

Intel have developed a neural network library based on their "Math Kernel
Library", which provides optimised maths routines.

### Getting MKL-DNN

MKL-DNN is likewise [on GitHub](https://github.com/intel/mkl-dnn). Intel
provides a separate download of the MKL from their website, which can be
found [here](https://software.intel.com/en-us/mkl).

### Building MKL-DNN

Once installed, MKL-DNN can be built with a simple CMake invocation,
though it is recommended to read the MKL-DNN `README.md` to understand
the full range of the options available.

```bash
$ export MKLROOT=/path/to/intel/mkl # needed if in nonstandard location
$ cmake -GNinja -DMKLDNN_BUILD_TESTS=OFF -DMKLDNN_BUILD_EXAMPLES=OFF
  -DMKLDNN_USE_MKL=FULL # or FULL:STATIC for static linking
  -DMKLDNN_LIBRARY_TYPE=SHARED # or STATIC, for a static lib
  -DCMAKE_BUILD_TYPE=Release # for optimisation
  -DCMAKE_BUILD_PREFIX=/path/of/your/choice # creates export CMake file
$ ninja install # Installing generates the CMake export file
```

### Using MKL-DNN with SYCL-DNN

To tell SYCL-DNN's CMake to build the MKL-DNN targets, and where to find
the MKL-DNN libraries, add the following to your CMake command line:
```bash
$ cmake -DSNN_BENCH_MKLDNN=ON -Dmkldnn_DIR=/mkl-dnn-install/lib/cmake/mkldnn
```
This will create new CMake targets, prepended with `mkl_`, corresponding
to the other SYCL-DNN benchmarks.

## CLBlast backend

The library [CLBlast](https://github.com/cnugteren/clblast) can be used as a
different backend to compute the matrix multiplies required in SYCL-DNN. Since
it is a compiled library, the exported library targets file generated by
CLBlast when it is built and installed can be used to tell SYCL-DNN where it is
located.

### Using CLBlast with SYCL-DNN

Download and build CLBlast from GitHub, then when building SYCL-DNN, specify
the location of the CLBlast exports file:

```bash
git clone git@github.com/cnugteren/clblast
mkdir -p clblast/build && cd clblast/build
cmake ../ -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=$PWD/../install
make -j`nproc` install
cd -
git clone git@github.com/codeplaysoftware/sycl-dnn
mkdir -p sycl-dnn/build && cd sycl-dnn/build
cmake -DCLBlast_DIR=$PWD/../../clblast/install/lib/cmake/CLBlast
  -DSNN_TEST_CLBLAST=ON -DSNN_BENCH_CLBLAST=ON
make -j`nproc`
```

After this sequence of steps, you will have a set of tests and benchmarks that
use CLBlast for matrix multiplications (as well as whatever other backends were
enabled in SYCL-DNN when built).
