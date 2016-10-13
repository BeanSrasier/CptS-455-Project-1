Sean Brasier and Erik Lystad

1. Running the command project1Server 5 gives a fail to bind error 98. This means that the port is already binded and in use by another application.

2. Length of log file: 1000087 bytes. This is the correct value as it matches what the server recevied and output to the terminal.

3. Result of command sha256sum outfile.txt: 738ef17c83e6c80f7a7d18e8566d8050f3a44bc00b5524c412859c43e0d64094  outfile.txt

4. There seemed to be a discernable difference between the time taken to run each command. byteAtATimeCmd took much longer, this being the case because it has to do 1000 times as many sends, 1000 times as many receives, and 1000 times as many writes to the log file as kByteAtATime. We could observe the difference by adding a timer function that starts at the beginning of the function and ends at the end of the function, and compare the two times.

5. About 18 hours.
