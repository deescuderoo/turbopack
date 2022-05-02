# Turbo-Packing

This is the code for the passively secure protocol presented in the paper "Honest Majority MPC with Constant Online Communication".

## Requirements

The requirements are 
- `cmake >= 3.15`
- `lcov`
- `Catch2`
- `secure-computation-library` (included)

These are installed as follows:

### CMake

Follow the [official instructions](https://cmake.org/install/). 

### LCov

Follow the [official instructions](http://ltp.sourceforge.net/coverage/lcov.php) (also available in the standard Ubuntu repository).

### Catch2

We require version 2 of Catch2 (the current version is 3).
To install version 2, proceed as follows:
```
$ git clone -b v2.x https://github.com/catchorg/Catch2.git
$ cd Catch2
$ cmake -Bbuild -H. -DBUILD_TESTING=OFF
$ sudo cmake --build build/ --target install
```

### Secure Computation Library

This self-contained library handles communication, finite field types, polynomial evaluation/interpolation, and other primitives required for our protocol.
To compile it, first enter the `secure-computation-library` directory and then run
```
$ cmake . -DCMAKE_BUILD_TYPE=Release -B build
$ cd build
$ make
```

## Installing

With all the pre-requisites in place, go to the main directory and run
```
$ cmake . -DCMAKE_BUILD_TYPE=Release -B build
$ cd build
$ make
```

This creates two executables in the main directory: `ours.x` and `dn07.x`, which corresponds to our protocol and DN07, respectively. 

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
This will run first our protocol, followed by DN07, and print runtimes for some parts of these protocols.
