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

// handles SIGINT
void ctrl_c(int);

// sends data to clientFds excluding fd
void sendToAllBut(int fd, char * buffer, int count);

// converts cstring to port
uint16_t readPort(char * txt);

// sets SO_REUSEADDR
void setReuseAddr(int sock);


void sendToAll(char * buffer, int count){
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

void sendToAllinGame(char * buffer, int count){
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

void sendToAllinLobby(char * buffer, int count){
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
	char buffer[255];
    if (gamersState[clientFd] == 0) {
        lobbyCounter--;
        snprintf(buffer, sizeof(buffer), "Gracz wyszedl! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
	    sendToAllinLobby(buffer, 56);
    }
    else if (gamersState[clientFd] == 1) {
        gameCounter--;
        snprintf(buffer, sizeof(buffer), "Gracz wyszedl! Aktualnie w grze znaduje sie %d graczy.\n", gameCounter);
        sendToAll(buffer, 56);
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
		char buffer[255];
		snprintf(buffer, sizeof(buffer), "Nowy gracz! Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
		sendToAllinLobby(buffer, 54);
        
        printf("new connection from: %s:%hu (fd: %d)\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), clientFd);
    }
}

void eventOnClientFd(int indexInDescr) {
    auto clientFd = descr[indexInDescr].fd;
    auto revents = descr[indexInDescr].revents;
    
    if(revents & POLLIN){
        char buffer[255];
        int count = read(clientFd, buffer, 255);
		if(count < 1) {
            revents |= POLLERR; 
		}
		else if (buffer[0] == currentLetter) {
			write(clientFd, "Dobra robota!\n", 15);
		}
		else {
			write(clientFd, "Niestety slowo powinno sie zaczynac na inna litere!\n", 53);
		}

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
    char buffer[17] = "Obecna litera: ";
	char letter = 'a' + rand()%26;
	currentLetter = letter;
	buffer[15] = NULL;
	buffer[15] = letter;
	buffer[16] = '\n';
	sendToAll(buffer, 17);
}

void gameManager(){
	std::chrono::steady_clock sc;
	auto startBreak = sc.now();
	auto startGame = sc.now();
	auto startRoundTime = sc.now();

	while(true) {
		auto breakTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startBreak);
		auto gameTimeSpan = static_cast<std::chrono::duration<double>>(sc.now() - startGame);
		char buffer[255];
		if (breakTimeSpan.count() > breakTime && gameState == 0 && gamersCounter >= 2) {
			gameState = 1;
			startGame = sc.now();
			for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd) {
					gamersState[descr[i].fd] = 1;
					gameCounter++;
            	}
			}
			snprintf(buffer, sizeof(buffer), "Rozpoczynace gre, aktualnie jest was %d graczy\n", gameCounter);
			lobbyCounter = 0;
			sendToAllinGame(buffer, 48);
		}
		else if ((gameTimeSpan.count() > gameTime && gameState == 1) || (gamersCounter < 2 && gameState == 1)) {
			startBreak = sc.now();
			gameState = 0;
            snprintf(buffer, sizeof(buffer), "Gra sie konczy, uzytkownicy wracaja do lobby\n");
			gameCounter = 0;
			sendToAllinGame(buffer, 46);
            for(int i = 0 ; i < descrCount; ++i){
                if(descr[i].fd != servFd) {
					gamersState[descr[i].fd] = 0;
					lobbyCounter++;
            	}
			}
            snprintf(buffer, sizeof(buffer), "Aktualnie w lobby znaduje sie %d graczy.\n", lobbyCounter);
            sendToAllinLobby(buffer, 42);
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


