#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <netdb.h>
#include <poll.h> 
#include <unordered_set>
#include <signal.h>
#include <chrono>
#include <thread>
#include <string>
#include <charconv>
#include <cstring>
#include <iostream>
#include <fstream>

const int sendInfo = 10;
const int sendLetter = 11;
const int sendTimeLobby = 12;
const int sendTimeGame = 13;
const int sendError = 14;

const int sendRankingStart = 15;
const int sendRankingPosition = 16;
const int sendRankingEnd = 17;

const int sendNicknamesStart = 18;
const int sendNickname = 19;
const int sendNicknamesEnd = 20;

const int sendNickUniqnessInfo = 21;

const int receiveNickname = 100;
const int receiveCountry = 101;
const int receiveCity = 102;
const int receiveName = 103;
const int receiveAnimal = 104;
const int receiveJob = 105;
const int receiveObject = 106;

const int startGameSignal = 200;
const int stopGameSignal = 201;

char countries[255][20];
char cities[255][20];
char names[255][20];
char animals[255][20];
char jobs[255][20];
char objects[255][20];

// server socket
int servFd;

// data for poll
int descrCapacity = 16;
int descrCount = 1;
pollfd * descr;

// data for the game
char currentLetter;
int gamersCounter = 0;
int lobbyCounter = 0;
int gameCounter = 0;
//game state = 1 : there is no game rigth now
//game state = 2 : there is a game rigth now
int gamersState[255];
int gameState = 1;
int roundTime = 30;
int breakTime = 40;
int gameTime = 30;

struct playerStats {
    char nickname[10];
    int points;
};

playerStats playersStats[255];

char buffer[255];

//Array Management
void clearBuffer();
void clearArray(char* message, int size);
int setBuffer(int type, char* content);
void clearGameArrays();

// handles SIGINT
void ctrl_c(int);
uint16_t readPort(char * txt);
void setReuseAddr(int sock);

//Send message to clients
void sendToAll(int count);
void sendToClient(int receiverFd, int count);
void sendToAllinGame(int count);
void sendToAllinLobby(int count);

//Players Management 
void clearPoints();
void gamerLeaves(int clientFd);
bool isNicknameUnique(char* nickname);
void sendNicknamesLobby();

//Game Management
void drawingLetter();
bool areValuesUnique(int currentFd, char array[255][20]);
void createRanking();
void sendRanking();

//Network events handling
void eventOnServFd(int revents) ;
void eventOnClientFd(int indexInDescr);
    

//Messages Menagement
int extractMessageType(char* readBuffer);
void extractMessageText(char* readBuffer, char* messageText);
 
