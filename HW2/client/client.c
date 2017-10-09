#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>

int device;

void clearStdin() {
	int c;
	while((c=getchar()) != '\n' && c != EOF) { }
}

void readFromDev() {
	clearStdin();

	int count;
	printf("Enter the number of bytes you want to read: ");
	scanf("%d", &count);	

	char *data;
	data = malloc(count * sizeof(char));
	int error = read(device, data, count);
	
	if(error < 0) printf("Error occured in read\n");
	else printf("Data read from device: %s\n", data);
	
	free(data);
	
	clearStdin();
	return;
}

void writeToDev() {
	size_t count = 1000; // Max size we will allow
	char *data;
	data = malloc(count * sizeof(char));
	
	clearStdin();

	printf("Enter the data you want to write to the device: ");
	size_t characters = getline(&data, &count, stdin);
	
	int error = write(device, data, characters-1);
	
	if(error < 0) printf("Error occured in read\n");
	else printf("Data written to device.\n");
	free(data);
	return;
}

void seekDev() {
	clearStdin();
	
	int offset;
	printf("Enter an offset value: ");
	scanf("%d", &offset);	

	int whence;	
	printf("Enter a value for whence (third parameter): ");
	scanf("%d", &whence);	

	int error = lseek(device, offset, whence);
	
	if(error < 0) printf("Error occured in seek.\n");
	else printf("Seek successfull.\n");
	
	clearStdin();
	return;
}

int main() {

	device = open("/dev/simple_character_device", O_RDWR);
	
	int running = 1;
	while(running) {
		
		printf("Press r to read, w to write, s to seek, or e to exit: ");

		char command;
		command = getchar();	

		switch(command)
		{
			case 'r':
				readFromDev();
				break;
			case 'w':
				writeToDev();
				break;
			case 's':
				seekDev();
				break;
			case 'e':
				running = 0;
				break;

		} 

	}

	close(device);

	return 0;
}
