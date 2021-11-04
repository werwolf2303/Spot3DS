#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <3ds.h>
#include <ctime>

#include <malloc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>

//Needed to remove compiler errors -- makefile was also changed
#define _GLIBCXX_USE_CXX11_ABI 0/1
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

int sendURL(const char* urls)
{
	//gfxSet3D(true); //Uncomment if using stereoscopic 3D
	consoleInit(GFX_TOP, NULL); //Change this line to consoleInit(GFX_BOTTOM, NULL) if using the bottom screen.
	//unsigned long int t = static_cast<unsigned long int> (std::time(NULL));

	int sock, bytes_recieved, ret, numberOfChatMessages;
    std::vector<std::string> chatMessages;

	char send_data[256], recv_data[256], recv_data_holder[256];
	send_data[0] = recv_data[0] = recv_data_holder[0] = 0;

	struct hostent *host;
	struct sockaddr_in server_addr;
	static u32 *SOC_buffer = NULL;
    numberOfChatMessages = 0;
    // allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if(SOC_buffer == NULL) {
        gfxExit();
        socExit();
        return 0;
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
        gfxExit();
        socExit();
        return 0;
	}

    sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock == -1) {
        gfxExit();
        socExit();
        return 0;
	}

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = inet_addr("192.168.2.212");

	if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        socExit();
        return 0;
	}

	if(fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
        socExit();
        return 0;
	}

    strcpy(send_data, urls);
    send(sock, send_data, (strlen(send_data) + 1), 0);
	socExit();
	return 0;
}