void gameManager(){
	std::chrono::steady_clock sc;
	auto startBreak = sc.now();
	auto startGame = sc.now();
	auto startRoundTime = sc.now();
    auto sendSeconds = sc.now();
    char message[255];
    int count, timeToNextGame, timeToGameEnd;
    bool firstIteration = true;
    int retry = 1;

	while(true) {

        clearArray(&message[0], 255);

		auto sendSecondsSpan = static_cast<std::chrono::duration<double>>(sc.now() - sendSeconds);
		auto breakTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startBreak);
		auto gameTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startGame);

        if (sendSecondsSpan.count() > 1) {
            sendSeconds = sc.now();
            if (gameState == 1) {
                if ((int)(breakTime - breakTimeSpan.count()) >= 0) {
                    timeToNextGame = (int)(breakTime - breakTimeSpan.count());
                    snprintf(message, sizeof(message), "%d\n", timeToNextGame);
                    count = setBuffer(sendTimeLobby, &message[0]);
                    sendToAllinLobby(count);
                }
                else {
                    retry--;
                    if (retry == 0) {
                        snprintf(message, sizeof(message), "Czas przerwy minal, jednak w lobby jest za malo uzytkownikow zeby zaczac gre\n");
                        count = setBuffer(sendInfo, &message[0]);
                        sendToAllinLobby(count);
                        retry = 10;
                    }
                }
            }
            else if (gameState == 2) {
                timeToNextGame = (int)(gameTime + breakTime - gameTimeSpan.count());
                snprintf(message, sizeof(message), "%d\n",  timeToNextGame);
                count = setBuffer(sendTimeLobby, &message[0]);
    			sendToAllinLobby(count);

                timeToGameEnd = (int)(gameTime - gameTimeSpan.count());
                snprintf(message, sizeof(message), "%d\n", timeToGameEnd);
                count = setBuffer(sendTimeGame, &message[0]);
			    sendToAllinGame(count);
            }
        }


		if (breakTimeSpan.count() > breakTime && gameState == 1 && gamersCounter >= 2) {
            firstIteration = false;
            retry = 1;
			gameState = 2;
			startGame = sc.now();
			startRoundTime = sc.now();
			for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd && strcmp(playersStats[descr[i].fd].nickname, "") != 0) {
					gamersState[descr[i].fd] = 2;
					gameCounter++;
            	}
			}

			lobbyCounter = 0;
            snprintf(message, sizeof(message), "\n");
            count = setBuffer(startGameSignal, &message[0]);
			sendToAll(count);
			drawingLetter();

            clearArray(&message[0], 255);

            snprintf(message, sizeof(message), "Rozpoczynacie gre, aktualnie jest was %d graczy\n", gameCounter);
            count = setBuffer(sendInfo, &message[0]);
			sendToAllinGame(count);
		}
		else if ((gameTimeSpan.count() > gameTime && gameState == 2) || (gamersCounter < 2 && gameState == 2)) {
            if (firstIteration == false) {
                createRanking();
                sendRanking();
                clearPoints();
            }
			startBreak = sc.now();
			gameState = 1;
            clearArray(&message[0], 255);
            snprintf(message, sizeof(message), "Gra sie konczy, uzytkownicy wracaja do lobby\n");
            count = setBuffer(sendInfo, &message[0]);
			gameCounter = 0;
			sendToAllinGame(count);
            for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd && strcmp(playersStats[descr[i].fd].nickname, "")) {
					gamersState[descr[i].fd] = 1;
					lobbyCounter++;
            	}
			}
            sendNicknamesLobby();
            clearArray(&message[0], 255);
            snprintf(message, sizeof(message), "\n");
            count = setBuffer(stopGameSignal, &message[0]);
			sendToAll(count);

            clearArray(&message[0], 255);
            snprintf(message, sizeof(message), "Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
            count = setBuffer(sendInfo, &message[0]);
            sendToAllinLobby(count);
		}

		if (gameState == 2) {
			auto roundTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startRoundTime);
			if (roundTimeSpan.count() >= roundTime) {
                createRanking();
				drawingLetter();
				startRoundTime = sc.now();
			}
		}
	}
}

int main(int argc, char ** argv){
    
    int pos;
    std::string temp;
    char temp_ip[20], temp_port[4];
    std::ifstream config("config.txt");

    getline(config, temp);
    pos = temp.find(" : ");
    breakTime = stoi(temp.substr(pos + 3));
    getline(config, temp);
    pos = temp.find(" : ");
    gameTime = stoi(temp.substr(pos + 3));
    getline(config, temp);
    pos = temp.find(" : ");
    roundTime = stoi(temp.substr(pos + 3));
    getline(config, temp);
    pos = temp.find(" : ");
    strcpy(temp_ip, temp.substr(pos + 3).c_str());
    printf("%s\n", temp_ip);
    in_addr ip;
    inet_aton(temp_ip, &ip);

    getline(config, temp);
    pos = temp.find(" : ");
    strcpy(temp_port, temp.substr(pos + 3).c_str());

    // get and validate port number
    // if(argc != 2) error(1, 0, "Need 1 arg (port)");
    auto port = readPort(temp_port);
    
    // create socket
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if(servFd == -1) error(1, errno, "socket failed");
    
    // graceful ctrl+c exit
    signal(SIGINT, ctrl_c);
    // prevent dead sockets from throwing pipe errors on write
    signal(SIGPIPE, SIG_IGN);
    
    setReuseAddr(servFd);
    
    // bind to any address and port provided in arguments
    
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr = ip};
    int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) error(1, errno, "bind failed");
    
    // enter listening mode
    res = listen(servFd, 1);
    if(res) error(1, errno, "listen failed");

    descr = (pollfd*) malloc(sizeof(pollfd)*descrCapacity);
    
    descr[0].fd = servFd;
    descr[0].events = POLLIN;

    config.close();

	std::thread t(gameManager);

    while(true){

		// std::thread t(drawingLetter);
        int ready = poll(descr, descrCount, -1);
        if(ready == -1){
            error(0, errno, "poll failed");
            ctrl_c(SIGINT);
        }
        
        for(int i = 0 ; i < descrCount && ready > 0 ; ++i){
            if(descr[i].revents){
                if(descr[i].fd == servFd)
                    eventOnServFd(descr[i].revents);
                else
                    eventOnClientFd(i);
                ready--;
            }
        }
    }
}

