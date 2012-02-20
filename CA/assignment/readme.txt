To compile the code, command is
#make

to clean the environment :
#make clean

code has been written in openCV environment.
it has been tested on "Ubuntu 10.04 LTS - the Lucid Lynx - released in April
2010" with opencv library "rev 2.0.0-3ubuntu2".

This code does the edge detection for the number of frames in a video file.
It accepts following input:
-i: input mpeg file.
-o: output mpeg file.
-t: threshold for edge detection.
-c: number of CPUs
-n: number of processes (threads actually)

A typical execution with 4 cpus and 4 processes can be done with following
command.
#./edge_detect -i umcp.mpg -o output.mpg -t 200 -c 4 -n 4

After running this command , it waits for a user interrupt(ctrl-c). Waiting has
been introduced to verify scheduling of processes on different CPUS.
This scheduling can be verified with following command, while running the above
execution in background (by putting & at the end).
#ps -TP

Once scheduling verified, above execution can be brought back to forground with
following command.
#fg

Now press ctrl-c to execute it.

Following table shows about its performance with varied number of cpus and
processes.

No. of CPU	No of processes		Time (in us)
1			1		10529815
1			2		10070986
1			3		09905863
1			4		10090756
1			5		10126281
2			1		10330996
2			2		06806299
2			3		06772445
2			4		06721058
2			5		06453078
2			6		06603475
3			1		10303337
3			2		06622124
3			3		06332407
3			4		06119462
3			5		06056034
3			6		06160946
3			7		05987967
4			1		10534749
4			2		06590646
4			3		05848971
4			4		05708176
4			5		05657253
4			6		05363943
4			7		05140710
4			8		05694022

Observation/Explanation for result:
1. Performance increases if we increase number of processor.
2. If number of processor remains constant and more number of threads are
executed on per cpu basis, then performace increses untill a certain linit
then , it starts decreasing. It starts decreasing, because of thread scheduling
overhead when CPU is fully loaded.
3. One typical case with cpu=3 and processes = 5, 6, 7. Here we observed that
performance deteriorated during 5 to 6, but it improved during 6 to 7.
Surprizing, is not it!!! No, its not surprizing. When we use 6 processes, all
CPUscare equally loaded with 2 threads. However, CPU0 has slightly more load ,
as it also scheduls main thread which envokes all the child thread. CPU0 was
already fully loaded. When we increase number of processes to 7, cpu 1 which
was not fully loaded earlier, now gets loaded completely.So, performance
improves.

for any query please contact to
pratyush.anand@gmail.com
pratyush.anand@st.com
+91-9971271781
