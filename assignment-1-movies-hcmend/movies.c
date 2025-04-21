// Name: Hailey Mendenhall

// Complete this file to write a C program that adheres to the assignment
// specification. Refer to the assignment document in Canvas for the
// requirements. Refer to the example-directory/ contents for some example
// code to get you started.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

// Frees an array of people and their first_name and last_name character arrays
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

void print_menu(){
    printf("\n");
    printf("1. Show Movies released in the specified year\n");
    printf("2. Show highest rated movie for each year\n");
    printf("3. Show the title and year of release of all movies in a specific language\n");
    printf("4. Exit from the program\n");
    printf("\n");
    return;    
}

void print_year(struct node* head) {
    char* year_to_search = NULL;
    size_t n = 0;
    printf("Enter the year for which you want to see movies: ");
    getline(&year_to_search, &n, stdin);
    int int_year = strtol(year_to_search, NULL, 10);
    free(year_to_search);

    struct node* temp = head;
    int found = 0;

    while (temp != NULL) {
        struct movie* m = temp->data;
        if(m->year == int_year){
            found = 1;
            printf("%s\n", m->title);
        }
        temp = temp->next;    
    }
    if(!found){
        printf("No data about movies released in the year %d\n", int_year);
    }
}

void ratings_by_year(struct node* head){
    struct movie* highest_rated[2022] = {NULL};

    struct node* temp = head;
    while(temp != NULL){
        struct movie* m = temp->data;
        if(highest_rated[m->year] == NULL || m->rating > highest_rated[m->year]->rating){
            highest_rated[m->year] = m;
        }
        temp = temp->next;
    }
    int year;
    for(year = 1900; year <= 2021; year++){
        if(highest_rated[year] != NULL){
            printf("%d %.1lf %s \n", year, highest_rated[year]->rating, highest_rated[year]->title);
        }
    }
}

void print_language(struct node* head){
    char* language_to_search = NULL;
    size_t n = 0;
    printf("Enter the language for which you want to see movies: ");
    getline(&language_to_search, &n, stdin);
    language_to_search[strlen(language_to_search) - 1] = '\0';
    
    struct node* temp = head;
    int found = 0;

    while(temp != NULL){
        struct movie* m = temp->data;
        char** movie_languages = m->languages;
        int i;
        for(i = 0; i < m->num_languages; i++){
            if( strcmp(language_to_search, movie_languages[i]) == 0){
                printf("%d %s\n", m->year, m->title);
                found = 1;
                break;
            }            
        }
        temp = temp->next;
    }
    if(!found){
        printf("No data about movies released in %s\n", language_to_search);
    }
    free(language_to_search);
}

int main(int argc, char* argv[]){
    if (argc < 2){
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    FILE* file = fopen(argv[1], "r");
    if(file == NULL){
        fprintf(stderr, "Error: Could not open file %s\n", argv[1]);
        return 1;
    }

    struct node* head = NULL;
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    getline(&line, &len, file);

    int movie_count = 0;
    while((read = getline(&line, &len, file)) != -1){
        struct movie* m = create_movie(line);
        head = add_node(head, m);
        movie_count++;
    }
    free(line);
    fclose(file);

    printf("Processed file %s and parsed data for %d movies\n", argv[1], movie_count);

    int choice = 0;
    while(choice != 4){
        print_menu();
        printf("PLease enter a choice from 1 to 4: ");
        char* decision = NULL;
        size_t n = 0;
        getline(&decision, &n, stdin);
        choice = strtol(decision, NULL, 10);
        free(decision);
        if (choice == 1){
            print_year(head);
        } else if (choice == 2){
            ratings_by_year(head);
        } else if (choice == 3){
            print_language(head);
        } else {
            printf("Goodbye! Thank you for playing my movie catalog program :)\n");
        }    
    }

    
    free_list(head);	

	return 0;
}