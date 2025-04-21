// Name: Hailey Mendenhall

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements.

// You can, and probably SHOULD, use a lot of your work from assignment 1 to
// get started on this assignment. In particular, when you get to the part
// about processing the selected file, you'll need to load and parse all of the
// data from the selected CSV file into your program. In assignment 1, you
// created structure types, linked lists, and a bunch of code to facilitate
// doing exactly that. Just copy and paste that work into here as a starting
// point :)

// This assignment has no requirements about linked lists, so you can
// use a dynamic array (and expand it with realloc(), for example) instead if
// you would like. But then you'd have to redo a bunch of work.

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct movie {
	char* title; // Dynamically allocated, support any size name
    int year;
	char** languages; 
    int num_languages;
    double rating;
};

struct node {
    struct movie* data;
    struct node* next;
};

void free_movie(struct movie* m) {
	free(m->title);
    int i;
    for(i = 0; i < m->num_languages; i++){
        free(m->languages[i]);
    }          
    free(m->languages);
    free(m);        
	
}

void free_list(struct node* head){
    struct node* temp;
    while (head != NULL){
        temp = head;
        head = head->next;
        free_movie(temp->data);
        free(temp);
    }
}

char** parse_languages(char* languages_field, int* num_languages){
    char* temp = strdup(languages_field + 1);
    temp[strlen(temp) - 1] = '\0';
    char* buffer;
    char** languages = NULL;
    *num_languages = 0;
    char* token = strtok_r(temp, ";", &buffer);
    while(token != NULL){
        (*num_languages)++;
        languages = realloc(languages, sizeof(char*) * (*num_languages));
        languages[(*num_languages) - 1] = strdup(token);
        token = strtok_r(NULL, ";", &buffer);
    }
    free(temp);
    return languages;
}

struct movie* create_movie(char* line){
    struct movie* m = malloc(sizeof(struct movie));
    char* token;

    token = strtok(line, ",");
    m->title = strdup(token);

    token = strtok(NULL, ",");
    m->year = strtol(token, NULL, 10);

    token = strtok(NULL, ",");
    m->languages = parse_languages(token, &(m->num_languages));

    token = strtok(NULL, ",");
    m->rating = strtod(token, NULL);

    return m;
}

struct node* add_node(struct node* head, struct movie* movie_data){
    struct node* new_node = malloc(sizeof(struct node));
    new_node->data = movie_data;
    new_node->next = NULL;

    if(head == NULL){
        return new_node;
    }

    struct node* temp = head;
    while(temp->next != NULL){
        temp = temp->next;
    }
    temp->next = new_node;

    return head;
}

char* create_movie_directory(const char* onid){
	srand(time(NULL));
	int random_number = rand() % 100000;
	char* directory_name = malloc(256 * sizeof(char));

	sprintf(directory_name, "%s.movies.%d", onid, random_number);

	if(mkdir(directory_name, 0777) == -1){
		perror("Error creating directory");
		free(directory_name);
		return NULL;
	}	

	printf("Created directory with name %s\n", directory_name);
	return directory_name;
}

void process_file(const char* filename){
	char* onid = "mendenhh";
	char* directory = create_movie_directory(onid);
	if(directory){
		FILE* file = fopen(filename, "r");
        if (file == NULL) {
            printf("Error opening the file: %s\n", filename);
            free(directory);
            return;
        }

        struct movie* movie_data = NULL;
        struct node* head = NULL;
        char line[1024];
		fgets(line, sizeof(line), file);

        // Assuming the file format is well-formed
        while (fgets(line, sizeof(line), file)) {
            movie_data = create_movie(line);
            head = add_node(head, movie_data);
        }

        fclose(file);

        // Process movies by year
        struct node* current = head;
        while (current != NULL) {
            struct movie* movie = current->data;

            // Create year-specific files
            char year_filename[256];
            sprintf(year_filename, "%s/%d.txt", directory, movie->year);

            FILE* year_file = fopen(year_filename, "a");
            if (year_file != NULL) {
                fprintf(year_file, "%s\n", movie->title);
                fclose(year_file);
            }

            current = current->next;
        }

        // Free memory
        free_list(head);
        free(directory);
    }
}
	



