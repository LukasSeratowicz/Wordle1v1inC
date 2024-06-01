#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include	<unistd.h>

#include "client.c"
#include "server.c"

#define MAX_WORD_LENGTH 6 // 5 char + null term

void clearScreen() {
    system("clear");
}



int main() {
    printf("Loading...\n");
    FILE *file;
    char **words = NULL;
    int wordCount = 0;
    char buffer[MAX_WORD_LENGTH];
    
    file = fopen("wordle-La.txt","r");
    if (file == NULL) {
        perror("Error opening wordle-La.txt");
        return 1;
    }
    while (fscanf(file, "%s", buffer) != EOF){
        char *word = (char *)malloc(MAX_WORD_LENGTH * sizeof(char));
        if (word == NULL){
            printf("Memory allocation failed.\n");
            return 1;
        }
        strcpy(word,buffer);

        words = (char **)realloc(words, (wordCount + 1) * sizeof(char *));
        if (words == NULL){
            printf("Memory allocation failed.\n");
            return 1;
        }

        words[wordCount] = word;
        wordCount++;
    }
    fclose(file);
    for (int i = 0; i<5; i++){
        printf("%s\n",words[i]);
    }
    clearScreen();
    printf("=====================\n");
    printf("Welcome to Wordle 1v1\n");
    printf("=====================\n");
    printf("Type 1 to join the game\n");
    printf("Type 2 to host a game\n");
    
    int in = -1;
    do {
        printf("choice: ");
        scanf("%d", &in);
        getchar(); // To consume the newline 
        if(in < 1 || in > 2){
            clearScreen();
            printf("Your choice (%d) was incorrect!\n",in);
            printf("=====================\n");
            printf("Welcome to Wordle 1v1\n");
            printf("=====================\n");
            printf("Type 1 to join the game\n");
            printf("Type 2 to host a game\n");
        }
    } while (in < 1 || in > 2);

    clearScreen();
    
    //testGameLogic("Main");
    if (in == 1) {
        startClient(words, wordCount);
    } else if (in == 2) {
        initializeWordListServer(words, wordCount);
        startServer();
    }

    //executeGameLogic("Main");

    printf("Click anything to close...");
    getchar();
    return 0;
}