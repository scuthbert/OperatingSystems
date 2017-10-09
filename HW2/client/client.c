#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

int device;

int main() {

	device = open("/dev/simple_character_device", O_RDWR);
	
	int running = 1;
	while(running) {
		
		printf("Press r to read, w to write, s to seek, or e to exit:\n");
		
		char command;
		command = getchar();	

		char *data;
		data = malloc(128 * sizeof(char));
		
	
		switch(command)
		{
			case 'r':
				read(device, data, 128);
				printf("%s\n", data);
				break;
			case 'w':
				*data = "testtesttesttesttest";
				write(device, data, 128); 
				break;
			case 's':
				printf("%s\n", &command);
				break;
			case 'e':
				running = 0;
				break;

		} 

	}

	close(device);

	return 0;
}
