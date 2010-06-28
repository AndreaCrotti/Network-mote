#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#define HOSTS "known_hosts"

int main(int argc, char* argv) {
//[[
	FILE *hosts = fopen(HOSTS, "r");

	printf("\nReading known hosts from file: %s \n\n", HOSTS);

	if(hosts == NULL) {
		printf("fopen(%s): %d\n", HOSTS, errno);
		return -1;
	}

	printf("Staying alive\n");
		while(!feof(hosts)) {
		char line[AP_HOST_LINE_BUFFER];
		//line = malloc(1000);
		int matches = fscanf(hosts, "%100s\n", line);
		//printf("Matches %d\n", matches);
		printf("Line in hosts %s\n", line);

	}
	return 0;
}
