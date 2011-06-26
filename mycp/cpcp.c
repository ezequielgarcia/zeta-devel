#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define CHUNKSIZE		4096

#define CIRCULARSIZE	CHUNKSIZE*1000;

#ifdef DEBUG
#define PDEBUG printf
#else
#define PDEBUG
#endif

// Circular shared buffer
static void* circular = NULL;
static size_t circular_size = CIRCULARSIZE;
static size_t rpos = 0;
static size_t wpos = 0;

char* file_name_in;
char* file_name_out;

int done = 0;

static
void produce(void* src, size_t len)
{
	size_t cont_space;

	// Tengo suficiente lugar contiguo?
	cont_space = circular_size - rpos;
	if (cont_space >= len) {

		// Copio en un solo bloque
		memcpy(circular+rpos, src, len);

		// Muevo posicion
		rpos += len;
		if (rpos == circular_size)
			rpos = 0;
	}
	else {
		// Copio hasta el final del buffer
		memcpy(circular+rpos, src, cont_space);
		// Termino de copiar desde el principio
		memcpy(circular, src+cont_space, len-cont_space);	
		rpos = len - cont_space;
	}
}

static
size_t getfreespace()
{
	return (rpos < wpos) ? (wpos-rpos) : (wpos + (circular_size - rpos)); 
}

static
void* producer(void* arg) 
{
	FILE* fin;
	void* chunk;
	size_t chunk_size = CHUNKSIZE;
	size_t bytes_read;

	fin = fopen(file_name_in, "rb");
	if (!fin) {
		printf("Can`t open %s for reading\n", file_name_in);	
		fclose(fin);
		return NULL;
	}

	chunk = malloc(chunk_size);	
	if (!chunk) {
		printf("Can`t allocate %zd bytes buffer\n", chunk_size);	
		fclose(fin);
		return NULL;
	}

	while (!feof(fin)) {

		// Pedimos un byte más que lo que necesitamos, de lo contrario luego de escribir el buffer
		// tendríamos rpos == wpos, que es la situacion de buffer vacio.
		while (getfreespace() < (chunk_size+1)) {
			// Significa que el buffer esta casi lleno
			PDEBUG("[P] Buffer full, waiting for consumer...\n");
			usleep(1000);
		}

		PDEBUG("[P] Reading %zd bytes from %s...\n", chunk_size, file_name_in);
		bytes_read = fread(chunk, 1, chunk_size, fin);
		PDEBUG("[P] ... read %zd bytes\n", bytes_read);

		if (bytes_read > 0) {
			produce(chunk, bytes_read);
		}
	}

	fclose(fin);
	return NULL;
} 

static
void* consumer(void* arg) 
{
	FILE* fout;
	void* chunk;
	size_t chunk_size = CHUNKSIZE;
	size_t bytes_written, filledspace, cont_space, bytes_write;

	fout = fopen(file_name_out, "wb");
	if (!fout) {
		printf("Can`t open %s for writing\n", file_name_out);	
		return NULL;
	}

	chunk = malloc(chunk_size);	
	if (!chunk) {
		printf("Can`t allocate %zd bytes buffer\n", chunk_size);	
		fclose(fout);
		return NULL;
	}

	while (1) {

		// Significa que el buffer esta vacio, el thread solo puede salir de aca
		if (wpos == rpos) {
			while (wpos==rpos && !done) {
				PDEBUG("[C] Buffer empty, waiting for producer...\n");
				usleep(1000);
			}

			if (done && wpos==rpos)
				break;
		}

		filledspace = (wpos<=rpos) ? (rpos-wpos) : (rpos + (circular_size-wpos));

		if (filledspace < chunk_size)
			bytes_write = filledspace;
		else
			bytes_write = chunk_size;
	
		// Tengo suficiente lugar contiguo?
		cont_space = circular_size - wpos;
		if (cont_space >= bytes_write) {
			// Copio en un solo bloque
			memcpy(chunk, circular+wpos, bytes_write);
			wpos += bytes_write;

			if (wpos == circular_size)
				wpos = 0;
		}
		else {
			// Copio hasta el final
			memcpy(chunk, circular+wpos, cont_space);
			// Termino de copiar desde el principio
			memcpy(chunk+cont_space, circular, bytes_write-cont_space);	
			wpos = bytes_write-cont_space;
		}

		PDEBUG("[C] Writing %zd bytes to %s...\n", bytes_write,  file_name_out);
		bytes_written = fwrite(chunk, 1, bytes_write, fout);
		PDEBUG("[C] ...   written %zd bytes\n", bytes_written);

		if (bytes_written != bytes_write) {
			printf("oops!\n");
			fclose(fout);
			return NULL;
		}
	}

	fclose(fout);
	return NULL;
} 

int main(int argc, char* argv[])
{
	pthread_t producer_h, consumer_h;

	if (argc < 3) {
		printf("Not enough args. Usage %s SOURCE DEST [BUFFER SIZE]\n", argv[0]);
		return 1;
	}

	file_name_in = (char*)argv[1];
	file_name_out = (char*)argv[2];

	// Alloc big shared buffer
	circular = malloc(circular_size);
	if (!circular) {
		printf("Can`t allocate %zd bytes buffer\n", circular_size);	
		return 1;
	}

	// Launch producer
	pthread_create(&producer_h, NULL, producer, NULL);

	// Launch consumer
	pthread_create(&consumer_h, NULL, consumer, NULL);

	// Wait for producer
	pthread_join(producer_h, NULL);

	printf("Producer terminated\n");

	// Notify consumer
	done = 1;

	// Wait for consumer
	pthread_join(consumer_h, NULL);

	printf("Consumer terminated\n");

	return 0;
}