//Array Management
void clearBuffer() {
    for(int i = 0; i < 255; i++) {
        buffer[i] = 0;
    }
}

void clearArray(char* message, int size) {
    for (int i = 0; i < size; i++) {
        message[i] = 0;
    }
}

int setBuffer(int type, char* content) {
    clearBuffer();

    int length = std::to_string(type).length();
    char numType[4];
	std::to_chars(&numType[0], &numType[0] + length, type);
    for(int i = length; i < 4; i++ ) {
        numType[i] = char(32);
    }

    int tmp = numType[3];
    snprintf(&buffer[0], 4, numType);
    snprintf(&buffer[4], sizeof(buffer) - 4, content);
    printf("content %s\n", content);


    return std::strlen(content) + 4;
}

void clearGameArrays() {
    int i, j;

    for(i = 0; i < 255; i++) {
        for (j = 0; j < 20; j++) {
            countries[i][j] = 0;
            cities[i][j] = 0;
            names[i][j] = 0;
            animals[i][j] = 0;
            jobs[i][j] = 0;
            objects[i][j] = 0;
        }
    }
}


//Send Message to clients
void sendToAll(int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        if (gamersState[clientFd] == 1 || gamersState[clientFd] == 2) {
            int res = write(clientFd, buffer, count);
            if(res!=count){
                printf("removing %d\n", clientFd);
                gamerLeaves(clientFd);
                shutdown(clientFd, SHUT_RDWR);
                close(clientFd);
                descr[i] = descr[descrCount-1];
                descrCount--;
                sendNicknamesLobby();
                continue;
            }
        }
        i++;
    }
}

void sendToClient(int receiverFd, int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        if (clientFd == receiverFd) {
            int res = write(receiverFd, buffer, count);
            if(res!=count){
                printf("removing %d\n", clientFd);
                gamerLeaves(clientFd);
                shutdown(clientFd, SHUT_RDWR);
                close(clientFd);
                descr[i] = descr[descrCount-1];
                descrCount--;
                sendNicknamesLobby();
                continue;
            }
        }
        i++;
    } 
}


void sendToAllinGame(int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
		if (gamersState[clientFd] == 2) {
        	int res = write(clientFd, buffer, count);
			if(res!=count){
				printf("removing %d\n", clientFd);
                gamerLeaves(clientFd);
				shutdown(clientFd, SHUT_RDWR);
				close(clientFd);
				descr[i] = descr[descrCount-1];
				descrCount--;
                sendNicknamesLobby();
				continue;
			}
        }
        i++;
    }
}

void sendToAllinLobby(int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
		if (gamersState[clientFd] == 1) {
        	int res = write(clientFd, buffer, count);
			if(res!=count){
				printf("removing %d\n", clientFd);
                gamerLeaves(clientFd);
				shutdown(clientFd, SHUT_RDWR);
				close(clientFd);
				descr[i] = descr[descrCount-1];
				descrCount--;
                sendNicknamesLobby(); 
				continue;
			}
        }
        i++;
    }
}

//Players Management
void clearPoints() {
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        playersStats[clientFd].points = 0;
        i++;
    }
}

void gamerLeaves(int clientFd) {
	gamersCounter--;
    int count;
    if (gamersState[clientFd] == 1) {
        lobbyCounter--;
        char message[255];
        snprintf(message, sizeof(message), "Gracz wyszedl! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
        count = setBuffer(sendInfo, &message[0]);
	    sendToAllinLobby(count);
    }
    else if (gamersState[clientFd] == 2) {
        gameCounter--;
        char message[255];
        snprintf(message, sizeof(message), "Gracz wyszedl! Aktualnie w grze znaduje sie %d graczy.\n", gameCounter);
        count = setBuffer(sendInfo, &message[0]);
        sendToAllinGame(count);
    }
    strcpy(playersStats[clientFd].nickname, "");

}

bool isNicknameUnique(char* nickname) {
    int i = 1;
    bool res = true;
    while(i < descrCount){

        int clientFd = descr[i].fd;
        if (strcmp(nickname, playersStats[clientFd].nickname) == 0) {
            res = false;
            break;
        }
        i++;
        }
    return res;
}

void sendNicknamesLobby() {
    char message[255];
    int count;


    snprintf(message, sizeof(message), "\n");
    count = setBuffer(sendNicknamesStart, &message[0]);
	sendToAllinLobby(count);

    clearArray(&message[0], 255);

    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;

        if (gamersState[clientFd] == 1 && strcmp(playersStats[clientFd].nickname, "") != 0) {
            clearArray(&message[0], 255);
            snprintf(message, sizeof(message), "%s\n", playersStats[clientFd].nickname);
            count = setBuffer(sendNickname, &message[0]);
            sendToAllinLobby(count);
        }
        
        i++;
    }

    clearArray(&message[0], 255);

    snprintf(message, sizeof(message), "\n");
    count = setBuffer(sendNicknamesEnd, &message[0]);
	sendToAllinLobby(count);

}


