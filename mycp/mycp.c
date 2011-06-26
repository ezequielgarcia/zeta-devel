#include <stdio.h>
#include <stdlib.h>

#define BUFFSIZE	1024*32

#ifdef DEBUG
#define PDEBUG printf
#else
#define PDEBUG 
#endif

int main(int argc, char* argv[])
{
	FILE* fin;
	FILE* fout;
	void* buffer;
	size_t buffer_size = BUFFSIZE;
	size_t bytes_read, bytes_written;

	if (argc < 3) {
		printf("Not enough args. Usage %s SOURCE DEST [BUFFER SIZE]\n", argv[0]);
		return 1;
	}

	fin = fopen(argv[1], "rb");
	if (!fin) {
		printf("Can`t open %s for reading\n", argv[1]);	
		return 1;
	}

	fout = fopen(argv[2], "wb");
	if (!fout) {
		printf("Can`t open %s for writing\n", argv[2]);	
		return 1;
	}

	// Have buffer_size in args
	if (argc > 3) 
		buffer_size = atoi(argv[3]);
	
	buffer = malloc(buffer_size);	
	if (!buffer) {
		printf("Can`t allocate %zd bytes buffer\n", buffer_size);	
		return 1;
	}

	while (!feof(fin)) {

		PDEBUG("Reading %zd bytes from %s... ", buffer_size, argv[1]);
		bytes_read = fread(buffer, 1, buffer_size, fin);
		PDEBUG("%zd bytes\n", bytes_read);

		if (bytes_read > 0) {

			PDEBUG("Writing %zd bytes to %s...", bytes_read, argv[2]);
			bytes_written = fwrite(buffer, 1, bytes_read, fout);
			PDEBUG("%zd bytes\n", bytes_written);

			if (bytes_written != bytes_read) {
				printf("oops!\n");
				fclose(fin);
				fclose(fout);
				return 1;
			}
		}
		else {
			if (ferror(fin)) {
				printf("Error reading file\n");
				fclose(fin);
				fclose(fout);
				return 1;
			}
		}
	}

	fclose(fin);
	fclose(fout);

	return 0;
}