char* find_largest_file(){
	DIR* working_directory = opendir(".");
	if(!working_directory){
		printf("Could not open directory\n");
		return NULL;
	}

	struct dirent* entry;
	struct stat file_data;
	char* largest_file = NULL;
	int largest_size = 0;

	while((entry = readdir(working_directory)) != NULL){
		if(strncmp(entry->d_name, "movies", 6) == 0 && strstr(entry->d_name, ".csv")){
			if(stat(entry->d_name, &file_data) == 0){
				if(file_data.st_size > largest_size){
					largest_size = file_data.st_size;
					free(largest_file);
					largest_file = strdup(entry->d_name);	
				}
			}
		}	
	}
	closedir(working_directory);
	return largest_file;	
}

char* find_smallest_file(){
	DIR* working_directory = opendir(".");
	if(!working_directory){
		printf("Could not open directory\n");
		return NULL;
	}

	struct dirent* entry;
	struct stat file_data;
	char* smallest_file = NULL;
	int smallest_size = -1;

	while((entry = readdir(working_directory)) != NULL){
		if(strncmp(entry->d_name, "movies", 6) == 0 && strstr(entry->d_name, ".csv") != NULL){
			if(stat(entry->d_name, &file_data) == 0){
				if(smallest_size == -1 || file_data.st_size < smallest_size){					
					smallest_size = file_data.st_size;
					free(smallest_file);
					smallest_file = strdup(entry->d_name);	
				}
			}
		}	
	}
	closedir(working_directory);
	return smallest_file;
}

char* get_users_file(){
    printf("Enter the file name: ");
    char* filename = NULL;
    size_t len = 0;
    getline(&filename, &len, stdin);
    filename[strcspn(filename, "\n")] = '\0'; // Remove newline

    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("File not found.\n");
        free(filename);
        return NULL;
    }
    fclose(file);
    return filename;
}

void handle_decision(int file_choice){
	char* filename = NULL;
	if (file_choice == 1){
		filename = find_largest_file();
	} else if (file_choice == 2){
		filename = find_smallest_file();
	} else if (file_choice == 3){
		filename = get_users_file();
	}


	if(filename != NULL){
		printf("Now processing the chosen file named %s\n", filename);
		process_file(filename);
		free(filename);
	}else{
		printf("No valid file selected.\n");
	}
}

int process_file_menu(){
	printf("\n");
	printf("Which file do you want to process? \n");
    printf("Enter 1 to pick the largest file\n");    
    printf("Enter 2 to pick the smallest file\n");
    printf("Enter 3 to specifify the name of a file \n");
	printf("Enter a choice from 1 to 3: ");

	char* decision = NULL;
	size_t n = 0;
	getline(&decision, &n, stdin);

	printf("\n");
	int choice = strtol(decision, NULL, 10);
	free(decision);
    return choice;
}

int main_menu(){
	printf("\n");
    printf("1. Select file to process\n");    
    printf("2. Exit from the program\n");
    printf("Please enter a choice 1 or 2: ");

	char* decision = NULL;
	size_t n = 0;
	getline(&decision, &n, stdin);

	printf("\n");
	int choice = strtol(decision, NULL, 10);
	free(decision);
    return choice; 
}

int main() {
	umask(0027);
	int user_choice = 0;
	int file_choice = 0;
	while(user_choice != 2){
		user_choice = main_menu();
		if(user_choice == 1){
			file_choice = process_file_menu();
			handle_decision(file_choice);
		}else {
			printf("Goodbye! Thank you for using my program :)\n");
        }    		
	}	

	return 0;
}
