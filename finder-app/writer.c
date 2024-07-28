#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	
	FILE *file;
	
	openlog(NULL, LOG_PID, LOG_USER);
	
	if(argc != 3) {
		syslog(LOG_ERR, "Incorrect number of arguments: %d", argc);
		return 1;
	}
	
	const char *writefile = argv[1];
	const char *writestr = argv[2];

	file = fopen(writefile, "w");
	
	if(!file) {
		syslog(LOG_ERR, "Error opening the file: %s", writefile);
		return 1;
	}
	
	if(fprintf(file, "%s", writestr) < 0) {
		syslog(LOG_ERR, "Error writing to file: %s %s", writefile, writestr);
		fclose(file);
		return 1;
	}
	else {
		syslog(LOG_DEBUG, "Writing %s to %s", writefile, writestr);
	}
	
	if(fclose(file) != 0) {
		syslog(LOG_ERR, "Error closing the file: %s", writefile);
		return 1;
	}

	return 0;
}
