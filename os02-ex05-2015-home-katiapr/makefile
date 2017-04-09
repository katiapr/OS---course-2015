out: main.o mem_sim.o
	gcc main.o mem_sim.o -o out


main.o: main.c mem_sim.h
	gcc -c main.c

mem_sim.o: mem_sim.c mem_sim.h
	gcc -c mem_sim.c

clean:
	rm main.o mem_sim.o -o
