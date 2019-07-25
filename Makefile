build: quadtree.c
	gcc -Wall quadtree.c -o quadtree -g -lm
clean:
	rm -rf quadtree
archive: quadtree.c
	zip Deaconu_Andreea_Carina_314CC_Tema2_SD.zip README Makefile quadtree.c
check: quadtree.c
	valgrind --tool=memcheck --leak-check=full --leak-check=full --show-leak-kinds=all
