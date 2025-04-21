#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

int main(int argc, const char* argv[]) {
    //check if there are not enough arguments
    if(argc < 2) {
        fprintf(stderr, "Usage: keygen keylength\n");
        exit(1);
    }

    srand(time(NULL));
    char* endptr;
    int length = strtol(argv[1], &endptr, 10);
    //throw error if tries to make a negative key length
    if(*endptr != '\0' || length < 1) {
        fprintf(stderr, "Key length must be a positive integer\n");
        exit(1);
    }
    
    //generate a random key of the given length
    char key[length + 1];
    int i;
    for (i = 0; i < length; i++) {
        int r = rand() % 27;
        if(r == 26) {
            key[i] = ' ';
        } else {
            key[i] = r + 65;
        }
    }
    key[length] = '\0';
    printf("%s\n", key);

    return 0;	
}
