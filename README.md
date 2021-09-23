# SEAL-Embedded

SEAL-Embedded is an open-source ([MIT licensed](LICENSE)) homomorphic encryption toolset for embedded devices developed by the Cryptography and Privacy Reserach Group at Microsoft.
It implements the designs of CKKS-style encoding and encryption with small code and memory footprint that is published in [this peer-reviewed paper](https://tches.iacr.org/index.php/TCHES/article/view/8991).
SEAL-Embedded is written in C and C++ and primarily designed for building high-level applications on [Azure Sphere](https://azure.microsoft.com/en-us/services/azure-sphere/)'s ARM A7 processor.
To enable wider experiments by developers and researchers, SEAL-Embedded includes configurations that target ARM M4 as well.

## Acknowledgments

The majority of SEAL-Embedded was developed by [Deepika Natarajan](https://github.com/dnat112) from University of Michigan during an internship at Microsoft, despite not presented in the Git history.

## Introduction

Homomorphic encryption allows computation on encryption data without decryption.
To understand what homomorphic encryption and Microsoft SEAL are, we refer readers to [the Introduction section of Microsoft SEAL](https://github.com/microsoft/SEAL#introduction).
Typically, a client generates a secret key and public keys; data are encrypted with the secret key or public keys; encrypted data are sent to a untrusted server for computation; encrypted results are returned to the client who then decrypts the results.
SEAL-Embedded enables embedded devices to encrypt data; it *does not* perform key generation, computation on encrypted data, or decryption.

SEAL-Embedded consists of mainly two components: a device library ([`device/lib`](device/lib)) to build an application that encrypts data and an adapter application ([`adapter`](adapter)) for compability with Microsoft SEAL.
The simplest workflow is described as follows:
1. run adapter to generate a secret key and public keys;
1. build an application with device library and create an image of the application together with public keys for a target device;
1. run adapter on an remote server to receive and serialize encrypted data to Microsoft-SEAL-compatible format;
1. build an application with Microsoft SEAL to compute on encrypted data;
1. decrypt encrypted results with Microsoft SEAL on a safe device that has the secret key.

## Warning

This library is research code and is not yet intended for production use.
Use at your own risk.
Holding secret key on device's storage for asymmetric encryption is extremely dangerous, regardless of whether the storage type is persistent or not.
Use public key (symmetric encryption), or consult a security expert.
See more information related to [SECURITY](SECURITY.md).

## Building SEAL-Embedded

On all platforms SEAL-Embedded is built with CMake.
We recommend using out-of-source build although in-source build works.
Below we give instructions for how to configure and build SEAL-Embedded components.

### SEAL-Embedded Adapter

The adapter ([`adapter`](adapter)) application automatically downloads and builds Microsoft SEAL as a dependency.
System requirements are listed in [SEAL/README.md](https://github.com/microsoft/SEAL/blob/main/README.md#requirements).

On Unix-like systems:
```powershell
cd adapter
cmake -S . -B build
cmake --build build
./build/bin/se_adapter # run adapter
```

On Windows, following [this guide](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio?view=msvc-160), use Visual Studio 2019 to open [`adapter/CMakeLists.txt`](adapter/CMakeLists.txt) as a CMake project.

### SEAL-Embedded Device Library

The device library comes with optional executables: unit tests ([`device/test`](device/test)) and benchmark ([`device/bench`](device/bench)).
The library itself, unit tests, and benchmark can be built on a native system (Linux or macOS) with same development environment listed in SEAL-Embedded adapter.
```powershell
cd adapter
cmake -S . -B build -DSE_BUILD_LOCAL=ON
cmake --build build
```
This by default builds a library and unit tests.
The following options are configurable for custom builds.

| CMake option | Values | Information  |
| ------------ | ------ | ------------ |
| SE_BUILD_LOCAL | **ON** / OFF | Set to `OFF` to target an embedded device. |
| SE_BUILD_M4 | ON / **OFF** | Set to `ON` to target ARM M4, `OFF` to target ARM A7 on Azure Sphere. |
| SE_M4_IS_SPHERE | ON / **OFF** | Set to `ON` to target the real-time core on Azure Sphere, `OFF` to target other ARM M4 processors. |
| CMAKE_BUILD_TYPE | **Debug**</br>Release</br>MinSizeRel</br>RelWithDebInfo | Set to `Release` for the best run-time performance and the smallest code size. |
| SE_BUILD_TYPE | **Tests**</br>Bench</br>Lib | Set to `Bench` to build instead the benchmark executable, to `Lib` to build a library only. |

To target an embedded device, the specific SDK for the target device is usually required.
Either the unit tests or the benchmark can be built and imaged on the device.
Using Azure Sphere as an example, please follow [this guide](https://docs.microsoft.com/en-us/azure-sphere/install/qs-blink-application?tabs=windows%2Ccliv2beta&pivots=visual-studio).
**Note: SEAL-Embedded Adapter must be built and executed first to generate required key and precomputation files.**
Ultimately, the users can develop their own application following [this guide](https://docs.microsoft.com/en-us/azure-sphere/install/qs-real-time-application?tabs=windows%2Ccliv2beta&pivots=visual-studio) and statically link to SEAL-Embedded device library.

For targeting ARM M4 on Azure Sphere or other development boards, please follow related documents and tutorials.
For the convinience of verifying ARM M4 compatibility, we also provide [`device/lib/sdk_config.h`](device/lib/sdk_config.h) for an easier deployment on Nordic Semiconductor nRF5* series.

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
