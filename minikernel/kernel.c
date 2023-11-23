/*
 *  kernel/kernel.c
 *
 *  Minikernel. Versi�n 1.0
 *
 *  Fernando P�rez Costoya
 *
 */

/*
 *
 * Fichero que contiene la funcionalidad del sistema operativo
 *
 */

#include "kernel.h"    /* Contiene defs. usadas por este modulo */
#include <string.h> /*Funciones para trabajo con cadenas */
#include <stdlib.h>
#include <stdio.h>

/*
 *
 * Funciones relacionadas con la tabla de procesos:
 *	iniciar_tabla_proc buscar_BCP_libre
 *
 */

/*
 * Funci�n que inicia la tabla de procesos
 */
static void iniciar_tabla_proc() {
    int i;

    for (i = 0; i < MAX_PROC; i++)
        tabla_procs[i].estado = NO_USADA;
}

/*
 * Funci�n que busca una entrada libre en la tabla de procesos
 */
static int buscar_BCP_libre() {
    int i;

    for (i = 0; i < MAX_PROC; i++)
        if (tabla_procs[i].estado == NO_USADA)
            return i;
    return -1;
}

/*
 *
 * Funciones que facilitan el manejo de las listas de BCPs
 *	insertar_ultimo eliminar_primero eliminar_elem
 *
 * NOTA: PRIMERO SE DEBE LLAMAR A eliminar Y LUEGO A insertar
 */

/*
 * Inserta un BCP al final de la lista.
 */
static void insertar_ultimo(lista_BCPs *lista, BCP *proc) {
    if (lista->primero == NULL)
        lista->primero = proc;
    else
        lista->ultimo->siguiente = proc;
    lista->ultimo = proc;
    proc->siguiente = NULL;
}

/*
 * Elimina el primer BCP de la lista.
 */
static void eliminar_primero(lista_BCPs *lista) {

    if (lista->ultimo == lista->primero)
        lista->ultimo = NULL;
    lista->primero = lista->primero->siguiente;
}

/*
 * Elimina un determinado BCP de la lista.
 */
static void eliminar_elem(lista_BCPs *lista, BCP *proc) {
    BCP *paux = lista->primero;

    if (paux == proc)
        eliminar_primero(lista);
    else {
        for (; ((paux) && (paux->siguiente != proc));
               paux = paux->siguiente);
        if (paux) {
            if (lista->ultimo == paux->siguiente)
                lista->ultimo = paux;
            paux->siguiente = paux->siguiente->siguiente;
        }
    }
}

/*
 *
 * Funciones relacionadas con la planificacion
 *	espera_int planificador
 */

/*
 * Espera a que se produzca una interrupcion
 */
static void espera_int() {
    int nivel;

    printk("-> NO HAY LISTOS. ESPERA INT\n");

    /* Baja al m�nimo el nivel de interrupci�n mientras espera */
    nivel = fijar_nivel_int(NIVEL_1);
    halt();
    fijar_nivel_int(nivel);
}

/*
 * Funci�n de planificacion que implementa un algoritmo FIFO.
 */
static BCP *planificador() {
    while (lista_listos.primero == NULL)
        espera_int();        /* No hay nada que hacer */
    return lista_listos.primero;
}

/*
 *
 * Funcion auxiliar que termina proceso actual liberando sus recursos.
 * Usada por llamada terminar_proceso y por rutinas que tratan excepciones
 *
 */
static void liberar_proceso() {
    BCP *p_proc_anterior;

    liberar_imagen(p_proc_actual->info_mem); /* liberar mapa */

    p_proc_actual->estado = TERMINADO;
    eliminar_primero(&lista_listos); /* proc. fuera de listos */

    /* Realizar cambio de contexto */
    p_proc_anterior = p_proc_actual;
    p_proc_actual = planificador();

    printk("-> C.CONTEXTO POR FIN: de %d a %d\n",
           p_proc_anterior->id, p_proc_actual->id);

    liberar_pila(p_proc_anterior->pila);
    cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    return; /* no deber�a llegar aqui */
}

/*
 *
 * Funciones relacionadas con el tratamiento de interrupciones
 *	excepciones: exc_arit exc_mem
 *	interrupciones de reloj: int_reloj
 *	interrupciones del terminal: int_terminal
 *	llamadas al sistemas: llam_sis
 *	interrupciones SW: int_sw
 *
 */

/*
 * Tratamiento de excepciones aritmeticas
 */
