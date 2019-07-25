#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

typedef struct pixel { 
	unsigned char red, green, blue;
} __attribute__((packed)) Pixel; //un pixel citit din fisierul PPM

typedef struct block {
	unsigned char red, green, blue;
	struct block* top_left;
	struct block* top_right;
	struct block* bottom_right;
	struct block* bottom_left;
} *Block;   //un nod din arbore

typedef struct QuadtreeNode {
	unsigned char blue, green, red;
	uint32_t area;
	int32_t top_left, top_right;
	int32_t bottom_left, bottom_right;
} __attribute__((packed)) QuadtreeNode;

Block initTree() {
	Block new = malloc(sizeof(struct block));
	new->top_left = NULL;
	new->top_right = NULL;
	new->bottom_right = NULL;
	new->bottom_left = NULL;
	new->red = 0;
	new->green = 0;
	new->blue = 0;
	return new;
}

void initChildren(Block *b) {
	(*b)->top_left = initTree();
	(*b)->top_right = initTree();
	(*b)->bottom_right = initTree();
	(*b)->bottom_left = initTree();
}

void freeTree(Block *b) {
	if ((*b) == NULL)
		return;
	freeTree(&(*b)->top_left);
	freeTree(&(*b)->top_right);
	freeTree(&(*b)->bottom_right);
	freeTree(&(*b)->bottom_left);
	free(*b);
}

void divide(Block bloc, Pixel **grid, long long size, int i1, int i2, int j1, int j2, int prag, uint32_t *nr_nod, uint32_t *nr_cul) {

	long long blue = 0, green = 0, red = 0, sum = 0, mean;
	int i, j;

	//se calculeaza culoarea medie a blocului
	//i1, i2, j1, j2 sunt coordonate pentru colturile blocului 
	for (i = i1; i < i2; i++)
		for (j = j1; j < j2; j++) {
			red += (int)grid[i][j].red;
			green += (int)grid[i][j].green;
			blue += (int)grid[i][j].blue;
		}
	red = red / size;
	green = green / size;
	blue = blue / size;
	//se completeaza valorile in nodul respectiv din arborele de compresie
	bloc->red = red;
	bloc->green = green;
	bloc->blue = blue;
	//se calculeaza scorul de similaritate
	for (i = i1; i < i2; i++)
		for (j = j1; j < j2; j++)
			sum += pow((red - (int)grid[i][j].red), 2) + pow((green - (int)grid[i][j].green), 2) + pow((blue - (int)grid[i][j].blue), 2);
	mean = sum / (3 * size);
	//decide daca blocul mai trebuie divizat sau devine nod frunza in arbore
	if (mean <= prag)
		(*nr_cul)++;
	else {
		initChildren(&bloc);
		(*nr_nod) += 4;
		divide(bloc->top_left, grid, size / 4, i1, (i2 + i1) / 2, j1, (j2 + j1) / 2, prag, nr_nod, nr_cul);
		divide(bloc->top_right, grid, size / 4, i1, (i2 + i1) / 2, (j2 + j1) / 2, j2, prag, nr_nod, nr_cul);
		divide(bloc->bottom_right, grid, size / 4, (i2 + i1) / 2, i2, (j2 + j1) / 2, j2, prag, nr_nod, nr_cul);
		divide(bloc->bottom_left, grid, size / 4, (i2 + i1) / 2, i2, j1, (j2 + j1) / 2, prag, nr_nod, nr_cul);
	}
}

void tree_To_Vector(Block root, QuadtreeNode *v, uint32_t size, int *index) {

	//completeaza informatia elementului din vector
	v[(*index)].area = size;
	v[(*index)].red = root->red;
	v[(*index)].green = root->green;
	v[(*index)].blue = root->blue;
	if (root->top_left != NULL) { //daca nu este nod frunza, apeleaza functia pentru fii sai
		int keep = (*index);
		(*index)++;
		v[keep].top_left = (*index);
		tree_To_Vector(root->top_left, v, size / 4, index);
		(*index)++;
		v[keep].top_right = (*index);
		tree_To_Vector(root->top_right, v, size / 4, index);
		(*index)++;
		v[keep].bottom_right = (*index);
		tree_To_Vector(root->bottom_right, v, size / 4, index);
		(*index)++;
		v[keep].bottom_left = (*index);
		tree_To_Vector(root->bottom_left, v, size / 4, index);
	}
	else {
		v[(*index)].top_left = -1;
		v[(*index)].top_right = -1;
		v[(*index)].bottom_right = -1;
		v[(*index)].bottom_left = -1;
	}
}