//Game Management
void drawingLetter() {

	currentLetter = 'a' + rand()%26;

    char message[255];
    snprintf(message, sizeof(message), "%c \n",  currentLetter);
    int count;
    count = setBuffer(sendLetter, &message[0]);
	sendToAll(count);
}

bool areValuesUnique(int currentFd, char array[255][20]) {
    int i = 1;
    bool isUnique = true;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        if (clientFd != currentFd ){
            if (strcasecmp(array[clientFd], array[currentFd]) == 0) {
                isUnique = false;
                break;
            }
        }
        i++;
        }
        return isUnique;
}

void createRanking() {
    int i = 1;
    bool isUnique;
    printf("Letter for which we're counting: %c\n", currentLetter);
    while(i < descrCount){
        int clientFd = descr[i].fd;

        //check country
        if (countries[clientFd][0] != currentLetter && countries[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, countries);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }

          //check city
        if (cities[clientFd][0] != currentLetter && cities[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, cities);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }

          //check name
        if (names[clientFd][0] != currentLetter && names[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, names);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }

          //check animal
        if (animals[clientFd][0] != currentLetter && animals[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, animals);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }

          //check job
        if (jobs[clientFd][0] != currentLetter && jobs[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, jobs);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }

          //check object
        if (objects[clientFd][0] != currentLetter && objects[clientFd][0] != toupper(currentLetter)) {
            playersStats[clientFd].points += 0;
        }
        else {
            isUnique = areValuesUnique(clientFd, objects);
            if (isUnique) {
                playersStats[clientFd].points += 10;
            }
            else {
                playersStats[clientFd].points += 5;
            }
        }
        i++;
        }
        clearGameArrays();
    }

void sendRanking() {
    char message[255];
    int count;

    snprintf(message, sizeof(message), "%d\n", gameCounter);
    count = setBuffer(sendRankingStart, &message[0]);
	sendToAll(count);

    clearArray(&message[0], 255);

    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;

        if (gamersState[clientFd] == 2) {
            clearArray(&message[0], 255);

            snprintf(message, sizeof(message), "%s;%d\n", playersStats[clientFd].nickname, playersStats[clientFd].points);
            count = setBuffer(sendRankingPosition, &message[0]);
            sendToAll(count);
        }
        
        i++;
    }

    clearArray(&message[0], 255);

    snprintf(message, sizeof(message), "\n");
    count = setBuffer(sendRankingEnd, &message[0]);
	sendToAll(count);

}

//Network Event Menagement
void eventOnServFd(int revents) {
    // Wszystko co nie jest POLLIN na gnieździe nasłuchującym jest traktowane
    // jako błąd wyłączający aplikację
    if(revents & ~POLLIN){
        error(0, errno, "Event %x on server socket", revents);
        ctrl_c(SIGINT);
    }
    
    if(revents & POLLIN){
        sockaddr_in clientAddr{};
        socklen_t clientAddrSize = sizeof(clientAddr);
        
        auto clientFd = accept(servFd, (sockaddr*) &clientAddr, &clientAddrSize);
        if(clientFd == -1) error(1, errno, "accept failed");
        
        if(descrCount == descrCapacity){
            // Skończyło się miejsce w descr - podwój pojemność
            descrCapacity<<=1;
            descr = (pollfd*) realloc(descr, sizeof(pollfd)*descrCapacity);
        }
        
        descr[descrCount].fd = clientFd;
        descr[descrCount].events = POLLIN|POLLRDHUP;
        descrCount++;


        playersStats[clientFd].points = 0;
        strcpy(playersStats[clientFd].nickname, "");


        
        printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
    }
}





