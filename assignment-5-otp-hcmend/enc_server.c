#include <arpa/inet.h>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

//function to encrypt the plaintext with the key
void encrypt(char* plaintext, char* key, char* ciphertext) {
    int i;
    for (i = 0; i < strlen(plaintext); i++) {    
        int pt_val; 
        int key_val;   
        //check if the character is a letter or a space and assign it to the correct integer value for encryption
        if(plaintext[i] >= 'A' && plaintext[i] <= 'Z'){
            pt_val = plaintext[i] - 'A';
        }else if(plaintext[i] == ' '){
            pt_val = 26;
        }
        //same for key
       if(key[i] >= 'A' && key[i] <= 'Z'){
            key_val = key[i] - 'A';
        }else if(key[i] == ' '){
            key_val = 26;
        }
        //encrypt the plaintext with the key
        int ct_val = (pt_val + key_val) % 27;
        if(ct_val == 26){
            ciphertext[i] = ' ';
        }else{
            ciphertext[i] = ct_val + 'A'; 
        }             
    }
    ciphertext[i] = '\0';
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <listening_port>\n", argv[0]);
        exit(1);
    }

    //create a socket
    int listen_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_fd < 0) {
        fprintf(stderr, "Error opening socket\n");
        exit(1);
    }

    //bind the socket to the port
    struct sockaddr_in bind_addr;
    bind_addr.sin_family = AF_INET;
    bind_addr.sin_addr.s_addr = INADDR_ANY;
    bind_addr.sin_port = htons(atoi(argv[1]));    
    int bind_result = bind(listen_socket_fd, (struct sockaddr*)&bind_addr, sizeof(bind_addr));
    //check if the bind was successful
    if (bind_result == -1) {
        fprintf(stderr, "Error on bind\n");
        exit(1);
    }
    //listen for incoming connections
    int listen_result = listen(listen_socket_fd, 8);
    //check if the listen was successful
    if (listen_result == -1) {
        fprintf(stderr, "Error on listen\n");
        exit(1);
    }

    //create an array to store the child pids
    pid_t child_pids[300]; 
    size_t n_child_pids = 0;
    while (1) {
        //accept incoming connections
        struct sockaddr_in client_addr;
        socklen_t client_addr_size = sizeof(client_addr);

        pid_t temp_child_pids[300];
        size_t temp_n_child_pids = 0;
        int i;
        for (i = 0; i < n_child_pids; i++){
            int exit_status;
            int wait_result = waitpid(child_pids[i], &exit_status, WNOHANG);
            if(!wait_result){
                temp_child_pids[temp_n_child_pids] = child_pids[i];
                temp_n_child_pids++;    
            }
        }

        for(i = 0; i < temp_n_child_pids; i++){
            child_pids[i] = temp_child_pids[i];
        }
        n_child_pids = temp_n_child_pids;

        //accept the connection
        int communication_socket_fd = accept(listen_socket_fd, (struct sockaddr*)&client_addr, &client_addr_size);
        if (communication_socket_fd < 0) {
            fprintf(stderr, "Error on accept\n");
            return 1;
        }
        
        //fork a child process to handle the connection
        pid_t fork_result = fork();
       
        
        if (fork_result == 0) {  
            //this is the child process now
            //send 'e' to the client        
            int n = send(communication_socket_fd, "e", 1, 0);
            if(n < 0){
                fprintf(stderr, "Error sending 'e' to client\n");
                close(communication_socket_fd);
                return 1;
            }
            //make sure it got 'e' back
            char received_char;
            n = recv(communication_socket_fd, &received_char, 1, 0);
            if(n < 0 || received_char != 'e'){
                fprintf(stderr, "Error: client type mismatch\n");
                close(communication_socket_fd);
                return 1;
            }

            // Receive the length of the message it is going to get
            int message_length;
            int total_bytes_received = 0;
            while (total_bytes_received < sizeof(message_length)) {
                n = recv(communication_socket_fd, ((char*)&message_length) + total_bytes_received, sizeof(message_length) - total_bytes_received, 0);
                if (n < 0) {
                    fprintf(stderr, "Error receiving message length from client\n");
                    close(communication_socket_fd);
                    return 1;
                }
                total_bytes_received += n;
            }

            // Allocate memory for the full message
            char* full_message = malloc(message_length + 1);
            if (full_message == NULL) {
                fprintf(stderr, "Error allocating memory for full_message\n");
                close(communication_socket_fd);
                return 1;
            }

            // Receive the full message
            total_bytes_received = 0;
            while (total_bytes_received < message_length) {
                n = recv(communication_socket_fd, full_message + total_bytes_received, message_length - total_bytes_received, 0);
                if (n < 0) {
                    fprintf(stderr, "Error receiving full_message from client\n");
                    free(full_message);
                    close(communication_socket_fd);
                    return 1;
                }
                total_bytes_received += n;
            }
            full_message[message_length] = '\0';

            // Get plaintext and key from full_message
            char* buffer;
            char* plaintext = strtok_r(full_message, "$$", &buffer);
            char* key = strtok_r(NULL, "@@", &buffer);

            // Encrypt plaintext with key
            char* ciphertext = malloc(strlen(plaintext) + 1);
            if (ciphertext == NULL) {
                fprintf(stderr, "Error allocating memory for ciphertext\n");
                free(full_message);
                close(communication_socket_fd);
                return 1;
            }
            encrypt(plaintext, key, ciphertext);

            // Send ciphertext to client      
            int total_bytes_to_send = strlen(ciphertext);
            int total_bytes_sent = 0;
            while (total_bytes_sent < total_bytes_to_send) {
                //sends ciphertext in loop so it is all sent
                int bytes_sent = send(communication_socket_fd, ciphertext + total_bytes_sent, total_bytes_to_send - total_bytes_sent, 0);
                if (bytes_sent < 0) {
                    fprintf(stderr, "Error sending ciphertext to client\n");
                    free(full_message);
                    free(ciphertext);
                    //close socket
                    close(communication_socket_fd);
                    return 1;
                }
                total_bytes_sent += bytes_sent;
            }

            free(full_message);
            free(ciphertext);
            shutdown(communication_socket_fd, SHUT_RDWR);
            close(communication_socket_fd);
            exit(0);
        }
        //close the socket
        close(communication_socket_fd);
        child_pids[n_child_pids] = fork_result;
        n_child_pids++;
    }
}