void tree_To_Grid(Pixel **grid, Block root, int i1, int i2, int j1, int j2) {

	int i, j;
	//completeaza valorile pixelilor corespunzatori blocului
	for (i = i1; i < i2; i++)
		for (j = j1; j < j2; j++) {
			grid[i][j].red = root->red;
			grid[i][j].green = root->green;
			grid[i][j].blue = root->blue;
		}
	if (root->top_left != NULL) { //apeleaza functia pentru fii sai
		tree_To_Grid(grid, root->top_left, i1, (i2 + i1) / 2, j1, (j2 + j1) / 2);
		tree_To_Grid(grid, root->top_right, i1, (i2 + i1) / 2, (j2 + j1) / 2, j2);
		tree_To_Grid(grid, root->bottom_right, (i2 + i1) / 2, i2, (j2 + j1) / 2, j2);
		tree_To_Grid(grid, root->bottom_left, (i2 + i1) / 2, i2, j1, (j2 + j1) / 2);
	}
}

//citeste fisierul PPM si formeaza matricea de pixeli
Pixel** readImage(char *fname, uint32_t *width, uint32_t *height) {

	FILE *f = fopen(fname, "rb");
	char tip_fis[3], c[2];
	uint32_t max, i;
	fscanf(f, "%s\n%d %d\n%d%c", tip_fis, width, height, &max, c);
	Pixel **grid;
	grid = malloc(*height * sizeof(Pixel *));
	for (i = 0; i < *height; i++) 
		grid[i] = malloc(*width * sizeof(Pixel));
	for (i = 0; i < *height; i++)
		fread(grid[i], sizeof(Pixel), *width, f);
	printf("%d\n", (int)grid[0][0].red);
	fclose(f);
	return grid;
}

void Cerinta1(int prag, char *name1, char *name2) {

	uint32_t width, height, i, numar_culori = 0, numar_noduri = 5;
	Pixel **grid = readImage(name1, &width, &height);
	long long size = width * height;
	//se initializeaza radacina arborelui de compresia si se completeaza
	Block root = initTree();
	initChildren(&root);
	divide(root->top_left, grid, size / 4, 0, height / 2, 0, width / 2, prag, &numar_noduri, &numar_culori);
	divide(root->top_right, grid, size / 4, 0, height / 2, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root->bottom_right, grid, size / 4, height / 2, height, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root->bottom_left, grid, size / 4, height / 2, height, 0, width / 2, prag, &numar_noduri, &numar_culori);
	for (i = 0; i < height; i++)
		free(grid[i]);
	free(grid);
	
	//se formeaza vectorul si se scrie in fisierul de output
	QuadtreeNode *v;
	v = (QuadtreeNode *)malloc(numar_noduri * sizeof(QuadtreeNode));
	int index = 0;
	tree_To_Vector(root, v, size, &index);
	freeTree(&root);
	FILE *fout = fopen(name2, "wb");
	fwrite(&numar_culori, sizeof(uint32_t), 1, fout);
	fwrite(&numar_noduri, sizeof(uint32_t), 1, fout);
	fwrite(v, sizeof(QuadtreeNode), numar_noduri, fout);
	fclose(fout);
	free(v);
}

