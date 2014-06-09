# Reads the output from svm-train and extracts the final accuracy
# The last line will be something like 

# ... (libSVM status)
# Cross Validation Accuracy = 49.9% 

# so we want to extract the 49.9 to a file objective.txt which will be
# read by the wrapper and sent to the 'master' GA node.

import os.path
import re

def ProcessOutput():
    if not os.path.isfile('std.out'):
        print "Cannot find file std.out!"
        return -1
        
    f = open('std.out', 'rb')
    for line in f:
        last_line = line
    
    # Extract the number from the last element of last line
    # The wrapper will pipe the output to obj.out so we don't need to write this to a file
    print re.sub("[^0-9.-]", "", last_line.split(' ')[-1])
  
if __name__ == "__main__":
    ProcessOutput()
