# List of Changes

## Version 1.1.0

### New Features

- Added explicit support for other parameter sets (for degree = 1024, 2048, 8192, and 16384, as well as an alternate prime set for degree = 4096)
- Adapter can now just be build once for any of the supported degrees, and degree can be specied as a command line argument
- Added scripts to [`device/scripts`](device/scripts) to automatically test configurations of the library

### Bug Fixes

- Added an extra index map setting to fix memory bug encountered in certain edge-case configurations of the library
- Fixed error with generated header files for special primes > 30 bits
- Fixed error with IFFT header file generation
- Fixed error with NTT tests

### Minor API Changes

- Scale is now automatically set for default parameters
- `set_custom_parms_ckks` now includes a parameter to set the scale

### Other

- Corrected minor typos in code comments
- Added description comments for some functions
- Added `const` and explicit type casting as good practice
- Added `memset` after array creation instances for when `malloc` is not used
- Removed support for degree < 1024, which was just for initial debugging
- Fixed path issue when building for an Azure Sphere A7 target
- Updated adapter [`CMakeLists.txt`](adapter/CMakeLists.txt) to use Microsoft SEAL 3.7.2
- Other minor code changes

## Version 1.0.0

- Initial commit