void Cerinta2(char *name1, char *name2) {

	FILE *f1, *fout;
	f1 = fopen(name1, "rb"); //citeste din fisier
	uint32_t numar_culori, numar_noduri, i;
	QuadtreeNode *v;
	fread(&numar_culori, sizeof(uint32_t), 1, f1);
	fread(&numar_noduri, sizeof(uint32_t), 1, f1);
	v = malloc(numar_noduri * sizeof(QuadtreeNode));
	fread(v, sizeof(QuadtreeNode), numar_noduri, f1);
	fclose(f1);

	//transpune informatia din vectorul citit (v) intr-unul cu elemente de tipul nodurilor arborelui de compresie (w)
	Block w = malloc(numar_noduri * sizeof(struct block)), root = &w[0];
	for (i = 0; i < numar_noduri; i++) {
		w[i].red = v[i].red;
		w[i].green = v[i].green;
		w[i].blue = v[i].blue;
		if (v[i].top_left == -1) //ajunge la un nod frunza
			w[i].top_left = NULL;
		else
			w[i].top_left = &w[v[i].top_left]; //leaga nodul parinte de fiul sau
		if (v[i].top_right == -1)
			w[i].top_right = NULL;
		else
			w[i].top_right = &w[v[i].top_right];
		if (v[i].bottom_right == -1)
			w[i].bottom_right = NULL;
		else
			w[i].bottom_right = &w[v[i].bottom_right];
		if (v[i].bottom_left == -1)
			w[i].bottom_left = NULL;
		else
			w[i].bottom_left = &w[v[i].bottom_left];
	}
	uint32_t width = floor(sqrt(v[0].area)), height = width;
	free(v);

	Pixel **grid;
	grid = malloc(height * sizeof(Pixel *));
	for (i = 0; i < height; i++)
		grid[i] = malloc(width * sizeof(Pixel));
	//extrage informatia din arbore in matricea de pixeli si scrie in fisier
	tree_To_Grid(grid, root, 0, height, 0, width); 
	free(w);
	fout = fopen(name2, "wb");
	fprintf(fout, "%s\n%u %u\n%u\n", "P6", width, height, 255);
	for (i = 0; i < height; i++)
		fwrite(grid[i], sizeof(Pixel), width, fout);
	for (i = 0; i < height; i++)
		free(grid[i]);
	free(grid);
	fclose(fout);
}

Block horizontal_m(Block root) {

	Block keep = root;
	if (root->top_left != NULL) { //daca nu e nod frunza
		//se interschimba blocul din stanga sus cu cel din dreapta sus
		Block aux = root->top_left;
		root->top_left = root->top_right;
		root->top_right = aux;
		//se interschimba blocul din stanga jos cu cel din dreapta jos
		aux = root->bottom_right;
		root->bottom_right = root->bottom_left;
		root->bottom_left = aux;
		//apeleaza functia pentru fii 
		root->top_left = horizontal_m(root->top_left);
		root->top_right = horizontal_m(root->top_right);
		root->bottom_right = horizontal_m(root->bottom_right);
		root->bottom_left = horizontal_m(root->bottom_left);
	}
	return keep;
}

Block vertical_m(Block root) {

	Block keep = root;
	if (root->top_left != NULL) { //daca nu e nod frunza
		//se interschimba blocul din stanga sus cu cel din stanga jos
		Block aux = root->top_left;
		root->top_left = root->bottom_left;
		root->bottom_left = aux;
		//se interschimba blocul din dreapta sus cu cel din dreapta jos
		aux = root->top_right;
		root->top_right = root->bottom_right;
		root->bottom_right = aux;
		//apeleaza functia pentru fii
		root->top_left = vertical_m(root->top_left);
		root->top_right = vertical_m(root->top_right);
		root->bottom_right = vertical_m(root->bottom_right);
		root->bottom_left = vertical_m(root->bottom_left);
	}
	return keep;
}

void Cerinta3(int type, int prag, char *name1, char *name2) {

	//se citeste matricea de pixeli, se formeaza arborele de compresie
	uint32_t width, height, i, numar_culori = 0, numar_noduri = 5;
	Pixel **grid = readImage(name1, &width, &height);
	long long size = width * height;
	Block root = initTree();
	initChildren(&root);
	divide(root->top_left, grid, size / 4, 0, height / 2, 0, width / 2, prag, &numar_noduri, &numar_culori);
	divide(root->top_right, grid, size / 4, 0, height / 2, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root->bottom_right, grid, size / 4, height / 2, height, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root->bottom_left, grid, size / 4, height / 2, height, 0, width / 2, prag, &numar_noduri, &numar_culori);
	//oglindeste imaginea modificand legaturile din arbore
	switch (type) {
		case 'h':
			root = horizontal_m(root);
			break;
		case 'v':
			root = vertical_m(root);
			break;
	}
	//formeaza noua matrice de pixeli si o scrie in fisier
	tree_To_Grid(grid, root, 0, height, 0, width);
	freeTree(&root);
	FILE *fout = fopen(name2, "wb");
	fprintf(fout, "%s\n%u %u\n%u\n", "P6", width, height, 255);
	for (i = 0; i < height; i++)
		fwrite(grid[i], sizeof(Pixel), width, fout);
	for (i = 0; i < height; i++)
		free(grid[i]);
	free(grid);
	fclose(fout);
}

