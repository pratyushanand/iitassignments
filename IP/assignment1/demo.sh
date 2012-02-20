rm *.tiff
#2.1 : Change from RGB to Gray Scale
./convert -i face.jpg -o face_gray.tiff
./convert -i map.jpg -o map_gray.tiff
#1: Halftone using Floud and Steinberg
./halftone -i face_gray.tiff -o face_halftone.tiff
./halftone -i map_gray.tiff -o map_halftone.tiff
#2.2 : Preprocess the image : Smooth using Median filter with kernel size 3x3
./enhancement -s -i face_gray.tiff -o face_smooth.tiff -t median -x 3 -y 3
./enhancement -s -i map_gray.tiff -o map_smooth.tiff -t median -x 3 -y 3
#2.3.1 : Detect edge using Soble operator with threshold value 100
./edge_detect -i face_smooth.tiff -o face_edge.tiff -t 130
./edge_detect -i map_smooth.tiff -o map_edge.tiff -t 380
#2.3.1 : Link edgees using Hough Transform with threshold value 100
./edge_link -i map_edge.tiff -o map_link.tiff -t 150
#2.5 : cleaning of edges
./cleaning -i face_edge.tiff -o face_cleaned.tiff -x 8 -y 8 -t 8
./cleaning -i map_link.tiff -o map_cleaned.tiff -x 3 -y 3 -t 2
#2.4 : thickening of edges
./thickening -i face_cleaned.tiff -o face_thick.tiff
./thickening -i map_cleaned.tiff -o map_thick.tiff
#2.6 : superposition of line art with original image
./superposition -i face_gray.tiff -j face_thick.tiff -o face_superposition.tiff
./superposition -i map_gray.tiff -j map_thick.tiff -o map_superposition.tiff
