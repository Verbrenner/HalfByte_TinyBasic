100 CLS
110 X=1
120 Y=39
130 T=X+X
140 F=T+T
150 D=RND(6)+X
160 CLS
170 GOSUB 1000
180 DELAY 2000
190 GOTO 150
1000 IF D=2 SET X,Y
1005 IF D>=4 SET X,Y
1010 IF D=6 SET X,Y+T
1020 IF D>=3 SET X,Y+F
1030 IF D=1 SET X+T,Y+T
1032 IF D=3 SET X+T,Y+T
1034 IF D=5 SET X+T,Y+T
1040 IF D>=3 SET X+F,Y
1050 IF D=6 SET X+F,Y+T
1060 IF D=2 SET X+F,Y+F
1062 IF D>=4 SET X+F,Y+F
1070 RETURN