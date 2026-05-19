# Tiling-algorithm-int-float
This project intends to develop a smart way of tiling given certain variable type, amount of outputs and range.

User must have a c environment equipped in their editor to access the code

Current version of code only make samples across the range evenly
-ints functions output values in even stepsize.
-float functions only output values varying exponential part in even distance.

Int inputs:
- signed : bool
- precision_bits : uint32_t
- num_samples : size_t

Float inputs:
- precision_bits : uint32_t
- num_samples : size_t

compile and run test:
-make test

delete build directory:
-make clean