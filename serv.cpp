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

const int sendInfo = 10;
const int sendLetter = 11;
const int sendTime = 12;
const int sendError = 13;


const int receiveNickname = 100;
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
//game state = 0 : there is no game rigth now
//game state = 1 : there is a game rigth now
int gamersState[255];
int gameState = 0;
int roundTime = 10;
int breakTime = 10;
int gameTime = 30;

struct playerStats {
    char nickname[10];
    int points;
};

playerStats playersStats[255];

char buffer[255];

void clearBuffer() {
    for(int i = 0; i < 255; i++) {
        buffer[i] = 0;
    }
}

int setBuffer(int type, char* content) {
    clearBuffer();
    char numType[4];
	std::to_chars(&numType[0], &numType[3], type);

    snprintf(&buffer[0], 4, numType);
    snprintf(&buffer[4], sizeof(buffer) - 4, content);

    return std::strlen(content) + 4;
}

// handles SIGINT
void ctrl_c(int);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);

void sendToAll(int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        int res = write(clientFd, buffer, count);
        if(res!=count){
            printf("removing %d\n", clientFd);
            shutdown(clientFd, SHUT_RDWR);
            close(clientFd);
            descr[i] = descr[descrCount-1];
            descrCount--;
            continue;
        
        }
        i++;
    }
}