void eventOnClientFd(int indexInDescr) {
    auto clientFd = descr[indexInDescr].fd;
    auto revents = descr[indexInDescr].revents;
    
    if(revents & POLLIN){
        char readBuffer[255];
        char message[255];
        char messageText[255];
        char nickname[10];
        bool nicknameUnique;

        clearArray(&readBuffer[0], 255);

        int count = read(clientFd, readBuffer, 255);

		if(count < 1) {
            revents |= POLLERR; 
		}
        else {

            int i = 0;
            int temp = 0;
            char tempBuffer[20];

            while (i < count) {
                if (readBuffer[i] == '\n') {
                    clearArray(&tempBuffer[0], 20);
                    clearArray(&message[0], 255);
                    clearArray(&messageText[0], 255);

                    strncpy(tempBuffer, &readBuffer[temp], i - temp);
                    temp = i + 1;
                    int messageType = extractMessageType(&tempBuffer[0]);
                    extractMessageText(&tempBuffer[0], &messageText[0]);

                    switch (messageType) {
                        case receiveNickname:
                            strncpy(nickname, messageText, 10);
                            nicknameUnique = isNicknameUnique(&nickname[0]);

                            if (nicknameUnique == true) {
                                strncpy(playersStats[clientFd].nickname, nickname, 10);
                                snprintf(message, sizeof(message), "true\n");
                                count = setBuffer(sendNickUniqnessInfo, &message[0]);
                                sendToClient(clientFd, count);
                                gamersCounter++;
                                gamersState[clientFd] = 1;
                                lobbyCounter++;
                                snprintf(message, sizeof(message), "Nowy gracz! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
                                count = setBuffer(sendInfo, &message[0]);
                                sendToAllinLobby(count);
                                sendNicknamesLobby();
                            }
                            else {
                                snprintf(message, sizeof(message), "false\n");
                                count = setBuffer(sendNickUniqnessInfo, &message[0]);
                                sendToClient(clientFd, count);
                            }
                            break;
                        case receiveCountry:
                            strncpy(countries[clientFd], messageText, 20);
                            break;
                        case receiveCity:
                            strncpy(cities[clientFd], messageText, 20);
                            break;
                        case receiveName:
                            strncpy(names[clientFd], messageText, 20);
                            break;
                        case receiveAnimal:
                            strncpy(animals[clientFd], messageText, 20);
                            break;
                        case receiveJob:
                            strncpy(jobs[clientFd], messageText, 20);
                            break;
                        case receiveObject:
                            strncpy(objects[clientFd], messageText, 20);
                            break;
                    }
                }
                i++;
            }
           
        }
    }
    
    if(revents & ~POLLIN){
        int i = 1;
        while(i < descrCount){
            int tempClientFd = descr[i].fd;
            if (clientFd == tempClientFd) {
                printf("removing %d\n", clientFd);
                gamerLeaves(clientFd);
                // remove from description of watched files for poll
                descr[indexInDescr] = descr[descrCount-1];
                descrCount--;
                shutdown(clientFd, SHUT_RDWR);
                sendNicknamesLobby();
                close(clientFd);
            }
            i++;
        } 
    }
}

//Messagess Menagement
int extractMessageType(char* readBuffer) {
    char tmp[4];
    for (int i = 0; i < 4; i++) {
        tmp[i] = readBuffer[i];
    }

    return atoi(tmp);
}

void extractMessageText(char* readBuffer, char* messageText) {
    for (int i = 4; i < 255; i++) {
        messageText[i-4] = readBuffer[i];
    }
}

uint16_t readPort(char * txt){
    char * ptr;
    auto port = strtol(txt, &ptr, 10);
    if(*ptr!=0 || port<1 || (port>((1<<16)-1))) error(1,0,"illegal argument %s", txt);
    return port;
}

void setReuseAddr(int sock){
    const int one = 1;
    int res = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    if(res) error(1,errno, "setsockopt failed");
}

void ctrl_c(int){
    for(int i = 1 ; i < descrCount; ++i){
        shutdown(descr[i].fd, SHUT_RDWR);
        close(descr[i].fd);
    }
    close(servFd);
    printf("Closing server\n");
    exit(0);
}
