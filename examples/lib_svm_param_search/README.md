## Parameter search for libSVM

libSVM (http://www.csie.ntu.edu.tw/~cjlin/libsvm/) is a popular Support Vector Machine library. This toy example demonstrates how GridGA can be used to search for optimal parameters for penalty (c) and gamma (g) for a classification problem.

To run this example from the commandline:

    run_ga lib_svm_param_search

where `lib_svm_param_search` is the directory containing the files listed here (i.e change to one level above this directory and pass the dir name to `run_ra`).