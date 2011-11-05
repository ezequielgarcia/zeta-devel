
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#define FILENAME "test"

void main()
{
	printf("Este proceso tiene pid %d\n", getpid());

	int fd = open(FILENAME, O_RDWR);
	int size = lseek(fd, 0, SEEK_END);
	printf("Arcivo '%s' abierto. Ver enlace '/proc/%d/%d'\n", FILENAME, getpid(), fd);

	void* p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);       
	printf("Ahora obtuvimos un mapeo del fd %d en la direccion %p de este proceso.\n",fd, p);

	printf("Borramos el archivo? Despues el enlace 'proc/%d/%d' apunta a (deleted)\n", getpid(), fd);
	getchar();
	unlink(FILENAME);

	printf("Una vez borrado el archivo, el fd no puede usarse para leer el mismo. Sin embargo, el mapeo si nos sirve.\n");

	printf("El kernel todavia no elimina el inodo. Lo podemos verificar escribiendo el mapeo a stdout\n");
	getchar();
	write(STDOUT_FILENO, p, size);

	printf("Cerramos el archivo? Despues ya no exisitira el enlace '/proc/%d/%d'\n", getpid(), fd);
	getchar();
	close(fd);

	printf("Enter para salir.\n");
	getchar();
}