static void exc_arit() {

    if (!viene_de_modo_usuario())
        panico("excepcion aritmetica cuando estaba dentro del kernel");


    printk("-> EXCEPCION ARITMETICA EN PROC %d\n", p_proc_actual->id);
    liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de excepciones en el acceso a memoria
 */
static void exc_mem() {

    if (!viene_de_modo_usuario())
        panico("excepcion de memoria cuando estaba dentro del kernel");


    printk("-> EXCEPCION DE MEMORIA EN PROC %d\n", p_proc_actual->id);
    liberar_proceso();

    return; /* no deber�a llegar aqui */
}

/*
 * Tratamiento de interrupciones de terminal
 */
static void int_terminal() {
    char car;

    car = leer_puerto(DIR_TERMINAL);
    printk("-> TRATANDO INT. DE TERMINAL %c\n", car);

    return;
}

/*
 * Tratamiento de interrupciones de reloj
 */
static void int_reloj() {
    printk("-> TRATANDO INT. DE RELOJ\n");
    BCPptr dormidoActual = lista_dormidos.primero;
    while (dormidoActual != NULL) {
        (dormidoActual->segundosDormido)--;

        if (dormidoActual->segundosDormido < 0) {
            halt();
        }

        if (dormidoActual->segundosDormido == 0) {
            dormidoActual->estado = LISTO;

            //Eliminar dormidoActual de la lista de bloqueados
            eliminar_elem(&lista_dormidos, dormidoActual);

            //Añadir dormidoActual a lista de listos
            insertar_ultimo(&lista_listos, dormidoActual);

        }
        dormidoActual = dormidoActual->siguiente;
    }
    return;
}

/*
 * Tratamiento de llamadas al sistema
 */
static void tratar_llamsis() {
    int nserv, res;

    nserv = leer_registro(0);
    if (nserv < NSERVICIOS)
        res = (tabla_servicios[nserv].fservicio)();
    else
        res = -1;        /* servicio no existente */
    escribir_registro(0, res);
    return;
}

/*
 * Tratamiento de interrupciuones software
 */
static void int_sw() {

    printk("-> TRATANDO INT. SW\n");

    return;
}

/*
 *
 * Funcion auxiliar que crea un proceso reservando sus recursos.
 * Usada por llamada crear_proceso.
 *
 */
static int crear_tarea(char *prog) {
    void *imagen, *pc_inicial;
    int error = 0;
    int proc, i;
    BCP *p_proc;

    proc = buscar_BCP_libre();
    if (proc == -1)
        return -1;    /* no hay entrada libre */

    /* A rellenar el BCP ... */
    p_proc = &(tabla_procs[proc]);

    /* crea la imagen de memoria leyendo ejecutable */
    imagen = crear_imagen(prog, &pc_inicial);
    if (imagen) {
        p_proc->info_mem = imagen;
        p_proc->pila = crear_pila(TAM_PILA);
        fijar_contexto_ini(p_proc->info_mem, p_proc->pila, TAM_PILA,
                           pc_inicial,
                           &(p_proc->contexto_regs));
        p_proc->id = proc;
        p_proc->estado = LISTO;

        p_proc->segundosDormido = 0;
        for(i = 0; i < NUM_MUT_PROC; i++){
            p_proc->descriptoresMutex[i] = -1;
        }

        /* lo inserta al final de cola de listos */
        insertar_ultimo(&lista_listos, p_proc);
        error = 0;
    } else
        error = -1; /* fallo al crear imagen */

    return error;
}

/*
 *
 * Rutinas que llevan a cabo las llamadas al sistema
 *	sis_crear_proceso sis_escribir
 *
 */

/*
 * Tratamiento de llamada al sistema crear_proceso. Llama a la
 * funcion auxiliar crear_tarea sis_terminar_proceso
 */
int sis_crear_proceso() {
    char *prog;
    int res;

    printk("-> PROC %d: CREAR PROCESO\n", p_proc_actual->id);
    prog = (char *) leer_registro(1);
    res = crear_tarea(prog);
    return res;
}

/*
 * Tratamiento de llamada al sistema escribir. Llama simplemente a la
 * funcion de apoyo escribir_ker
 */
int sis_escribir() {
    char *texto;
    unsigned int longi;

    texto = (char *) leer_registro(1);
    longi = (unsigned int) leer_registro(2);

    escribir_ker(texto, longi);
    return 0;
}

/*
 * Tratamiento de llamada al sistema terminar_proceso. Llama a la
 * funcion auxiliar liberar_proceso
 */
int sis_terminar_proceso() {

    printk("-> FIN PROCESO %d\n", p_proc_actual->id);

    liberar_proceso();

    return 0; /* no deber�a llegar aqui */
}


//TODO llamada dormir
int sis_dormir() {
    unsigned int segundos = (unsigned int) leer_registro(1);

    //Fijar nivel 1
    int nivel = fijar_nivel_int(NIVEL_1);
    if(segundos < 1){
        printf("Error\n");
        fijar_nivel_int(nivel);
        return -1;
    }

    //Variable local proceso a dormir
    BCP proceso_dormir = p_proc_actual;
    //Seleccionar nuevo proceso a ejeuctar
    p_proc_actual = planificador();


    //Cambiar estado del proceso
    proceso_dormir->estado = BLOQUEADO;
    proceso_dormir->segundosDormido = segundos * TICK;



    //Eliminar el proceso de la lista de listos
    eliminar_primero(&lista_listos);

    //Añadir proceso a lista de bloqueados
    insertar_ultimo(&lista_dormidos, proceso_dormir);






    //Cambiar contexto

    cambio_contexto(&(proceso_dormir.contexto_regs), &(p_proc_actual->contexto_regs));


    //Vuelves a permitir interrupciones
    fijar_nivel_int(nivel);
    return 0;
}


//TODO servicio obtener_id_pr
/*
 *
 */
int sis_obtener_id_pr() {
    return p_proc_actual->id;
}

//TODO servicio crear_mutex
/*
 *
 */
int sis_crear_mutex() {
    int nivel = fijar_nivel_int(1);
    char *nombre = (char *) leer_registro(1);
    int tipo = (int) leer_registro(2);
    int encontrado = 0;
    int i, size = 0, lista = 0;



    //Si el tipo es erroneo, finalizar con error -1
    if (tipo != RECURSIVO && tipo != NO_RECURSIVO)return -1;

    if (strlen(nombre) > MAX_NOM_MUT)
        return -2;          //Si el nombre es más largo que el máximo, finalizar con error -2


    for (i = 0; i < NUM_MUT && !encontrado; i++) {
        if (lista_mutex[i] != NULL) {
            if (!strcmp(nombre, lista_mutex[i]->nombre)) {
                encontrado = 1;
            }
            size++;
        }
    }

    if (encontrado)return -1;     // Si existe otro mutex con ese nombre, finalizar con error


    //comprobar descriptores libres para el proceso
    int *mutexes = p_proc_actual->descriptoresMutex;
    int descriptoresLibres = NUM_MUT_PROC;
    i = 0;
    while (i < NUM_MUT_PROC) {
        if (mutexes[i] != -1) {
            descriptoresLibres--;
        }
        i++;
    }
    if (descriptoresLibres <= 0)return -5;               //Si no quedan descriptores libres, devuelves con error -5


    //si la lista está llena, esperar a que se libere
    if (size == NUM_MUT) {
        p_proc_actual->estado = BLOQUEADO;
        insertar_ultimo(&(lista_bloqueados_mutex), p_proc_actual);
        BCPptr proceso_bloquear = p_proc_actual;
        p_proc_actual = planificador();
        cambio_contexto(&(proceso_bloquear->contexto_regs), &(p_proc_actual->contexto_regs));
        fijar_nivel_int(nivel);
        return 0;
    }

    // Inicializar el mutex
    Mutexptr mutex = (Mutexptr) malloc(sizeof(Mutex));
    mutex->nombre = nombre;
    mutex->tipo = tipo;
    mutex->proc_esperando = 0;
    mutex->estado = UNLOCKED;
    mutex->bloqueos = 0;
    mutex->contadorProcesos = 0;


    // Buscar posicion libre en la lista
    int posicion = 0;
    encontrado = 0;
    for (int i = 0; i < NUM_MUT && !encontrado; i++) {
        if (lista_mutex[i] == NULL)encontrado = 1;
        posicion = i;
    }

    mutex->id = posicion;
    lista_mutex[posicion] = mutex;

    fijar_nivel_int(nivel);             // Devolver nivel al anterior

    for(i = 0; i < NUM_MUT; i++){
        if(lista_mutex[i] != NULL)lista++;
    }
    printf("Crear. %d\n", lista);

    return mutex->id;
}

int sis_abrir_mutex() {
    int nivel = fijar_nivel_int(1);
    char *nombre = (char *) leer_registro(1);
    int encontrado = 0, i, posicion;
    Mutexptr mutex = NULL;
    for (i = 0; i < NUM_MUT && !encontrado; i++) {
        if (&lista_mutex[i] != NULL) {
            if (!strcmp(lista_mutex[i]->nombre, nombre)) {
                encontrado = 1;
                mutex = lista_mutex[i];
            }
        }
    }
    if (!encontrado) {
        fijar_nivel_int(nivel);
        return -1;
    }
    encontrado = 0;
    for(i = 0; i < NUM_MUT_PROC && !encontrado; i++){
        if(p_proc_actual->descriptoresMutex[i] == -1)encontrado = 1;
        posicion = i;
    }
    if(!encontrado){
        fijar_nivel_int(nivel);
        return -2;
    }
    p_proc_actual->descriptoresMutex[posicion] = mutex->id;
    printf("Abrir %d.\n", mutex->id);
    fijar_nivel_int(nivel);
    return mutex->id;
}

int sis_lock() {
    int nivel = fijar_nivel_int(1);
    unsigned int mutexId = (unsigned int) leer_registro(1);
    int i, encontrado = 0;
    Mutexptr mutex = NULL;

    // Buscar el mutex
    for (i = 0; i < NUM_MUT && !encontrado; i++) {
        if (lista_mutex[i] != NULL) {
            if (lista_mutex[i]->id == mutexId) {
                encontrado = 1;
                mutex = lista_mutex[i];
            }
        }
    }
    if (!encontrado) {                // Si no lo encuentra finaliza con error
        fijar_nivel_int(nivel);
        return -1;
    }


    printf("LOCK. %d por p%d\n", mutexId, p_proc_actual->id);
    if (mutex->estado == UNLOCKED) {
        mutex->proceso = p_proc_actual;
        mutex->estado = LOCKED;
        if (mutex->tipo == RECURSIVO)mutex->bloqueos++;
        fijar_nivel_int(nivel);
        return 0;
    }
    if (mutex->tipo == NO_RECURSIVO) {       //El mutex está bloqueado
        if (mutex->proceso == p_proc_actual) {
            fijar_nivel_int(nivel);
            return -1;
        }
        mutex->contadorProcesos++;
        p_proc_actual->estado = BLOQUEADO;// Bloquear proceso
        eliminar_primero(&lista_listos);
        insertar_ultimo(&mutex->lista_Procesos_Esperando,
                        p_proc_actual);   // Insertar proceso en lista de bloqueados por el mutex
        // Cambiar contexto
        BCPptr proceso = p_proc_actual;
        p_proc_actual = planificador();
        cambio_contexto(&(proceso->contexto_regs), &(p_proc_actual->contexto_regs));
        fijar_nivel_int(nivel);
        return 0;
    }
    if (mutex->proceso == p_proc_actual) {
        mutex->bloqueos++;
        fijar_nivel_int(nivel);
        return 0;
    }
    mutex->contadorProcesos++;
    p_proc_actual->estado = BLOQUEADO;                                  // Bloquear proceso
    eliminar_primero(&lista_listos);
    // Insertar proceso en lista de bloqueados por el mutex
    insertar_ultimo(&mutex->lista_Procesos_Esperando, p_proc_actual);
    // Cambiar contexto
    BCPptr proceso = p_proc_actual;
    p_proc_actual = planificador();
    cambio_contexto(&(proceso->contexto_regs), &(p_proc_actual->contexto_regs));
    fijar_nivel_int(nivel);


    return 0;
}

int sis_unlock() {
    unsigned int mutexId = (unsigned int) leer_registro(1);
    int nivel = fijar_nivel_int(1);
    int i, encontrado = 0;
    Mutexptr mutex = NULL;

    // Buscar el mutex
    for (i = 0; i < NUM_MUT && !encontrado; i++) {
        if (lista_mutex[i] != NULL) {
            if (lista_mutex[i]->id == mutexId) {
                encontrado = 1;
                mutex = lista_mutex[i];
            }
        }
    }
    if (!encontrado) {                // Si no lo encuentra finaliza con error
        fijar_nivel_int(nivel);
        return -1;
    }


    if (mutex->proceso != p_proc_actual) {
        fijar_nivel_int(nivel);
        return -2;
    }
    if (mutex->tipo == NO_RECURSIVO) {
        if (mutex->estado == UNLOCKED) {
            fijar_nivel_int(nivel);
            return -3;
        } else {
            if (mutex->lista_Procesos_Esperando.primero == NULL) {
                printf("No recursivo, locked, no esperas\n");
                mutex->estado = UNLOCKED;
                fijar_nivel_int(nivel);
                return 0;
            } else {
                printf("No Recursivo y locked\n");
                mutex->contadorProcesos--;
                BCPptr procesoLiberado = mutex->lista_Procesos_Esperando.primero;
                procesoLiberado->estado = LISTO;
                eliminar_primero(&mutex->lista_Procesos_Esperando);
                insertar_ultimo(&lista_listos, procesoLiberado);
                mutex->proceso = procesoLiberado;
                fijar_nivel_int(nivel);
                return 0;
            }
        }
    } else {
        printf("UNLOCK DE %d\n", mutex->id);
        if (mutex->estado == UNLOCKED) {
            printf("IF 4\n");
            fijar_nivel_int(nivel);
            return -4;
        }
        if (mutex->bloqueos > 1) {
            mutex->bloqueos--;
            fijar_nivel_int(nivel);
            return 0;
        } else {
            mutex->bloqueos = 0;
            if (mutex->lista_Procesos_Esperando.primero == NULL) {
                printf("NO HAY NADIE ESPERANDO\n");
                mutex->estado = UNLOCKED;
                fijar_nivel_int(nivel);
                return 0;
            } else {
                mutex->contadorProcesos--;
                printf("HAY ALGUIEN ESPERANDO...............\n");
                BCPptr procesoLiberado = (mutex->lista_Procesos_Esperando).primero;
                eliminar_primero(&(mutex->lista_Procesos_Esperando));
                procesoLiberado->estado = LISTO;

                insertar_ultimo(&lista_listos, procesoLiberado);

                int num = 0;
                BCPptr m = lista_listos.primero;
                while(m != NULL){
                    num++;
                    m = m->siguiente;
                }
                printf("HAY %d PROCESOS LISTOS\n", num);
                fijar_nivel_int(nivel);
                return 0;
            }
        }

    }


}

int sis_cerrar_mutex() {
    int nivel = fijar_nivel_int(1);
    unsigned int mutexId = (unsigned int) leer_registro(1);
    int i, encontrado = 0, posicion;
    Mutexptr mutex = NULL;


  int lista = 0;
    for(i = 0; i < NUM_MUT; i++){
        if(lista_mutex[i] != NULL)lista++;
    }
    printf("Cerrar. %d\n", lista);
    // Buscar el mutex
    for (i = 0; i < NUM_MUT && !encontrado; i++) {
        if (lista_mutex[i] != NULL) {
            if (lista_mutex[i]->id == mutexId) {
                encontrado = 1;
                mutex = lista_mutex[i];
            }
        }
    }
    if (!encontrado) {                // Si no lo encuentra finaliza con error
        fijar_nivel_int(nivel);
        return -1;
    }

    while (mutex->estado == BLOQUEADO) {  // Si mutex bloqueado, desbloquearlo
        escribir_registro(1, mutex->id);
        sis_unlock(mutex->id);
    }


    if (mutex->contadorProcesos == 0) {
        encontrado = 0;
        int posicion = 0;
        for (i = 0; i < NUM_MUT && !encontrado; i++) {
            if (lista_mutex[i] != NULL) {
                if (lista_mutex[i]->id == mutexId) {
                    encontrado = 1;
                    posicion = i;
                }
            }
        }

        free(lista_mutex[posicion]);
        lista_mutex[posicion] = NULL;
    }

    encontrado = 0;
    for(i = 0; i < NUM_MUT_PROC && !encontrado; i++){
        if(p_proc_actual->descriptoresMutex[i] == mutexId)encontrado = 1;
        posicion = i;
    }
    p_proc_actual->descriptoresMutex[posicion] = -1;


    return 0;
}


int sis_leer_caracter() {
    char a;
    scanf("%c", &a);
    return a;
}

void lista_mutex_init() {
    int i;
    for (i = 0; i < NUM_MUT; i++) {
        lista_mutex[i] = NULL;
    }
}


/*
 *
 * Rutina de inicializaci�n invocada en arranque
 *
 */
int main() {
    /* se llega con las interrupciones prohibidas */
    lista_mutex_init(); //TODO Inicializar lista de mutex del sistema

    instal_man_int(EXC_ARITM, exc_arit);
    instal_man_int(EXC_MEM, exc_mem);
    instal_man_int(INT_RELOJ, int_reloj);
    instal_man_int(INT_TERMINAL, int_terminal);
    instal_man_int(LLAM_SIS, tratar_llamsis);
    instal_man_int(INT_SW, int_sw);


    iniciar_cont_int();        /* inicia cont. interr. */
    iniciar_cont_reloj(TICK);    /* fija frecuencia del reloj */
    iniciar_cont_teclado();        /* inici cont. teclado */

    iniciar_tabla_proc();        /* inicia BCPs de tabla de procesos */

    /* crea proceso inicial */
    if (crear_tarea((void *) "init") < 0)
        panico("no encontrado el proceso inicial");

    /* activa proceso inicial */
    p_proc_actual = planificador();
    cambio_contexto(NULL, &(p_proc_actual->contexto_regs));
    panico("S.O. reactivado inesperadamente");
    return 0;
}
