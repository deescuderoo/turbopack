# TurboPack

This is the code for the passively secure protocol presented in the paper "TurboPack: Honest Majority MPC with Constant Online Communication", presented at CCS 2022.

## Requirements

The requirements are 
- `cmake >= 3.15`
- `Catch2` (only necessary if running tests)
- `secure-computation-library` (included)

These are installed as follows:

### CMake

Follow the [official instructions](https://cmake.org/install/). 

### Catch2

Even though this is optional, unit tests are included and can be run, but this requires version 2 of Catch2 (the current version is 3).
This is not needed to run the protocol without the tests.
To install version 2 of Catch2, proceed as follows:
```
$ git clone -b v2.x https://github.com/catchorg/Catch2.git
$ cd Catch2
$ cmake -Bbuild -H. -DBUILD_TESTING=OFF
$ sudo cmake --build build/ --target install
```

### Secure Computation Library

This self-contained library handles communication, finite field types, polynomial evaluation/interpolation, and other primitives required for our protocol.
The library is [open source](https://github.com/anderspkd/secure-computation-library) under the GNU Affero General Public License.
For `TurboPack`, an earlier version of SCL was used, which is included in this repository under `secure-computation-library/`.
This version includes ad-hoc support for packed secret-sharing, which may be included into the main SCL repository in the future for more general use.
The library will be compiled alongside the protocol itself with the instructions below.

## Installing

With all the pre-requisites in place, go to the main directory and run
```
$ cmake . -DCMAKE_BUILD_TYPE=Release -B build
$ cd build
$ make
```

This creates two executables in the main directory: `ours.x` and `dn07.x`, which corresponds to our protocol and DN07, respectively. 

**Note:** Tests can be compiled and run by changing `Release` to `Debug` above.
This creates an executable in the `build` directory called `tests.x` which can be run as `./tests.x` to execute all tests, or you can also run `./tests.x -h` to see more testing options (this comes from Catch2).
The directory with the tests is `test\`.

## Running

Each executable corresponds to one party in either protocol, and they are set to be connected locally through localhost. 
To run our protocol, call
```
$ ./build/ours.x n_parties id size depth
```
Where `n_parties` is the number of parties, `id` is the id of the current party (starting at zero), `size` is the number of multiplication gates, and `depth` is the desired depth (this number must divide the number of multiplications, and the multiplications will be spread evenly across all layers).
Similar instructions hold for `dn07.x`.

There is a script that automates spawning these parties. Run
```
$ ./run.sh n_parties size depth
```
This will run first TurboPack, followed by DN07, and print runtimes for some parts of these protocols.
