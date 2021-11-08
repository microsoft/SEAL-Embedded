# SEAL-Embedded

SEAL-Embedded is an open-source ([MIT licensed](LICENSE)) homomorphic encryption toolset for embedded devices developed by the Cryptography and Privacy Reserach Group at Microsoft.
It implements the designs of CKKS-style encoding and encryption with small code and memory footprint that is published in [this peer-reviewed paper](https://tches.iacr.org/index.php/TCHES/article/view/8991).
SEAL-Embedded is written in C and C++ and primarily designed for building high-level applications on [Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/)'s ARM A7 processor.
To enable wider experiments by developers and researchers, SEAL-Embedded includes configurations that target ARM M4 as well.

## Acknowledgments

The majority of SEAL-Embedded was developed by [Deepika Natarajan](https://github.com/dnat112) from University of Michigan during an internship at Microsoft, despite her not being present in the Git history.

## Introduction

Homomorphic encryption allows for computation on encryption data without decryption.
Typically, a client generates a secret key and public keys; data is encrypted with the secret key or public keys; encrypted data is sent to a untrusted server for computation; encrypted results are returned to the client who then decrypts the results.
Interested users of SEAL-Embedded should read [the Introduction section of Microsoft SEAL](https://github.com/microsoft/SEAL#introduction) prior to using SEAL-Embedded to learn more about homomorphic encryption and the Microsoft SEAL library.
SEAL-Embedded enables embedded devices to encrypt data; it *does not* perform key generation, computation on encrypted data, or decryption (but does include an easy-to-use `adapter` interface to perform key generation with Microsoft SEAL).

SEAL-Embedded consists of mainly two components: a device library ([`device/lib`](device/lib)) to build an application that encrypts data, and an adapter application ([`adapter`](adapter)) for compability with Microsoft SEAL.
The simplest workflow is described as follows:
1. Run the adapter to generate a secret key and public keys
1. Build and run an application with the device library by creating an image of the application together with the public key (for asymmetric encryption) or secret key (for symmetric encryption) for a target device
1. Run the adapter on an remote server to receive and serialize encrypted data to Microsoft-SEAL-compatible format
1. Build and run an application with Microsoft SEAL to compute on encrypted data
1. Decrypt the encrypted results with Microsoft SEAL on a safe device that has the secret key

## Warning

This library is research code and is not yet intended for production use.
Use at your own risk.
Storing a secret key on device (i.e., using symmetric encryption) can be extremely dangerous. 
Use public key (asymmetric encryption), or consult a security expert before creating a symmetric key deployment.
See more information related to [SECURITY](SECURITY.md).

## Building SEAL-Embedded

On all platforms, SEAL-Embedded is built with CMake.
We recommend using an out-of-source build, although in-source builds work as well.
Below we give instructions for how to configure and build SEAL-Embedded components.

### SEAL-Embedded Adapter

The adapter ([`adapter`](adapter)) application automatically downloads and builds Microsoft SEAL as a dependency.
System requirements are listed in [SEAL/README.md](https://github.com/microsoft/SEAL/blob/main/README.md#requirements).

On Unix-like systems:
```powershell
cd adapter
cmake -S . -B build
cmake --build build -j
./build/bin/se_adapter # run adapter
```

On Windows, following [this guide](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-160), use Visual Studio 2019 to open [`adapter/CMakeLists.txt`](adapter/CMakeLists.txt) as a CMake project.

### SEAL-Embedded Device Library

The device library comes with optional executables: unit tests ([`device/test`](device/test)) and benchmarks ([`device/bench`](device/bench)).
The library itself, unit tests, and benchmarks can be built on a native system (Linux or macOS) with the same development environment listed in SEAL-Embedded adapter.
```powershell
cd device 
cmake -S . -B build -DSE_BUILD_LOCAL=ON
cmake --build build -j
./build/bin/seal_embedded_tests # run tests locally
```
This by default builds a library and unit tests.

To build the library and run tests for the Azure Sphere A7 target instead:
```powershell
cd device 
cmake -G Ninja -S . -B build -DSE_BUILD_LOCAL=OFF
cmake --build build -j
sh ./scripts/sphere_a7_launch_cl.sh
```
Open another terminal and invoke gdb:
```powershell
sh ./scripts/gdb_launch_sphere_a7.sh
```
The output should now print to the first terminal.

The following options are configurable for custom builds (default settings are in bold).

| CMake option | Values | Information  |
| ------------ | ------ | ------------ |
| SE_BUILD_LOCAL | **ON** / OFF | Set to `OFF` to target an embedded device. |
| SE_BUILD_M4 | ON / **OFF** | Set to `ON` to target ARM M4, `OFF` to target ARM A7 on Azure Sphere. |
| SE_M4_IS_SPHERE | ON / **OFF** | Set to `ON` to target the real-time core on Azure Sphere, `OFF` to target other ARM M4 processors. |
| CMAKE_BUILD_TYPE | **Debug**</br>Release</br>MinSizeRel</br>RelWithDebInfo | Set to `Release` for the best run-time performance and the smallest code size. |
| SE_BUILD_TYPE | **Tests**</br>Bench</br>Lib | Set to `Bench` to build instead the benchmark executable, to `Lib` to build a library only. |

**Note: SEAL-Embedded device code is built in Debug and Local mode by default!**
We recommend that all users of the library first ensure that the included tests and their target application run in `Debug` mode locally, for their desired memory configuration (see: [`user_defines.h`](device/lib/user_defines.h) and the SEAL-Embedded [paper](https://tches.iacr.org/index.php/TCHES/article/view/8991) for more details on memory configuration options).
After verifying that these tests pass, users should change `CMAKE_BUILD_TYPE` to `Release`, uncomment `SE_DISABLE_TESTING_CAPABILITY` in user_defines.h, and set `SE_ASSERT_TYPE` to `0` (`None`) in user_defines.h, for optimal performance.
To run benchmarks, change `SE_BUILD_TYPE` to `Bench` and uncomment `SE_ENABLE_TIMERS` in [`defines.h`](device/lib/defines.h).
Users should verify that everything runs as expected in their local development environment first before targetting an embedded device.

**Note: SEAL-Embedded Adapter must be built and executed first to generate required key and precomputation files.**
The adapter by default generates and writes files to `device/adapter_output_files`. 
If `device/adapter_output_files` does not exist, you should create this folder prior to running the adapter. 
Note that the adapter is not intended to support generation or usage of multiple parameter instances at once (i.e., using the adapter to create files for degree = 1024, then using the adapter to create files for degree = 4096, will cause some of the files generated for degree = 1024 to be overwritten).

To target an embedded device, the specific SDK for the target device is usually required.
Either the unit tests or benchmarks can be built and imaged on the device.
Using the Azure Sphere as an example, please follow [this guide](https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application?tabs=windows%2Ccliv2beta&pivots=visual-studio).
Ultimately, users can develop their own application following [this guide](https://docs.microsoft.com/en-us/azure-sphere/install/qs-real-time-application?tabs=windows%2Ccliv2beta&pivots=visual-studio) and statically link to the SEAL-Embedded device library.

For targeting ARM M4 on the Azure Sphere or other development boards, please follow related documents and tutorials. 
Note that stack size limitations may prevent certain configurations of the library to run on an Azure Sphere M4 target.
For the convinience of verifying ARM M4 compatibility, we also provide [`device/lib/sdk_config.h`](device/lib/sdk_config.h) for an easier deployment on Nordic Semiconductor nRF5* series devices.

## Contributing

The main purpose of open sourcing SEAL-Embedded is to enable developers to build privacy-enhancing applications or services.
We welcome any suggestions or contributions to improve the usebility, efficiency, and quality of SEAL-Embedded.
To keep the development active, the `main` branch will always include the most recent changes, beyond the lastest release tag.
For contributing to Microsoft SEAL, please see [CONTRIBUTING](CONTRIBUTING.md).

## Citing SEAL-Embedded

To cite SEAL-Embedded in academic papers, please use the following BibTeX entries.

```tex
    @Article{sealembedded,
        title = {{SEAL}-Embedded: A Homomorphic Encryption Library for the Internet of Things},
        author = {Deepika Natarajan and Wei Dai},
        journal = {{IACR} Transactions on Cryptographic Hardware and Embedded Systems},
        publisher = {Ruhr-Universit{\"a}t Bochum},
        year = 2021,
        month = jul,
        pages = {756--779},
        valume = 2021,
        number = 3,
        doi = {10.46586/tches.v2021.i3.756-779},
        issn = {2569-2925},
        note = {\url{https://tches.iacr.org/index.php/TCHES/article/view/8991}},
    }
```
