
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FILENAME "test"

void main()
{
	printf("Este proceso tiene pid %d.\n", getpid());

	// Abrir y obtener tama√o
	int fd = open(FILENAME, O_RDWR);
	int size = lseek(fd, 0, SEEK_END);

	printf("Arcivo '%s' abierto. Ver enlace '/proc/%d/%d'.\n", FILENAME, getpid(), fd);

	// Mapeo necesario para leer el inodo, por asi decirlo.
	void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);       

	printf("Ahora obtuvimos un mapeo del fd %d en la direccion %p de este proceso. Mas info en 'proc/%d/maps'.\n",fd, p, getpid());
	printf("Borramos el archivo? Despues el enlace 'proc/%d/%d' apuntara a algo 'deleted'.\n", getpid(), fd);
	getchar();

	// El archivo se puede borrar a pesar de estar abierto y mapeado como RW !!!
	unlink(FILENAME);

	printf("Una vez borrado el archivo, el fd no puede usarse para leer el mismo. Sin embargo, el mapeo si nos sirve.\n");
	printf("El kernel todavia no elimina el inodo. Lo podemos verificar escribiendo el mapeo a stdout.\n");
	getchar();

	// Podemos ver el contenido del archivo, seguramente tambien podemos modificarlo
	write(STDOUT_FILENO, p, size);

	printf("Cerramos el archivo? Despues ya no exisitira el enlace '/proc/%d/%d'.\n", getpid(), fd);
	getchar();

	// Cerramos el fd, el kernel aun no borra el inodo porque todavia existe el mapeo.
	// Este mapeo es un mecanismo de kernel, los datos NO residen en memoria sino en el inodo!
	// Obviamente, podrian eventualmente estar cacheados en memoria.
	close(fd);

	// Listo ahora el inodo se pierde pues ya no hay mas referencias a el.
	munmap(p, size);

	printf("Enter para salir.\n");
	getchar();
}
