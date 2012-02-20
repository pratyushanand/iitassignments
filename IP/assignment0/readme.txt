To compile the code, command is
#make

to clean the environment :
#make clean

code has been written in openCV environment.
it has been tested on "Ubuntu 10.04 LTS - the Lucid Lynx - released in April 2010" with opencv library "rev 2.0.0-3ubuntu2".


input args :
-i is name of input image
-o is name of ouput image
-s is scaling factor in float
-m is method, which can be either replicate or interpolate
it supports singal and three channel images with depth of 8 bit.

for any help
#./scale -h

input image is color.jpg. It has to be scaled by 2.4 times with interpolate method and an output image of name scaledup2pt4.jpg has to be built
#./scale -i color.jpg -o scaledup2pt4.jpg -s 2.4 -m interpolate

input image is raw.pgm. It has to be scaled by 0.3 times with replicate method and an output image of name scaledddwpt3.jpg has to be built
#./scale -i raw.pgm -o scaleddwpt3.jpg -s 0.3 -m replicate


for any query please contact to
pratyush.anand@gmail.com
pratyush.anand@st.com
+91-9971271781
