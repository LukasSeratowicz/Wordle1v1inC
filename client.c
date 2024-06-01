#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include <stdbool.h>

#define MAXLINE 1024
#define MAX_GUESSES 10
#define WORD_LENGTH 5
#define NAME_LENGTH 32
#define SA      struct sockaddr

//colors
#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define RESET "\x1b[0m"
#define RED "\x1b[31m"
#define BLUE "\x1b[34m"
#define MAGENTA "\x1b[35m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[37m"
#define BLACK "\x1b[30m"
#define BRIGHT_RED "\x1b[91m"
#define BRIGHT_GREEN "\x1b[92m"
#define BRIGHT_YELLOW "\x1b[93m"
#define BRIGHT_BLUE "\x1b[94m"
#define BRIGHT_MAGENTA "\x1b[95m"
#define BRIGHT_CYAN "\x1b[96m"
#define BRIGHT_WHITE "\x1b[97m"

char recv_send_type[MAXLINE / 2];
char recv_send_message[MAXLINE / 2];

char word_to_guess[WORD_LENGTH+1];
char my_guess[WORD_LENGTH+1];
char enemy_guess[WORD_LENGTH+1];

char client_name[NAME_LENGTH+1];
char client_opponent[NAME_LENGTH+1];

char guess_history[MAX_GUESSES][WORD_LENGTH + 1];
int current_guess = 0;

bool did_client_win = false;

char client_announcement[MAXLINE / 2];

int receiveMessage(int sockfd, char recvline[MAXLINE + 1]) {
    int n;
    while ( (n = read(sockfd, recvline, MAXLINE)) > 0) {
        recvline[n] = 0;    /* null terminate */
        if (fputs(recvline, stdout) == EOF){
            fprintf(stderr,"fputs error : %s\n", strerror(errno));
            return -1;
        }
        
        char *token;
        token = strtok(recvline, "|");
        if (token != NULL) {
            strncpy(recv_send_type, token, sizeof(recv_send_type) - 1);
            recv_send_type[sizeof(recv_send_type) - 1] = '\0';  // Ensure null-termination

            token = strtok(NULL, "|");
            if (token != NULL) {
                strncpy(recv_send_message, token, sizeof(recv_send_message) - 1);
                recv_send_message[sizeof(recv_send_message) - 1] = '\0';  // Ensure null-termination
            }
        }
        recv_send_message[strcspn(recv_send_message, "\r\n")] = 0;
        recv_send_type[strcspn(recv_send_type, "\r\n")] = 0;
        printf("[DEBUG] Server [TAG:%s] says: %s\n", recv_send_type, recv_send_message);

        if (strcmp(recv_send_type, "word_to_guess") == 0) {
            strcpy(word_to_guess,recv_send_message);
        }

        if (strcmp(recv_send_type, "name") == 0) {
            strcpy(client_opponent,recv_send_message);
        }

        if (strcmp(recv_send_type, "guess") == 0) {
            strcpy(enemy_guess,recv_send_message);
        }

        return 0;
    }
    if (n < 0) {
        fprintf(stderr, "read error: %s\n", strerror(errno));
        return -1;
    }
    return -1;
}

void sendMessage(int sockfd, char message[MAXLINE + 1]) {
    if (write(sockfd, message, strlen(message)) < 0) {
        fprintf(stderr, "write error: %s\n", strerror(errno));
    }
}

void printBoardClient() {
    printf("+===============+\n");
    //printf(" %s vs %s\n",nick,enemy_nick);
    printf( BLUE "%s" RESET " vs " RED "%s" RESET "\n",client_name,client_opponent);
    printf("+===============+\n");
    for (int i = 0; i < MAX_GUESSES; i++) {
        if (guess_history[i][0] == '\0') {
            printf("||_||_||_||_||_||\n");
        } else {
            for (int j = 0; j < WORD_LENGTH; j++) {
                if (guess_history[i][j] == word_to_guess[j]) {
                    printf("||" GREEN "%c" RESET, guess_history[i][j]);
                } else if (strchr(word_to_guess, guess_history[i][j])) {
                    printf("||" YELLOW "%c" RESET, guess_history[i][j]);
                } else {
                    printf("||%c", guess_history[i][j]);
                }
            }
            printf("||");
            if((i+1)%2==0){
                printf(" - " BLUE "%s" RESET ,client_name);
            }else{
                printf(" - " RED "%s" RESET ,client_opponent);
            }
            printf("\n");
        }
    }
    printf("+===============+\n");
    printf("%s",client_announcement);
}

