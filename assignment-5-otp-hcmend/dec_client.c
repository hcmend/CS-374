#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

//function to validate everything in string is capital letter or space
void validate_content(const char* content) {
    int i;
    for (i = 0; i < strlen(content); i++) {
        if ((content[i] < 'A' || content[i] > 'Z') && content[i] != ' ') {
            fprintf(stderr, "Error: invalid character in input\n");
            exit(1);
        }
    }
}

//function to strip newline characters from a string
void strip_newline(char* str) {
    size_t len = strlen(str);
    if (len > 0 && str[len - 1] == '\n') {
        str[len - 1] = '\0';
    }
}

int main(int argc, char* argv[]) {
    //make sure there are enough arguments
    if (argc < 4) {
        fprintf(stderr, "Not enough arguments!\n");
        exit(1);
    }

    //open ciphertext file
    FILE* ciphertext_file = fopen(argv[1], "r");
    if(ciphertext_file == NULL) {
        fprintf(stderr, "Error opening ciphertext file\n");
        exit(1);
    }

    //read ciphertext from file
    char* ciphertext = NULL;
    size_t ciphertext_len = 0;
    if(getline(&ciphertext, &ciphertext_len, ciphertext_file) == -1){
        fprintf(stderr, "Error reading ciphertext file\n");
        fclose(ciphertext_file);
        exit(1);
    }
    fclose(ciphertext_file);
    strip_newline(ciphertext);
    validate_content(ciphertext);

    //open key file
    FILE* key_file = fopen(argv[2], "r");
    if(key_file == NULL){
        fprintf(stderr, "Error opening key file\n");        
        exit(1);
    }

    //read key from file
    char* key = NULL;
    size_t key_len = 0;
    if(getline(&key, &key_len, key_file) == -1){
        fprintf(stderr, "Error reading key file\n");
        fclose(key_file);
        exit(1);
    }
    fclose(key_file);
    strip_newline(key);
    validate_content(key);
    
    //ensure key is at least as long as the ciphertext
    if(strlen(key) < strlen(ciphertext)){
        fprintf(stderr, "Error: key is shorter than ciphertext\n");
        exit(1);
    }

    //create a socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_fd < 0) {
        fprintf(stderr, "Error opening socket\n");
        exit(1);
    }

    //get address info
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    struct addrinfo* server_addr_list = NULL;
    int getaddrinfo_result = getaddrinfo("localhost", argv[3], &hints, &server_addr_list);
    if(getaddrinfo_result != 0){
        fprintf(stderr, "Error getting address info\n");
        exit(1);
    }
    struct addrinfo* itr = server_addr_list;
    int connect_result = -1;
    while(itr != NULL){
        //connect to the server
        connect_result = connect(socket_fd, itr->ai_addr, itr->ai_addrlen);
        //if connected, break out of loop
        if(connect_result == 0){
            break;
        }
        itr = itr->ai_next;
    }

    //check if connection was successful
    if(connect_result != 0){
        fprintf(stderr, "Error connecting to server\n");
        exit(1);
    }

    //send handshake character to server
    int n = send(socket_fd, "d", 1, 0);
    if(n < 0){
        fprintf(stderr, "Error sending 'd' to server\n");
        exit(1);
    }

    //receive handshake character from server
    char received_char;
    n = recv(socket_fd, &received_char, 1, 0);
    if(n < 0 || received_char != 'd'){
        fprintf(stderr, "Error: server type mismatch\n");
        exit(1);
    }

    //prepare message to send to server
    char* message = malloc(strlen(ciphertext) + strlen(key) + 5);
    if(message == NULL){
        fprintf(stderr, "Error allocating memory for message\n");
        exit(1);
    }
    //format message with delimeters so the server can parse it
    sprintf(message, "%s$$%s@@", ciphertext, key);

    // Send the length of the message
    int message_length = strlen(message);
    int total_bytes_sent = 0;
    while (total_bytes_sent < sizeof(message_length)) {
        n = send(socket_fd, ((char*)&message_length) + total_bytes_sent, sizeof(message_length) - total_bytes_sent, 0);
        if (n < 0) {
            fprintf(stderr, "Error sending message length to server\n");
            exit(1);
        }
        total_bytes_sent += n;
    }

    // Send the actual message
    int total_bytes_to_send = message_length;
    total_bytes_sent = 0;
    while (total_bytes_sent < total_bytes_to_send) {
        int bytes_sent = send(socket_fd, message + total_bytes_sent, total_bytes_to_send - total_bytes_sent, 0);
        if (bytes_sent < 0) {
            fprintf(stderr, "Error sending message to server\n");
            exit(1);
        }
        total_bytes_sent += bytes_sent;
    }
    
    //receive plaintext from server
    char* plaintext = malloc(strlen(ciphertext) + 1);
    if(plaintext == NULL){
        fprintf(stderr, "Error allocating memory for plaintext\n");
        exit(1);
    }
    int bytes_to_receive = strlen(ciphertext);
    int total_bytes_received = 0;
    while (total_bytes_received < bytes_to_receive) {
        int bytes_read = recv(socket_fd, plaintext + total_bytes_received, bytes_to_receive - total_bytes_received, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "Error receiving plaintext from server\n");
            exit(1);
        }
        total_bytes_received += bytes_read;
    }

    //print plaintext to stdout

    printf("%s\n", plaintext);
    free(ciphertext);
    free(key);
    free(message);
    free(plaintext);
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);    
    


}
