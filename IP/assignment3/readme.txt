To compile the code, command is
#make

to clean the environment :
#make clean

code has been written in openCV environment.
it has been tested on "Ubuntu 10.04 LTS - the Lucid Lynx - released in April 2010" with opencv library "rev 2.0.0-3ubuntu2".

[Part A: Image Compression ]
Gausian and Laplacian Pyramidal compression technique has been used for compression.
Both encoder and decoder has been implemented.
Encoder receives tif image as input and provides a compressed decoded image as output.
Decoder receives enocoded compressed image as input and tif image as output.

[Part B: Video Compression ]
Difference of two frames has been taken and then,
pyramidal segmenation technique has been used to segment out moving part.
First frame has been encoded as it is while for next frame only moving part has been 
encoded.
Here too an encoder and decoder has been implemented.

Code is totally original. However, implementation help of gausian and laplacian pyramid 
algorithm and various openCV function has been taken from internet.

for any query please contact to
pratyush.anand@gmail.com
pratyush.anand@st.com
+91-9971271781