bool checkGuessClient(char* guess) {
    printf("[DEBUG] Processing guess: %s\n", guess);
    strcpy(guess_history[current_guess], guess);
    current_guess++; 
    if (strcmp(guess, word_to_guess) == 0) {
        printf("[DEBUG]Correct!\n");
        return true;
    } else {
        printf("[DEBUG]Incorrect. Try again.\n");
        if(current_guess == MAX_GUESSES){
            return true;
        }
        return false;
    }
}

bool isInWordListClient(char word[], char **words, int wordCount) {
    word[strcspn(word, "\r\n")] = 0;
    for (int i = 0; i < wordCount; i++) {
        if (strncmp(word, words[i], WORD_LENGTH) == 0 && strlen(words[i]) == WORD_LENGTH) {
            return true;
        }
    }
    return false;
}

void clearInputBufferClient() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

int startClient(char **words, int wordCount)
{
    int                 sockfd, n;
    struct sockaddr_in  servaddr;
    char                recvline[MAXLINE + 1];
    char                message[MAXLINE + 1];
    int err;

    char ip_address[] = "127.0.0.1";  // Define the IP address here
    int port = 10000;                   // Define the port number here


    // GET NAME, IP AND PORT
    while (1) {
        printf("Enter your name (max %d characters): ", NAME_LENGTH);
        if (fgets(client_name, sizeof(client_name), stdin) != NULL) {
            
            size_t len = strlen(client_name);
            if (len > 0 && client_name[len - 1] == '\n') {
                client_name[len - 1] = '\0';
            }

            if (len > 1 && len <= NAME_LENGTH) {
                break;
            } else {
                printf("Invalid name. Please try again.\n");
            }
        } else {
            printf("Error reading input. Please try again.\n");
            clearerr(stdin);
        }
    }

    // get ip
    while (1) {
        printf("Enter the IP address: ");
        if (fgets(ip_address, sizeof(ip_address), stdin) != NULL) {
            
            size_t len = strlen(ip_address);
            if (len > 0 && ip_address[len - 1] == '\n') {
                ip_address[len - 1] = '\0';
            }

            struct sockaddr_in sa;
            if (inet_pton(AF_INET, ip_address, &(sa.sin_addr)) != 1) {
                printf("Invalid IP address. Please try again.\n");
                clearInputBufferClient();
            } else {
                break;
            }
        } else {
            printf("Error reading input. Please try again.\n");
            clearerr(stdin);
        }
    }

    clearInputBufferClient();

    //get port
    while (1) {
        char port_str[MAXLINE];
        printf("Enter the port number: ");
        if (fgets(port_str, sizeof(port_str), stdin) != NULL) {
            size_t len = strlen(port_str);
            if (len > 0 && port_str[len - 1] == '\n') {
                port_str[len - 1] = '\0';
            }

            port = atoi(port_str);
            if (port <= 0 || port > 65535) {
                printf("Invalid port number. Please enter a number between 1 and 65535.\n");
                clearInputBufferClient();
            } else {
                break;
            }
        } else {
            printf("Error reading input. Please try again.\n");
            clearerr(stdin);
        }
    }

    printf("Info entered: name=%s | ip=%s | port=%d\n",client_name,ip_address,port);
    sleep(1);

    if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr,"socket error : %s\n", strerror(errno));
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(port);    /* Use the defined port */
    if ( (err = inet_pton(AF_INET, ip_address, &servaddr.sin_addr)) <= 0){
        if(err == 0 )
            fprintf(stderr,"inet_pton error for %s \n", ip_address);
        else
            fprintf(stderr,"inet_pton error for %s : %s \n", ip_address, strerror(errno));
        return 1;
    }
    if (connect(sockfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr,"connect error : %s \n", strerror(errno));
        return 1;
    }

    system("clear");
    // Server connected, wait for his name and send yours
    if (receiveMessage(sockfd, recvline) == 0) {
        fflush(stdout);
    } else {
        printf("Failed to receive message.\n");
    }
    strcpy(message, "name|");
    strcat(message, client_name); 
    strcat(message, "\n");
    sendMessage(sockfd, message);




    if (receiveMessage(sockfd, recvline) == 0) {
        fflush(stdout);
    } else {
        printf("Failed to receive message.\n");
    }
    word_to_guess[strcspn(word_to_guess, "\r\n")] = 0;
    sleep(1);
    strcpy(message, "confirm_word_to_guess|");
    strcat(message, word_to_guess); 
    strcat(message, "\n");
    //printf("Sending %s",message);
    sendMessage(sockfd, message);

    printf("info so far:\n");
    printf("word_to_guess: %s\n",word_to_guess);
    printf("my name: %s\n",client_name);
    printf("their name: %s\n",client_opponent);

    //everything set, lets start the game!

    system("clear");
    printBoardClient();
    printf("Waiting for opponent...\n");
    // first we wait for enemies guess before we start ours
    if (receiveMessage(sockfd, recvline) == 0) {
        fflush(stdout);
    } else {
        printf("Failed to receive message.\n");
    }

    strcpy(client_announcement, "");
    bool is_correct = checkGuessClient(enemy_guess);
    if(is_correct){
        system("clear");
        printBoardClient();
        printf("GAME IS OVER\n");
        if(did_client_win){
            printf( GREEN "CONGRATULATIONS YOU WON!!!\n" RESET);
        }else{
            printf( RED "better luck next time :)\n" RESET);
        }
        exit(0);
    }

    while (1) {
        system("clear");
        printBoardClient();
        printf("Enter your guess (max %d characters): ", WORD_LENGTH);
        if (fgets(my_guess, sizeof(my_guess), stdin) != NULL) {
            
            size_t len = strlen(my_guess);
            if (len > 0 && my_guess[len - 1] == '\n') {
                my_guess[len - 1] = '\0';
                len--;
            }

            if (len == 0) {
                strcpy(client_announcement, "");
                continue;
            }

            if (len != WORD_LENGTH) {
                printf("Invalid word length. Please try again.\n");
                strcpy(client_announcement, "Invalid word length. Please try again.\n");
                continue;
            }

            // if (!isInWordListClient(my_guess, words, wordCount)) {
            //     printf("The guessed word is not in the word list.\n");
            //     strcpy(client_announcement, "The guessed word is not in the word list.\n");
            //     continue;
            // }

            strcpy(message, "guess|");
            strcat(message, my_guess);
            strcat(message, "\n");
            sendMessage(sockfd, message);

            bool is_correct = checkGuessClient(my_guess);
            if (is_correct) {
                did_client_win = true;
                break;
            }

        } else {
            printf("Error reading input. Please try again.\n");
            strcpy(client_announcement, "Error reading input. Please try again.\n");
            clearerr(stdin);
            continue;
        }
        //now receive it
        strcpy(client_announcement, "");
        system("clear");
        printBoardClient();
        printf("Waiting for opponent...\n");


        if (receiveMessage(sockfd, recvline) == 0) {
            fflush(stdout);
        } else {
            printf("Failed to receive message.\n");
        }

        bool is_correct = checkGuessClient(enemy_guess);
        if(is_correct){
            break;
        }
    }

    //game finished
    strcpy(client_announcement, "");
    system("clear");
    printBoardClient();
    printf("GAME IS OVER\n");
    if(did_client_win){
        printf( GREEN "CONGRATULATIONS YOU WON!!!\n" RESET);
    }else{
        printf( RED "better luck next time :)\n" RESET);
    }

    exit(0);
}
