Sean Brasier and Erik Lystad

1. Running the command project1Server 5 gives a fail to bind error 98. This means that the port is already binded and in use by another application.

2. Length of log file: 

3. Result of command sha256sum outfile.txt: 

4. There seemed to be a discernable difference between the time taken to run each command. byteAtATimeCmd took much longer, this being the case because it has to do 1000 times as many sends and 1000 times as many receives as kByteAtATime. We could observe the difference by adding a timer function that starts at the beginning of the function and ends at the end of the function, and compare the two times.

5. About 16 hours.
