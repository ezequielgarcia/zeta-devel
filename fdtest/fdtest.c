
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#define FILENAME "test"

void main()
{
	printf("Este proceso tiene pid %d\n", getpid());

	int fd = open(FILENAME, O_RDONLY);
	int size = lseek(fd, 0, SEEK_END);
	printf("Arcivo '%s' abierto. Ver enlace '/proc/%d/%d'\n", FILENAME, getpid(), fd);

	printf("Borramos el archivo? Despues el enlace 'proc/%d/%d' apunta a (deleted)\n", getpid(), fd);
	getchar();
	unlink(FILENAME);

	printf("El kernel todavia no elimina el inodo. Lo podemos verificar leyendo el contenido del fd %d\n", fd);
	char* p = (char*)malloc(size);
	getchar();
	read(fd, p, size);
	write(STDOUT_FILENO, p, size);

	printf("Cerramos el archivo? Despues ya no exisitira el enlace '/proc/%d/%d'\n", getpid(), fd);
	getchar();
	close(fd);

	printf("Enter para salir.\n");
	getchar();
}
