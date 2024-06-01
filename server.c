#include        <sys/types.h>   /* basic system data types */
#include        <sys/socket.h>  /* basic socket definitions */
#include        <sys/time.h>    /* timeval{} for select() */
#include        <time.h>        /* timespec{} for pselect() */
#include        <netinet/in.h>  /* sockaddr_in{} and other Internet defns */
#include        <arpa/inet.h>   /* inet(3) functions */
#include        <errno.h>
#include        <fcntl.h>       /* for nonblocking */
#include        <netdb.h>
#include        <signal.h>
#include        <stdio.h>
#include        <stdlib.h>
#include        <string.h>
#include        <unistd.h>
#include <stdbool.h>

#define MAXLINE 1024
#define LISTENQ 2

#define MAX_GUESSES 10
#define WORD_LENGTH 5
#define NAME_LENGTH 32

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

#define MAX_WORDS 20000

char **words;
int wordCount;

char word_to_guess[WORD_LENGTH+1];
char enemy_word_to_guess[WORD_LENGTH+1];
char my_guess[WORD_LENGTH+1];
char enemy_guess[WORD_LENGTH+1];

char recv_send_type[MAXLINE / 2];
char recv_send_message[MAXLINE / 2];

char server_name[NAME_LENGTH+1];
char server_opponent[NAME_LENGTH+1];

char guess_history[MAX_GUESSES][WORD_LENGTH + 1];
int current_guess_server = 0;

bool did_server_win = false;

char server_announcement[MAXLINE / 2];

char read_words_server[MAX_WORDS][WORD_LENGTH + 1];
int word_count_server = 0;

int receiveMessageServer(int sockfd, char recvline[MAXLINE + 1]) {
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
        
        if (strcmp(recv_send_type, "confirm_word_to_guess") == 0) {
            strcpy(enemy_word_to_guess,recv_send_message);
        }

        if (strcmp(recv_send_type, "name") == 0) {
            strcpy(server_opponent,recv_send_message);
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

void sendMessageServer(int sockfd, char message[MAXLINE + 1]) {
    if (write(sockfd, message, strlen(message)) < 0) {
        fprintf(stderr, "write error: %s\n", strerror(errno));
    }
}

void printBoardServer() {
    printf("+===============+\n");
    //printf(" %s vs %s\n",nick,enemy_nick);
    printf( BLUE "%s" RESET " vs " RED "%s" RESET "\n",server_name,server_opponent);
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
            if((i)%2==0){
                printf(" - " BLUE "%s" RESET ,server_name);
            }else{
                printf(" - " RED "%s" RESET ,server_opponent);
            }
            printf("\n");
        }
    }
    printf("+===============+\n");
    printf("%s",server_announcement);
}

bool checkGuessServer(char* guess) {
    printf("[DEBUG] Processing guess: %s\n", guess);
    strcpy(guess_history[current_guess_server], guess);
    current_guess_server++; 
    if (strcmp(guess, word_to_guess) == 0) {
        printf("[DEBUG]Correct!\n");
        return true;
    } else {
        printf("[DEBUG]Incorrect. Try again.\n");
        if(current_guess_server == MAX_GUESSES){
            return true;
        }
        return false;
    }
}

bool isInWordListServer(char word[WORD_LENGTH+1], char **words, int wordCount) {
    word[strcspn(word, "\r\n")] = 0;
    for (int i = 0; i < wordCount; i++) {
        if (strncmp(word, words[i], WORD_LENGTH) == 0) {
            return true;
        }
    }
    return false;
}

void clearInputBufferServer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

void initializeWordListServer(char **words, int count) {
    word_count_server = count < MAX_WORDS ? count : MAX_WORDS;
    for (int i = 0; i < word_count_server ; i++) {
        strncpy(read_words_server [i], words[i], WORD_LENGTH);
        read_words_server[i][WORD_LENGTH] = '\0';
    }
}
bool isWordInGlobalListServer(char word[]) {
    for (int i = 0; i < word_count_server; i++) {
        if (strncmp(word, read_words_server[i], WORD_LENGTH) == 0) {
            return true;
        }
    }
    return false;
}

void rollWordToGuess() {
    if (word_count_server > 0) {
        int randomIndex = rand() % word_count_server;
        strcpy(word_to_guess, read_words_server[randomIndex]);
    } else {
        printf("[ERROR] No words available to roll\n");
        exit(1);
    }
}

