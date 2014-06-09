GridGA
======

Genetic algorithm designed to wrap an existing executable or script, and run on an HTCondor (previously named Condor) compute cluster. HTCondor can be downloaded from http://research.cs.wisc.edu/htcondor.

GridGA is designed to 'wrap' around an existing executable or script. The requirement is that the executable is able to receive commandline arguments. Work is in progress to generate a config file if your executable requires this rather than a command line.

You specify the parameters in GridGA's config file, and these are then passed to your executable which is run in parallel with different combinations of parameters. Your executable is the 'objective function' which the GA uses to optimise the parameters.

GridGA is useful for the case where your objective function is computationally intensive (e.g. a backtest in a trading strategy) so the optimisation process benefits from running each test of the parameters in a seperate process in parallel running on a distributed cluster.

HTCondor can scale from a single machine to cluster of a virtually unlimited number of machines. You can also run more than one GridGA process at the same time.

GridGA is the genetic algorithm used in the DeepThought (http://www.deep-thought.co) application providing machine learning to trading systems.

## Requirements
To build from source, you'll need the boost libraries (http://www.boost.org). ZeroMQ libraries are included in the libs dir for Windows. For Linux you'll need to ensure ZeroMQ is installed.

## Example Usage
<TODO>
