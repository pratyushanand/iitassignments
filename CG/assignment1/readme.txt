;To compile the code, command is
#make

;to clean the environment :
#make clean

;for line drawing:

; -x is xStart co-ordinate
; -y is xEnd co-ordinate
; -e is yStart co-ordinate
; -z is yEnd co-ordinate
; -w is width of window
; -h is height of window

; to exit use ctrl-c

;case m = 0 (horizontal line)
#./line -x 5 -y 50 -e 200 -z 50 -w 500 -h 500

;case m = inf (vertical line)
#./line -x 50 -y 5 -e 50 -z 200 -w 500 -h 500

; case m +
#./line -x 50 -y 5 -e 100 -z 200 -w 500 -h 500

;case m -
#./line -x 50 -y 5 -e 300 -z 200 -w 500 -h 500

;for any help
#./line -h


;for circle drawing:

; -x is x co-ordinate of center
; -y is y co-ordinate of center
; -r is redius of circle
; -w is width of window
; -h is height of window

#./circle -x 100 -y -100 -r 200 -w 500 -h 500

; for any help
#./circle -h