int startServer()
{

    int                 listenfd, connfd;
    socklen_t           len;
    char                buff[MAXLINE], str[INET_ADDRSTRLEN+1];
    time_t              ticks;
    struct sockaddr_in  servaddr, cliaddr;
    char recvline[MAXLINE + 1];
    char initialMessage[MAXLINE + 1];

    char ip_address[] = "127.0.0.1";  // Define the IP address here
    int port = 10000;              // Define the port number here

    // GET NAME, IP AND PORT

    // get name
    while (1) {
        printf("Enter your name (max %d characters): ", NAME_LENGTH);
        if (fgets(server_name, sizeof(server_name), stdin) != NULL) {
            
            size_t len = strlen(server_name);
            if (len > 0 && server_name[len - 1] == '\n') {
                server_name[len - 1] = '\0';
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
                clearInputBufferServer();
            } else {
                break;
            }
        } else {
            printf("Error reading input. Please try again.\n");
            clearerr(stdin);
        }
    }

    clearInputBufferServer();

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
                clearInputBufferServer();
            } else {
                break;
            }
        } else {
            printf("Error reading input. Please try again.\n");
            clearerr(stdin);
        }
    }

    printf("Info entered: name=%s | ip=%s | port=%d\n",server_name,ip_address,port);
    sleep(1);


    // Start Connection

    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        fprintf(stderr, "socket error : %s\n", strerror(errno));
        return 1;
    }

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(ip_address);  /* Use the defined IP address */
    servaddr.sin_port = htons(port);  /* Use the defined port */

    if (bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
        fprintf(stderr, "bind error : %s\n", strerror(errno));
        return 1;
    }

    if (listen(listenfd, LISTENQ) < 0){
        fprintf(stderr, "listen error : %s\n", strerror(errno));
        return 1;
    }

    fprintf(stderr, "Waiting for a client ... \n");

    len = sizeof(cliaddr);
    if ((connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len)) < 0) {
        fprintf(stderr, "accept error: %s\n", strerror(errno));
        return 1;
    }

    bzero(str, sizeof(str));
    inet_ntop(AF_INET, &cliaddr.sin_addr, str, sizeof(str));
    printf("Connection from %s\n", str);

    fflush(stdout);
    

    system("clear");
    // Client connected, send him his name and retrieve his
    strcpy(initialMessage, "name|");
    strcat(initialMessage, server_name);
    strcat(initialMessage, "\n"); 
    sendMessageServer(connfd, initialMessage);

    if (receiveMessageServer(connfd, recvline) != 0) {
        printf("Failed to receive message.\n");
    }


    // Roll the word to guess, send it to client, and wait for response
    //words = read_words;
    //wordCount = word_count;
    srand(time(NULL)); // Seed the random number generator
    //int randomIndex = rand() % wordCount;
    //strcpy(word_to_guess, read_words[randomIndex]);
    //strcpy(word_to_guess, read_words_server[randomIndex]);
    rollWordToGuess();

    printf("WORD TO GUESS IS %s\n",word_to_guess);

    strcpy(initialMessage, "word_to_guess|");
    strcat(initialMessage, word_to_guess);
    strcat(initialMessage, "\n"); 
    sendMessageServer(connfd, initialMessage);

    if (receiveMessageServer(connfd, recvline) != 0) {
        printf("Failed to receive message.\n");
    }
    //printf("initialMessage message: %s\n", initialMessage);
    //printf("recvline message: %s\n", recvline);
    //printf("word_to_guess message: %s\n", word_to_guess);
    enemy_word_to_guess[strcspn(enemy_word_to_guess, "\r\n")] = 0;
    //printf("enemy_word_to_guess message: %s\n", enemy_word_to_guess);
    if(strcmp(word_to_guess,enemy_word_to_guess)==0){
        printf("Words match, yey\n");
    }else{
        printf("ERROR!| Words do not match!!\n");
        exit(2);
    }

    printf("info so far:\n");
    printf("word_to_guess: %s\n",word_to_guess);
    printf("my name: %s\n",server_name);
    printf("their name: %s\n",server_opponent);

    strcpy(server_announcement, "");
    //everything set, lets start the game!
    while (1) {
        system("clear");
        printBoardServer();
        printf("Enter your guess (max %d characters): ", WORD_LENGTH);
        if (fgets(my_guess, sizeof(my_guess), stdin) != NULL) {
            
            size_t len = strlen(my_guess);
            if (len > 0 && my_guess[len - 1] == '\n') {
                my_guess[len - 1] = '\0';
                len--; 
            }

            //printf("%d vs %d",len,WORD_LENGTH);
            //printf("Debug: len = %zu, expected = %d\n", len, WORD_LENGTH);
            //sleep(2);

            if (len == 0) {
                strcpy(server_announcement, "");
                continue;
            }

            if (len != WORD_LENGTH) {
                printf("Invalid word length. Please try again.\n");
                strcpy(server_announcement, "Invalid word length. Please try again.\n");
                continue;
            }

            // if (!isInWordListServer(my_guess, words, wordCount)) {
            //     printf("The guessed word is not in the word list.\n");
            //     strcpy(server_announcement, "The guessed word is not in the word list.\n");
            //     continue;
            // }

        if (!isWordInGlobalListServer(my_guess)) {
            printf("The guessed word is not in the word list.\n");
            strcpy(server_announcement, "The guessed word is not in the word list.\n");
            continue;
        }

            strcpy(initialMessage, "guess|");
            strcat(initialMessage, my_guess);
            strcat(initialMessage, "\n");
            sendMessageServer(connfd, initialMessage);

            bool is_correct = checkGuessServer(my_guess);
            if (is_correct) {
                did_server_win = true;
                break;
            }

        } else {
            printf("Error reading input. Please try again.\n");
            strcpy(server_announcement, "Error reading input. Please try again.\n");
            clearerr(stdin);
            continue;
        }
        //now receive it
        strcpy(server_announcement, "");
        system("clear");
        printBoardServer();
        printf("Waiting for opponent...\n");

        if (receiveMessageServer(connfd, recvline) != 0) {
            printf("Failed to receive message.\n");
        }

        bool is_correct = checkGuessServer(enemy_guess);
        if(is_correct){
            break;
        }

    }

    //game finished
    strcpy(server_announcement, "");
    system("clear");
    printBoardServer();
    printf("GAME IS OVER\n");
    if(did_server_win){
        printf( GREEN "CONGRATULATIONS YOU WON!!!\n" RESET);
    }else{
        printf( RED "better luck next time :)\n" RESET);
    }

    close(connfd);
    close(listenfd);

    exit(0);
}
