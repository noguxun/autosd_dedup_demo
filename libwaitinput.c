# include <stdio.h>
# include <stdlib.h>
# include <string.h>

void my_wait(char *s) {
	printf("\nPress '%s' followed by Enter to exit: ", s);
	char input[10];
	while (1) {
		if (fgets(input, sizeof(input), stdin) != NULL) {
			// Remove newline character if present
			size_t len = strlen(input);
			if (len > 0 && input[len-1] == '\n') {
				input[len-1] = '\0';
			}
			
			if (strncmp(input, s, 10) == 0) {
				printf("Exiting...\n");
				break;
			} else {
				printf("Type '%s' to exit: ", s);
			}
		}
	}
}	// Wait for user input "end" before exiting
