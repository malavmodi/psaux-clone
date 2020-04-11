all: PS_AUX

PS_AUX: 
	gcc -g -O my_ps.c -o my_ps -lm;
	tput rmam

clean:
	rm -rf my_ps;
