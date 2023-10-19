/*
 *  minikernel/include/kernel.h
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero de cabecera que contiene definiciones usadas por kernel.c
 *
 *      SE DEBE MODIFICAR PARA INCLUIR NUEVA FUNCIONALIDAD
 *
 */

#ifndef _KERNEL_H
#define _KERNEL_H


#include "const.h"
#include "HAL.h"
#include "llamsis.h"

/*
 *
 * Definicion del tipo que corresponde con el BCP.
 * Se va a modificar al incluir la funcionalidad pedida.
 *
 */
typedef struct BCP_t *BCPptr;

typedef struct BCP_t {
    int id;				/* ident. del proceso */
    int estado;			/* TERMINADO|LISTO|EJECUCION|BLOQUEADO*/
    contexto_t contexto_regs;	/* copia de regs. de UCP */
    void * pila;			/* dir. inicial de la pila */
	BCPptr siguiente;		/* puntero a otro BCP */
	void *info_mem;			/* descriptor del mapa de memoria */

    //TODO nuevo campo contador de segundos
    unsigned int segundosDormido;    /*Segundos que tiene que estar dormido el proceso*/

    //TODO nuevos campos mutex
    //TODO Inicializar cosas
    int descriptoresMutex[NUM_MUT_PROC]; /*Lista de descriptores de mutex asociada al proceso*/

} BCP;

typedef struct Mutex_t{
    char nombre[MAX_NOM_MUT];
    int tipo; //recursivo o no
    int proc_esperando; // contador de procesos esperando
    lista_BCPs lista_Procesos_Esperando; //procesos esperando
    int estado; //bloqueado o desbloqueado;

}Mutex;



/*
 *
 * Definicion del tipo que corresponde con la cabecera de una lista
 * de BCPs. Este tipo se puede usar para diversas listas (procesos listos,
 * procesos bloqueados en sem�foro, etc.).
 *
 */

typedef struct{
	BCP *primero;
	BCP *ultimo;
} lista_BCPs;


/*
 * Variable global que identifica el proceso actual
 */

BCP * p_proc_actual=NULL;

/*
 * Variable global que representa la tabla de procesos
 */

BCP tabla_procs[MAX_PROC];

/*
 * Variable global que representa la cola de procesos listos
 */
lista_BCPs lista_listos= {NULL, NULL};


//TODO Nueva lista dormidos
/*
 * Variable global que representa la cola de procesos dormidos
 */
lista_BCPs lista_dormidos= {NULL, NULL};

lista_BCPs lista_bloqueados_mutex= {NULL, NULL};

/*
 *
 * Definici�n del tipo que corresponde con una entrada en la tabla de
 * llamadas al sistema.
 *
 */
typedef struct{
	int (*fservicio)();
} servicio;


/*
 * Prototipos de las rutinas que realizan cada llamada al sistema
 */
int sis_crear_proceso();
int sis_terminar_proceso();
int sis_escribir();

//TODO servicios dormir
int sis_dormir();
int sis_obtener_id_pr();
//TODO servicio mutex
int sis_crear_mutex();
int sis_abrir_mutex();
int sis_lock();
int sis_unlock();
int sis_cerrar_mutex();



/*
 * Variable global que contiene las rutinas que realizan cada llamada
 */
//TODO nuevo
servicio tabla_servicios[NSERVICIOS]={	{sis_crear_proceso},
					{sis_terminar_proceso},
					{sis_escribir},
                    {sis_dormir},
                    {sis_obtener_id_pr},
                    {sis_crear_mutex},
                    {sis_abrir_mutex},
                    {sis_lock},
                    {sis_unlock},
                    {sis_cerrar_mutex}}

#endif /* _KERNEL_H */