//parcurge doi arbori de compresie diferiti si formeaza unul nou 
void combine(Block *root, Block root1, Block root2) {

	//culoarea dintr-un nod din arborele nou va fi media culorilor din cei 2
	(*root)->red = (root1->red + root2->red) / 2;
	(*root)->green = (root1->green + root2->green) / 2;
	(*root)->blue = (root1->blue + root2->blue) / 2;
	if (root1->top_left == NULL && root2->top_left == NULL) //s-au parcurs complet cei doi arbori
		return;
	initChildren(root);
	if (root1->top_left != NULL && root2->top_left != NULL) {
		combine(&(*root)->top_left, root1->top_left, root2->top_left);
		combine(&(*root)->top_right, root1->top_right, root2->top_right);
		combine(&(*root)->bottom_right, root1->bottom_right, root2->bottom_right);
		combine(&(*root)->bottom_left, root1->bottom_left, root2->bottom_left);
	}
	if (root1->top_left != NULL && root2->top_left == NULL) {
		combine(&(*root)->top_left, root1->top_left, root2);
		combine(&(*root)->top_right, root1->top_right, root2);
		combine(&(*root)->bottom_right, root1->bottom_right, root2);
		combine(&(*root)->bottom_left, root1->bottom_left, root2);
	}
	if (root1->top_left == NULL && root2->top_left != NULL) {
		combine(&(*root)->top_left, root1, root2->top_left);
		combine(&(*root)->top_right, root1, root2->top_right);
		combine(&(*root)->bottom_right, root1, root2->bottom_right);
		combine(&(*root)->bottom_left, root1, root2->bottom_left);
	}
}

void Bonus(int prag, char *name1, char *name2, char *name3)
{
	//formeaza primul arbore de compresie
	uint32_t width, height, i, numar_culori = 0, numar_noduri = 5;
	Pixel **grid1 = readImage(name1, &width, &height);
	long long size = width * height;
	Block root1 = initTree();
	initChildren(&root1);
	divide(root1->top_left, grid1, size / 4, 0, height / 2, 0, width / 2, prag, &numar_noduri, &numar_culori);
	divide(root1->top_right, grid1, size / 4, 0, height / 2, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root1->bottom_right, grid1, size / 4, height / 2, height, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root1->bottom_left, grid1, size / 4, height / 2, height, 0, width / 2, prag, &numar_noduri, &numar_culori);

	//formeaza al doilea arbore de compresie
	numar_culori = 0; numar_noduri = 5;
	Pixel **grid2 = readImage(name2, &width, &height);
	Block root2 = initTree();
	initChildren(&root2);
	divide(root2->top_left, grid2, size / 4, 0, height / 2, 0, width / 2, prag, &numar_noduri, &numar_culori);
	divide(root2->top_right, grid2, size / 4, 0, height / 2, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root2->bottom_right, grid2, size / 4, height / 2, height, width / 2, width, prag, &numar_noduri, &numar_culori);
	divide(root2->bottom_left, grid2, size / 4, height / 2, height, 0, width / 2, prag, &numar_noduri, &numar_culori);
	
	//formeaza noul arbore de compresie, apoi matricea de pixeli si o scrie in fisier
	Block root = initTree();
	combine(&root, root1, root2);
	freeTree(&root1);
	freeTree(&root2);
	tree_To_Grid(grid1, root, 0, height, 0, width);
	freeTree(&root);
	FILE *fout;
	fout = fopen(name3, "wb");
	fprintf(fout, "%s\n%u %u\n%u\n", "P6", width, height, 255);
	for (i = 0; i < height; i++) 
		fwrite(grid1[i], sizeof(Pixel), width, fout);
	for (i = 0; i < height; i++) {
		free(grid1[i]);
		free(grid2[i]);
	}
	free(grid1); free(grid2);
	fclose(fout);
}

int main(int argc, char **argv)
{
	switch (argv[1][1]) {
		case 'c':
			Cerinta1(atoi(argv[2]), argv[3], argv[4]);
			break;
		case 'd':
			Cerinta2(argv[2], argv[3]);
			break;
		case 'm':
			Cerinta3(argv[2][0], atoi(argv[3]), argv[4], argv[5]);
			break;
		case 'o':
			Bonus(atoi(argv[2]), argv[3], argv[4], argv[5]);
			break;
	}
	return 0;
}