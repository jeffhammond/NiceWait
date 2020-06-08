NiceWait: Stop MPI from spinning in blocking functions

# Options

`NICEWAIT_TIME` - number of microseconds to pause between test operations.

# Implementation

In short, we replace wait functions with a loop over test mediated by a pause call.

All blocking functions are replaced by their nonblocking variants followed by wait.

# Known Issues

This code is completely untested.  Your mileage may vary.

I have not thought about it much, but it's possible that this library changes
the behavior of multithreaded programs.  As such, please stick with
`MPI_THREAD_SINGLE` and `MPI_THREAD_FUNNELED` for now.

