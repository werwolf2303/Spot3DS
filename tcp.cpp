#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <3ds.h>
#include <ctime>
#include "hbkb.h"
#include <malloc.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <fstream>

//Needed to remove compiler errors -- makefile was also changed
#define _GLIBCXX_USE_CXX11_ABI 
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000

int sendURL() {
	//unsigned long int t = static_cast<unsigned long int> (std::time(NULL));
	char send_data[256], recv_data[256], recv_data_holder[256];
	send_data[0] = recv_data[0] = recv_data_holder[0] = 0;
	struct hostent* host;
	struct sockaddr_in server_addr;
	static u32* SOC_buffer = NULL;
	int sock, bytes_recieved, ret;
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_buffer == NULL) {
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

	if (sock == -1) {
		gfxExit();
		socExit();
		return 0;
	}

	consoleInit(GFX_BOTTOM, NULL);

	printf("Try connecting to server\n");
	// Define server
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = inet_addr("192.168.2.212");
	// END


	// When connection not succesful
	if (connect(sock, (struct sockaddr*) & server_addr, sizeof(server_addr)) == -1) {
		gfxExit();
		socExit();
		return 0;
	}
	// END 


	// ?? Error handler ??
	if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
		gfxExit();
		socExit();
		return 0;
	}
	// END
	printf("Connected\n");


	printf("Trying to send string\n");
	std::string dataToSend = "Thisisthespotifurl2933";
	uint32_t dataLength = htonl(dataToSend.size());
	send(sock, dataToSend.c_str(), dataToSend.size(), 0);
	//END
	printf("Passed send string function");
}

int main()
{
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL); // Bottom top screen selection
	


	//unsigned long int t = static_cast<unsigned long int> (std::time(NULL));
	char send_data[256], recv_data[256], recv_data_holder[256];
	send_data[0] = recv_data[0] = recv_data_holder[0] = 0;
	struct hostent* host;
	struct sockaddr_in server_addr;
	static u32* SOC_buffer = NULL;
	int sock, bytes_recieved, ret;
	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_buffer == NULL) {
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

	if (sock == -1) {
		gfxExit();
		socExit();
		return 0;
	}

	consoleInit(GFX_BOTTOM, NULL);

	printf("Try connecting to server\n");
	// Define server
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(8080);
	server_addr.sin_addr.s_addr = inet_addr("192.168.2.212");
	// END


	// When connection not succesful
	if (connect(sock, (struct sockaddr*) & server_addr, sizeof(server_addr)) == -1) {
		gfxExit();
		socExit();
		return 0;
	}
	// END 


	// ?? Error handler ??
	if (fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK) == -1) {
		gfxExit();
		socExit();
		return 0;
	}
	// END
	printf("Connected\n");


	printf("Trying to send string\n");
	std::string dataToSend = "Thisisthespotifurl2933";
	uint32_t dataLength = htonl(dataToSend.size());
	send(sock, dataToSend.c_str(), dataToSend.size(), 0);
	//END
	printf("Passed send string function\n");
	printf("Try to receive file\n");
	int general_socket_descriptor;
	std::fstream file;
	char buffer[1024] = {};
	int valread = read(general_socket_descriptor, buffer, 1024);
	printf("[LOG] : Data received\n");
	printf("[LOG] : Saving data to file.\n");
	file << buffer;
	close(sock);
	// INIT on screen keyboard
	HB_Keyboard sHBKB;
	// END

	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();
		u32 kDown = hidKeysDown();

		//Receive message from server
        //ret = recv(sock, recv_data, sizeof(recv_data), 0);

		if (kDown & KEY_START)
			break; //Break in order to return to hbmenu

		// Flush and swap frame-buffers
		gfxFlushBuffers();
		gfxSwapBuffers();
	}

	gfxExit();
	socExit();
	return 0;
}
