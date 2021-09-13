#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <3ds.h>
#include <unistd.h> /* read, write, close */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h> /* struct hostent, gethostbyname */
#include <malloc.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <arpa/inet.h>
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
#define NUMTHREADS 3
#define STACKSIZE (4 * 1024)

int useragent = "Mozilla/5.0 (New Nintendo 3DS like iPhone) AppleWebKit/536.30 (KHTML, like Gecko) NX/3.0.0.5.22 Mobile NintendoBrowser/1.10.10166.US";
int debug = false;
int size = 0;
int code = 0;
bool finished = false;
volatile bool runThreads = true;

static u32* SOC_buffer = NULL;
s32 sock = -1, csock = -1;

__attribute__((format(printf, 1, 2)))
void failExit(const char* fmt, ...);

const static char http_200[] = "HTTP/1.1 200 OK\r\n";

const static char indexdata[] = "<html> \
                               <head><title>A test page</title></head> \
                               <body> \
                               This small test page has had %d hits. \
                               </body> \
                               </html>";

const static char http_html_hdr[] = "Content-type: text/html\r\n\r\n";
const static char http_get_index[] = "GET / HTTP/1.1\r\n";


//---------------------------------------------------------------------------------
void socShutdown() {
	//---------------------------------------------------------------------------------
	printf("waiting for socExit...\n");
	socExit();

}

//---------------------------------------------------------------------------------
int startServer() {
	//---------------------------------------------------------------------------------
	int ret;

	u32	clientlen;
	struct sockaddr_in client;
	struct sockaddr_in server;
	char temp[1026];
	static int hits = 0;
	// register gfxExit to be run when app quits
	// this can help simplify error handling
	atexit(gfxExit);

	// allocate buffer for SOC service
	SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_buffer == NULL) {
		failExit("memalign: failed to allocate\n");
	}

	// Now intialise soc:u service
	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0) {
		failExit("socInit: 0x%08X\n", (unsigned int)ret);
	}

	// register socShutdown to run at exit
	// atexit functions execute in reverse order so this runs before gfxExit
	atexit(socShutdown);

	// libctru provides BSD sockets so most code from here is standard
	clientlen = sizeof(client);

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock < 0) {
		failExit("socket: %d %s\n", errno, strerror(errno));
	}

	memset(&server, 0, sizeof(server));
	memset(&client, 0, sizeof(client));

	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	server.sin_addr.s_addr = gethostid();

	printf("Point your browser to http://%s/\n", inet_ntoa(server.sin_addr));

	if ((ret = bind(sock, (struct sockaddr*) & server, sizeof(server)))) {
		close(sock);
		failExit("bind: %d %s\n", errno, strerror(errno));
	}

	// Set socket non blocking so we can still read input to exit
	fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);

	if ((ret = listen(sock, 5))) {
		failExit("listen: %d %s\n", errno, strerror(errno));
	}

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		csock = accept(sock, (struct sockaddr*) & client, &clientlen);

		if (csock < 0) {
			if (errno != EAGAIN) {
				failExit("accept: %d %s\n", errno, strerror(errno));
			}
		}
		else {
			// set client socket to blocking to simplify sending data back
			fcntl(csock, F_SETFL, fcntl(csock, F_GETFL, 0) & ~O_NONBLOCK);
			printf("Connecting port %d from %s\n", client.sin_port, inet_ntoa(client.sin_addr));
			memset(temp, 0, 1026);

			ret = recv(csock, temp, 1024, 0);

			if (!strncmp(temp, http_get_index, strlen(http_get_index))) {
				hits++;

				send(csock, http_200, strlen(http_200), 0);
				send(csock, http_html_hdr, strlen(http_html_hdr), 0);
				sprintf(temp, indexdata, hits);
				send(csock, temp, strlen(temp), 0);
			}

			close(csock);
			csock = -1;

		}

		u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;
	}

	close(sock);

	return 0;
}

//---------------------------------------------------------------------------------
void failExit(const char* fmt, ...) {
	//---------------------------------------------------------------------------------

	if (sock > 0) close(sock);
	if (csock > 0) close(csock);

	va_list ap;

	printf(CONSOLE_RED);
	va_start(ap, fmt);
	vprintf(fmt, ap);
	va_end(ap);
	printf(CONSOLE_RESET);
	printf("\nPress B to exit\n");

	while (aptMainLoop()) {
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();
		if (kDown & KEY_B) exit(0);
	}
}
void threadMain(void* arg)
{
	u64 sleepDuration = 1000000ULL * (u32)arg;
	int i = 0;
	while (runThreads)
	{
		startServer();
	}
}

