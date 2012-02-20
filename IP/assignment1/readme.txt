To compile the code, command is
#make

to clean the environment :
#make clean

code has been written in openCV environment.
it has been tested on "Ubuntu 10.04 LTS - the Lucid Lynx - released in April 2010" with opencv library "rev 2.0.0-3ubuntu2".


Code files have been names as the assignment subpart.

[A halftoned image using area based approach with Floyd and Steinberg error correction. This you have to implement.]
This part has been implemented in halftone.c.
For help of input arguments :
#./halftone -h

[Conversion of a color image to gray scale image if the input image is colored, you can use any in-built function for this.]
This part has been implemented in convert.c.
For help of input arguments :
#./convert -h

[Image enhancement for preprocessing, you can use in-built functions. These may include filters, histogram processing, grey level transformation. The purpose of these could be reduce noise, smoothing of the image or as pre-processing to help segmentation.]
This part has been implemented in enhancement.c.
For help of input arguments :
#./enhancement -h

[Segmentation-- includes edge detection]
This part has been implemented in edge_detect.c.
For help of input arguments :
#./edge_detect -h

[and linking, for line detection you are required to implement Hough transforms. This part should be completely implemented by you.]
This part has been implemented in edge_link.c.
For help of input arguments :
#./edge_link -h

[Thikening of the borders of the silhouttes. This is to implemented. It may be considered as a brush with some thickness as a parameter.]
This part has been implemented in thickening.c.
For help of input arguments :
#./thickening -h

[Cleaning (brush) -- to clean a specified region. This is to be implemented. The purpose of this operation is remove the small features. The region may be specified by giving a seed point.]
This part has been implemented in cleaning.c.
For help of input arguments :
#./cleaning -h

[Post processing operations. This may involve superposition of line art onto the original image.]
This part has been implemented in superposition.c.
For help of input arguments :
#./superposition -h

[You are required to demonstrate your implementation on atleast two types of image examples a) a city map b) a portrait of yourself.]
A demo.sh script has been prepared to demonstarte above implementation on face.jpg and map.jpg
#./demo.sh


for any query please contact to
pratyush.anand@gmail.com
pratyush.anand@st.com
+91-9971271781
