
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

static int _loaded = 0;

void __attribute__((__visibility__("default"))) loaded()
{
	_loaded = 1;
	printf("Yeah!\n");
}

int main(int argc, char* argv[])
{
	void* handle;

	if (argc < 2) {
	
		printf("Need a library .so to load!\n");
		return EXIT_FAILURE;
	}

	handle = dlopen(argv[1], RTLD_LAZY);
	if (handle) {

		printf("Library %s loaded\n", argv[1]);

		if (_loaded) {
			printf("Library %s constructed\n", argv[1]);
		}
		else {
			printf("[WARNING] Library %s not constructed!\n", argv[1]);
		}
	
		dlclose(handle);
	}
	else {
		printf("Could not load library %s\n", argv[1]);
	}

	return EXIT_SUCCESS;
}