int startThread()
{
	Thread threads[NUMTHREADS];
	int i;
	s32 prio = 0;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	if (debug) {
		printf("Main thread prio: 0x%lx\n", prio);
	}

	for (i = 0; i < NUMTHREADS; i++)
	{
		// The priority of these child threads must be higher (aka the value is lower) than that
		// of the main thread, otherwise there is thread starvation due to stdio being locked.
		threads[i] = threadCreate(threadMain, (void*)((i + 1) * 250), STACKSIZE, prio - 1, -2, false);
		if (debug) {
			printf("created thread %d: %p\n", i, threads[i]);
		}
	}
	// tell threads to exit & wait for them to exit
	runThreads = false;
	for (i = 0; i < NUMTHREADS; i++)
	{
		threadJoin(threads[i], U64_MAX);
		threadFree(threads[i]);
	}
}

int writeFile(int location, int content) {
	FILE* pFile;
	pFile = fopen(location, "w");
	if (pFile != NULL)
	{
		fputs(content, pFile);
		fclose(pFile);
	}
}
int readFile(int location) {
	u8* buffer;
	FILE* file = fopen(location, "rb");

	// seek to end of file
	//fseek(file, 0, SEEK_END);

	// file pointer tells us the size
	off_t size = ftell(file);

	// seek back to start
	//fseek(file, 0, SEEK_SET);

	//allocate a buffer
	buffer = malloc(size);

	//read contents !
	off_t bytesRead = fread(buffer, 1, size, file);

	//close the file because we like being nice and tidy
	fclose(file);
	return buffer;
}
static SwkbdCallbackResult MyCallback(void* user, const char** ppMessage, const char* text, size_t textlen)
{
	if (strstr(text, "lenny"))
	{
		*ppMessage = "Nice try but I'm not letting you use that meme right now";
		return SWKBD_CALLBACK_CONTINUE;
	}

	if (strstr(text, "brick"))
	{
		*ppMessage = "~Time to visit Brick City~";
		return SWKBD_CALLBACK_CLOSE;
	}

	return SWKBD_CALLBACK_OK;
}
Result http_download(const char* url)
{
	Result ret = 0;
	httpcContext context;
	char* newurl = NULL;
	u8* framebuf_top;
	u32 statuscode = 0;
	u32 contentsize = 0, readsize = 0, size = 0;
	u8* buf, * lastbuf;

	if (debug) {
		printf("Downloading %s\n", url);
	}

	do {
		ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url, 1);
		if (debug) {
			printf("return from httpcOpenContext: %" PRId32 "\n", ret);
		}

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);
		if (debug) {
			printf("return from httpcSetSSLOpt: %" PRId32 "\n", ret);
		}

		// Enable Keep-Alive connections
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);
		if (debug) {
			printf("return from httpcSetKeepAlive: %" PRId32 "\n", ret);
		}

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", useragent);
		if (debug) {
			printf("return from httpcAddRequestHeaderField: %" PRId32 "\n", ret);
		}

		// Tell the server we can support Keep-Alive connections.
		// This will delay connection teardown momentarily (typically 5s)
		// in case there is another request made to the same server.
		ret = httpcAddRequestHeaderField(&context, "Connection", "Keep-Alive");
		if (debug) {
			printf("return from httpcAddRequestHeaderField: %" PRId32 "\n", ret);
		}

		ret = httpcBeginRequest(&context);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if (ret != 0) {
			httpcCloseContext(&context);
			if (newurl != NULL) free(newurl);
			return ret;
		}
		if (statuscode = 153600) {
			debug = true;
			debugError();
		}
		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) {
			if (newurl == NULL) newurl = (char*)malloc(0x1000); // One 4K page for new URL
			if (newurl == NULL) {
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned
			if (debug) {
				printf("redirecting to url: %s\n", url);
			}
			httpcCloseContext(&context); // Close this context before we try the next
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	if (statuscode != 200) {
		if (debug) {
			//printf("URL returned status: %" PRId32 "\n", statuscode);
		}
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -2;
	}
	
	// This relies on an optional Content-Length header and may be 0
	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return ret;
	}

	if (debug) {
		printf("reported size: %" PRId32 "\n", contentsize);
	}
	size = contentsize;

	// Start with a single page buffer
	buf = (u8*)malloc(0x1000);
	if (buf == NULL) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		return -1;
	}
	if (debug) {
		printf(httpcDownloadData(&context, buf + size, 0x1000, &readsize));
	}
	do {
		// This download loop resizes the buffer as data is read.
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING) {
			lastbuf = buf; // Save the old pointer, in case realloc() fails.
			buf = (u8*)realloc(buf, size + 0x1000);
			if (buf == NULL) {
				httpcCloseContext(&context);
				free(lastbuf);
				if (newurl != NULL) free(newurl);
				return -1;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret != 0) {
		httpcCloseContext(&context);
		if (newurl != NULL) free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = (u8*)realloc(buf, size);
	if (buf == NULL) { // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if (newurl != NULL) free(newurl);
		return -1;
	}
	if (debug) {
		printf("downloaded size: %" PRId32 "\n", size);
	}

	if (size > (240 * 400 * 3 * 2))size = 240 * 400 * 3 * 2;

	framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(framebuf_top, buf, size);
	FILE* pFile;
	//char buffer[20];
	//std::string str(buf);
	//std::size_t length = str.copy(buffer, size);
	//buffer[length] = '\0';
	
	pFile = fopen("myfile.txt", "w");

	setvbuf(pFile, "Veryimportant", buf, size);

	// File operations here

	fclose(pFile);

	return 0;

	gfxFlushBuffers();
	gfxSwapBuffers();

	framebuf_top = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(framebuf_top, buf, size);

	gfxFlushBuffers();
	gfxSwapBuffers();
	gspWaitForVBlank();

	httpcCloseContext(&context);
	free(buf);
	if (newurl != NULL) free(newurl);
	finished = true;
	return 0;
}
int listDevices() {
	http_download("http://129.168.2.212:560/list");
}
int debugError() {
	printf("You are emulating in Citra! Network functions not available!!");
}
int checkConnection() {
	Result ret = 0;
	ret = http_download("https://www.google.de");
}
int checkDebug() {
	if (debug) {
		debugError();
	}
}
int printKeyboard() {
	//Hallo
}
int keyboard()
{
		static SwkbdState swkbd;
		static char mybuf[60];
		static SwkbdStatusData swkbdStatus;
		static SwkbdLearningData swkbdLearning;
		SwkbdButton button = SWKBD_BUTTON_NONE;
		bool didit = false;
			didit = true;
			swkbdInit(&swkbd, SWKBD_TYPE_NORMAL, 3, -1);
			swkbdSetInitialText(&swkbd, mybuf);
			swkbdSetHintText(&swkbd, "Enter track url");
			swkbdSetButton(&swkbd, SWKBD_BUTTON_LEFT, "OK", false);
			//swkbdSetButton(&swkbd, SWKBD_BUTTON_MIDDLE, "~Middle~", true);
			//swkbdSetButton(&swkbd, SWKBD_BUTTON_RIGHT, "Do It", true);
			swkbdSetFeatures(&swkbd, SWKBD_PREDICTIVE_INPUT);
			SwkbdDictWord words[2];
			//swkbdSetDictWord(&words[0], "lenny", "( ͡° ͜ʖ ͡°)");
			//swkbdSetDictWord(&words[1], "shrug", "¯\\_(ツ)_/¯");
			swkbdSetDictionary(&swkbd, words, sizeof(words) / sizeof(SwkbdDictWord));
			static bool reload = false;
			swkbdSetStatusData(&swkbd, &swkbdStatus, reload, true);
			swkbdSetLearningData(&swkbd, &swkbdLearning, reload, true);
			reload = true;
			button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
		if (didit)
		{
			if (button != SWKBD_BUTTON_NONE)
			{
				return mybuf;
			}
			else
				if (debug) {
					printf("swkbd event: %d\n", swkbdGetResult(&swkbd));
				}
		}
}

int a() {

}
int b() {

}
int y() {

}
int x() {

}
int start() {
	
}
int selectb() {

}
int setBottom() {
	consoleInit(GFX_BOTTOM, NULL);
}
int setTop() {
	consoleInit(GFX_TOP, NULL);
}
int setURL(int text) {
	
}
int debugmode() {
	printf("DEBUGMODE");
}
int main(int argc, char* argv[])
{
	gfxInitDefault();
	int text = "";
	setTop();
	checkConnection();
	checkDebug();
	if (debug) {
		debugmode();
	}
	printf("Spotify for 3DS\n\n");
	printf("A to input track\n\n");
	printf("Dummy application not functional\n\n\n");
	printf("Beta v1.0 non production ready");
	setBottom();
	// Main loop
	while (aptMainLoop())
	{
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();
		// Your code goes here
		u32 kDown = hidKeysDown();
		if (kDown & KEY_START)
			start();
		if (kDown & KEY_SELECT) {
			break;
		}
		if (kDown & KEY_A) {
			text = keyboard();
			httpcInit(0);
			Result ret = 0;
			ret = http_download(text);
			if (debug) {
				printf(ret);
			}
		}
		if (kDown & KEY_B) {
			listDevices();
		}
		if (kDown & KEY_Y) {
			if (debug) {
				startThread();
			}
		}
		if (kDown & KEY_Y) {
			x();
		}
	}

	gfxExit();
	return 0;
}


// Experimental section: NETWORKING //

Result http_download(string url,string loca)
{
	Result ret = 0;
	u32 statuscode=0;
	u32 contentsize=0, readsize=0, size=0x1000;
	char a[2048];
	string strNew;
	httpcContext context;
	extern PrintConsole top,bottom;
	consoleSelect(&top);
	cout<<"\x1b[2J";
	pBar bar;
	fs out;
	consoleSelect(&bottom);
	cout<<"\x1b[2J";
	bar.length_set(contentsize);
	bar.update(0);
	bar.print();
	consoleSelect(&top);
	cout<<"\x1b[33;1mDownloading : \x1b[37;1m"<<url<<endl;
	ret = httpcOpenContext(&context, HTTPC_METHOD_GET, url.c_str(), 0);
	if (ret != 0)
	{
		cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		return 1;
	}
	ret = httpcAddRequestHeaderField(&context, (char*)"User-Agent", (char*)"MULTIDOWNLOAD++");
	if (ret != 0)
	{
		cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		return 1;
	}
	ret = httpcSetSSLOpt(&context, 1 << 9);
	if (ret != 0)
	{
		cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		return 1;
	}
	ret = httpcBeginRequest(&context);
	if (ret != 0)
	{
		cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		return 1;
	}
	ret = httpcGetResponseStatusCodeTimeout(&context, &statuscode,6000000000);
	if (ret != 0)
	{	cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		cout<<"Wrong protocol/Internet not connected "<<url<<endl;
		httpcCloseContext(&context);
		return 1;
	}
		cout<<"\x1b[33;1mStatuscode:\x1b[37;1m"<<statuscode<<endl;
	if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308)) 
	{
		char newurl[1024];
		httpcGetResponseHeader(&context, (char*)"Location", newurl, 1024);
		httpcCloseContext(&context);
		return http_download(newurl,loca);
	}
	if (statuscode != 200)
		return 1;
	ret=httpcGetResponseHeader(&context, (char*)"Content-Disposition", a, 2048);
	if(ret!=0)
	{
		int pos = url.find_last_of("/");
		strNew = url.substr(pos+1);
	}	
	else
	{
		string contents(a,strlen(a));
		int pos = contents.find("=");
		strNew = contents.substr (pos+1);
	}
	if(strNew.find_first_of("\"")!= string::npos)
	{
		unsigned first=strNew.find_first_of("\"");
		unsigned last=strNew.find_last_of("\"");
		strNew = strNew.substr(first+1,last-first-1);
	}	
	cout <<"\x1b[33;1mFileName : \x1b[37;1m"<<strNew<<endl;
	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret != 0)
	{
		cout<<"\x1b[31;1mError 0x"<<hex<<setw(8)<<ret<<endl;
		return 1;
	}
	cout<<"\x1b[33;1msize(may be wrong) : \x1b[37;1m"<<contentsize<<endl;
	gfxFlushBuffers();
	gfxSwapBuffers();
	
	//We'll now check for invalid characters in filename(unlikely to occur)
	if(strNew.find("/")!=string::npos||strNew.find("#")!=string::npos||strNew.find("\\")!=string::npos||strNew.find(":")!=string::npos||strNew.find("*")!=string::npos
	||strNew.find("?")!=string::npos||strNew.find("\"")!=string::npos||strNew.find("<")!=string::npos||strNew.find(">")!=string::npos||strNew.find("|")!=string::npos
	||strNew.find("%")!=string::npos||strNew.empty())
	{
		strNew = tl(1,"Please enter the filename along with the extension.Eg:-multi.zip,k.3dsx,file.pdf,etc.","OK");
		cout<<strNew<<endl;
	}
	string location=loca + strNew;
	consoleSelect(&top);
	cout<<"\r\x1b[33;1mFile saved as : \x1b[37;1m"<<location<<endl;
	out.openfile(location);
	bar.length_set(contentsize);
	consoleSelect(&bottom);
	bar.print();
	do
	{
		u8 *buf = (u8*)linearAlloc(size);
		memset(buf, 0, size);
		ret = httpcDownloadData(&context, buf, 0x1000, &readsize);
		size += readsize;
		if(contentsize!=0){
		bar.update(readsize);
		bar.print();
		}
		else{
			;
		}
        out.writefile((const char*)buf,(size_t)readsize);
		linearFree(buf);
    }while(ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);
	out.closefile();
	consoleSelect(&top);
	if(ret!=0){
		httpcCloseContext(&context);
		return 1;
	}

	if((location.find(".zip")!=string::npos)||(location.find(".rar")!=string::npos)||(location.find(".tar")!=string::npos))
	{	cout<<"Archive file found"<<endl;
		extract(location);
	}
	httpcCloseContext(&context);
	return 0;
}

// End of Experimental section //
