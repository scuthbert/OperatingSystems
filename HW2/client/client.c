#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
	
	int device = open("/dev/simple_character_device", O_RDWR);

	close(device);

	return 0;
}
