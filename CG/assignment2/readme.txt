Group number:6

Members :
Pratyush Anand (2010EEY7515)
Mahesh Chandra (2009EEZ8466)
Mansi Sharma (2010EEZ8083)
R. Prashanth (2010EEZ8051

; this is a program for axonometric projection of a cube.
; user can start with any angle and then can rotate with mouse for different angle values.

;To compile the code, command is
#make

;to clean the environment :
#make clean

; -x is angle of rotation around x axis
; -y is angle of rotation around y axis
; to exit use ctrl-c

; to run the code
#./axonometric -x 35.264 -y -45
; default value of x is 35.264.
; default value of y is -45.
; so if you do not pass any argument it will start with these angle.
; further you can also rotate with the help of mouse and see different coloured face of cube.
; current angle value is printed on the console.

;for any help
#./axonometric -h
