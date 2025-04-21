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
    if (len > 0 && str[len - 1] == '\n' || str[len - 1] == '\r') {
        str[len - 1] = '\0';
    }
}

int main(int argc, char* argv[]) {
    //make sure there are enough arguments
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <plaintext> <key> <port>\n", argv[0]);
        exit(1);
    }

    //open plaintext file
    FILE* plaintext_file = fopen(argv[1], "r");
    if (plaintext_file == NULL) {
        fprintf(stderr, "Error opening plaintext file\n");
        exit(1);
    }

    //read plaintext from file
    char* plaintext = NULL;
    size_t plaintext_len = 0;
    if (getline(&plaintext, &plaintext_len, plaintext_file) == -1) {
        fprintf(stderr, "Error reading plaintext file\n");
        fclose(plaintext_file);
        exit(1);
    }
    fclose(plaintext_file);
    strip_newline(plaintext);
    validate_content(plaintext);

    //open key file
    FILE* key_file = fopen(argv[2], "r");
    if (key_file == NULL) {
        fprintf(stderr, "Error opening key file\n");
        exit(1);
    }

    //read key from file
    char* key = NULL;
    size_t key_len = 0;
    if (getline(&key, &key_len, key_file) == -1) {
        fprintf(stderr, "Error reading key file\n");
        fclose(key_file);
        exit(1);
    }
    fclose(key_file);
    strip_newline(key);
    validate_content(key);

    //ensure key is at least as long as the plaintext
    if (strlen(key) < strlen(plaintext)) {
        fprintf(stderr, "Error: key is shorter than plaintext\n");
        exit(1);
    }

    //create a socket
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "Error opening socket\n");
        exit(1);
    }

    //set up the server address
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    struct addrinfo* server_addr_list = NULL;
    int getaddrinfo_result = getaddrinfo("localhost", argv[3], &hints, &server_addr_list);
    if (getaddrinfo_result != 0) {
        fprintf(stderr, "Error getting address info\n");
        exit(1);
    }

    //coneect to the server
    struct addrinfo* itr = server_addr_list;
    int connect_result = -1;
    while (itr != NULL) {
        connect_result = connect(socket_fd, itr->ai_addr, itr->ai_addrlen);
        //if the connection is successful, break out of the loop
        if (connect_result == 0) {
            break;
        }
        itr = itr->ai_next;
    }

    if (connect_result != 0) {
        fprintf(stderr, "Error connecting to server\n");
        exit(1);
    }    

    freeaddrinfo(server_addr_list);

    //send handshake character to server
    int n = send(socket_fd, "e", 1, 0);
    if (n < 0) {
        fprintf(stderr, "Error sending 'e' to server\n");
        exit(1);
    }

    //receive handshake character from server
    char received_char;
    n = recv(socket_fd, &received_char, 1, 0);
    if (n < 0 || received_char != 'e') {
        fprintf(stderr, "Error: server type mismatch\n");
        exit(1);
    }

    //prepare message to send to server
    char* message = malloc(strlen(plaintext) + strlen(key) + 5);
    if (message == NULL) {
        fprintf(stderr, "Error allocating memory for message\n");
        exit(1);
    }

    //format message with delimeters so the server can parse it
    sprintf(message, "%s$$%s@@", plaintext, key);

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

    //receive the ciphertext from the server
    char* ciphertext = malloc(strlen(plaintext) + 1);
    if (ciphertext == NULL) {
        fprintf(stderr, "Error allocating memory for ciphertext\n");
        exit(1);
    }
    int bytes_to_receive = strlen(plaintext);
    int total_bytes_received = 0;
    while (total_bytes_received < bytes_to_receive) {
        int bytes_read = recv(socket_fd, ciphertext + total_bytes_received, bytes_to_receive - total_bytes_received, 0);
        if (bytes_read < 0) {
            fprintf(stderr, "Error receiving ciphertext from server\n");
            exit(1);
        }
        total_bytes_received += bytes_read;
    }

    //print the recieved ciphertext
    printf("%s\n", ciphertext);

    //clean up
    free(plaintext);
    free(key);
    free(message);
    free(ciphertext);
    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);
    
}