void sendToClient(int receiverFd, int count){
    int i = 1;
    while(i < descrCount){
        int clientFd = descr[i].fd;
        if (clientFd = receiverFd) {
            int res = write(receiverFd, buffer, count);
            if(res!=count){
                printf("removing %d\n", clientFd);
                shutdown(clientFd, SHUT_RDWR);
                close(clientFd);
                descr[i] = descr[descrCount-1];
                descrCount--;
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
		if (gamersState[clientFd] == 1) {
        	int res = write(clientFd, buffer, count);
			if(res!=count){
				printf("removing %d\n", clientFd);
				shutdown(clientFd, SHUT_RDWR);
				close(clientFd);
				descr[i] = descr[descrCount-1];
				descrCount--;
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
		if (gamersState[clientFd] == 0) {
        	int res = write(clientFd, buffer, count);
			if(res!=count){
				printf("removing %d\n", clientFd);
				shutdown(clientFd, SHUT_RDWR);
				close(clientFd);
				descr[i] = descr[descrCount-1];
				descrCount--;
				continue;
			}
        }
        i++;
    }
}

void gamerLeaves(int clientFd) {
	gamersCounter--;
    int count;
    if (gamersState[clientFd] == 0) {
        lobbyCounter--;
        char message[255];
        snprintf(message, sizeof(message), "Gracz wyszedl! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
        count = setBuffer(sendInfo, &message[0]);
	    sendToAllinLobby(count);
    }
    else if (gamersState[clientFd] == 1) {
        gameCounter--;
        char message[255];
        snprintf(message, sizeof(message), "Gracz wyszedl! Aktualnie w grze znaduje sie %d graczy.\n", gameCounter);
        count = setBuffer(sendInfo, &message[0]);
        sendToAllinGame(count);
    }

}

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
		gamersCounter++;
		gamersState[clientFd] = 0;
		lobbyCounter++;

        playersStats[clientFd].points = 0;

		char message[255];
        int count;
		snprintf(message, sizeof(message), "Nowy gracz! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
        count = setBuffer(sendInfo, &message[0]);
		sendToAllinLobby(count);
        
        printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
    }
}

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

void eventOnClientFd(int indexInDescr) {
    auto clientFd = descr[indexInDescr].fd;
    auto revents = descr[indexInDescr].revents;
    
    if(revents & POLLIN){
        char readBuffer[255];
        char message[255];
        char messageText[255];

        for(int i = 0; i < 255; i++) {
            readBuffer[i] = 0;
            message[i] = 0;
            messageText[i] = 0;
        }

        int count = read(clientFd, readBuffer, 255);
        int messageType = extractMessageType(&readBuffer[0]);
        extractMessageText(&readBuffer[0], &messageText[0]);
		if(count < 1) {
            revents |= POLLERR; 
		}
        else {
            switch (messageType) {
                case 100:
                    char nickname[10];
                    // for (int i = 0; i < 10; i++) {
                    //     nickname[i] = messageText[i];
                    // }
                    strncpy(nickname, messageText, 10);

                    bool nicknameUnique = isNicknameUnique(&nickname[0]);
                    // printf("%d", nicknameUnique);
                    if (nicknameUnique == true) {
                        strncpy(playersStats[clientFd].nickname, nickname, 10);
                    }
                    else {
                        snprintf(message, sizeof(message), "Nick %s jest juz zajety. Sprobuj innego.\n", nickname);
                        count = setBuffer(sendInfo, &message[0]);
                        sendToClient(clientFd, count);
                    }
            }
        }
		// else if (readBuffer[0] == currentLetter) {
		// 	write(clientFd, "Dobra robota!\n", 15);
		// }
		// else {
		// 	write(clientFd, "Niestety slowo powinno sie zaczynac na inna litere!\n", 53);
		// }

    }
    
    if(revents & ~POLLIN){
        printf("removing %d\n", clientFd);
        gamerLeaves(clientFd);
        // remove from description of watched files for poll
        descr[indexInDescr] = descr[descrCount-1];
        descrCount--;
        shutdown(clientFd, SHUT_RDWR);
        close(clientFd);
    }
}

void drawingLetter() {

	currentLetter = 'a' + rand()%26;

    char message[255];
    snprintf(message, sizeof(message), "Obecna litera: %c \n",  currentLetter);
    int count;
    count = setBuffer(sendLetter, &message[0]);
	sendToAll(count);
}

void gameManager(){
	std::chrono::steady_clock sc;
	auto startBreak = sc.now();
	auto startGame = sc.now();
	auto startRoundTime = sc.now();
    auto sendSeconds = sc.now();
    char message[255];
    int count;

	while(true) {
		auto sendSecondsSpan = static_cast<std::chrono::duration<double>>(sc.now() - sendSeconds);
		auto breakTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startBreak);
		auto gameTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startGame);

        // if (sendSecondsSpan.count() > 1) {
        //     sendSeconds = sc.now();
        //     if (gameState == 0) {
        //         snprintf(message, sizeof(message), "%f\n", breakTimeSpan.count());
        //     }
        //     else if (gameState == 1) {
        //         snprintf(message, sizeof(message), "%f\n", gameTimeSpan.count());
        //     }
        //     count = setBuffer(sendTime, &message[0]);
		// 	sendToAll(count);
        // }


		if (breakTimeSpan.count() > breakTime && gameState == 0 && gamersCounter >= 2) {
			gameState = 1;
			startGame = sc.now();
			for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd) {
					gamersState[descr[i].fd] = 1;
					gameCounter++;
            	}
			}
			lobbyCounter = 0;
            snprintf(message, sizeof(message), "Rozpoczynacie gre, aktualnie jest was %d graczy\n", gameCounter);
            count = setBuffer(sendInfo, &message[0]);
			sendToAllinGame(count);
		}
		else if ((gameTimeSpan.count() > gameTime && gameState == 1) || (gamersCounter < 2 && gameState == 1)) {
			startBreak = sc.now();
			gameState = 0;
            snprintf(message, sizeof(message), "Gra sie konczy, uzytkownicy wracaja do lobby\n");
            count = setBuffer(sendInfo, &message[0]);
			gameCounter = 0;
			sendToAllinGame(count);
            for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd) {
					gamersState[descr[i].fd] = 0;
					lobbyCounter++;
            	}
			}
            snprintf(message, sizeof(message), "Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
            count = setBuffer(sendInfo, &message[0]);
            sendToAllinLobby(count);
		}

		if (gameState == 1) {
			auto roundTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startRoundTime);
			if (roundTimeSpan.count() >= roundTime) {
				startRoundTime = sc.now();
				drawingLetter();
			}
		}
	}
}

int main(int argc, char ** argv){
    // get and validate port number
    if(argc != 2) error(1, 0, "Need 1 arg (port)");
    auto port = readPort(argv[1]);
    
    // create socket
    servFd = socket(AF_INET, SOCK_STREAM, 0);
    if(servFd == -1) error(1, errno, "socket failed");
    
    // graceful ctrl+c exit
    signal(SIGINT, ctrl_c);
    // prevent dead sockets from throwing pipe errors on write
    signal(SIGPIPE, SIG_IGN);
    
    setReuseAddr(servFd);
    
    // bind to any address and port provided in arguments
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons((short)port), .sin_addr={INADDR_ANY}};
    int res = bind(servFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) error(1, errno, "bind failed");
    
    // enter listening mode
    res = listen(servFd, 1);
    if(res) error(1, errno, "listen failed");

    descr = (pollfd*) malloc(sizeof(pollfd)*descrCapacity);
    
    descr[0].fd = servFd;
    descr[0].events = POLLIN;

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
