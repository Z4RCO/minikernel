### Minikernel de Sistema Operativo
Partiendo de un sistema de arranque, se ha desarrollado en C un minikernel con capacidad para gestionar procesos tanto gestión FIFO como con gestión de Round.Robin
Se ha imlpementado una llamada al sistema Sleep(), que permite a los procesos detenerse durante un cierto periodo de tiempo
También incluye un sistema de exclusión mutua Mutex, que permite a los procesos bloquearse utilizando llamadas y estructuras mutex para acceder a recursos compartidos.

## Características
- Syscall Sleep
- Syscalls y estructuras mutex
- Gestión de Procesos FIFO
- Gestión de Procesos Round-Robin
