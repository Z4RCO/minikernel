/*
 * usuario/yosoy.c
 *
 *  Minikernel. Versión 1.0
 *
 *  Fernando Pérez Costoya
 *
 */

/*
 * Programa de usuario que simplemente imprime su identificador
 */

#include "servicios.h"

#define TOT_ITER 50000	/* ponga las que considere oportuno */

int main(){
	int i, id;

	id=obtener_id_pr();
	for (i=0; i<TOT_ITER; i++)
		printf("yosoy (%d): i %d\n", id, i);
	printf("yosoy (%d): termina\n", id);
	return 0;
}

