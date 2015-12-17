  /*****************************************************************************
  * @file    d3_tcp_svr.c
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 28, 2015
  * @brief   Contains the implementation for the TCP Relay Server.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2015 Duvitech </center></h2>
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */

#include "d3_tcp_svr.h"

#include "d3_command_proc.h"
#include "nanocore_control_if.h"

#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <signal.h>

#define BUFFSIZE 255
/* D3 Stack Settings
#define RECORDING_LOC_NAME 		"/home/root/recording.mp4"
#define IMAGE_LOC_NAME 			"/home/root/capture.jpg"
#define VIDEO_ENC_CODEC			"TIVidenc1 codecName=h264enc engineName=codecServer"
#define VIDEO_DEC_CODEC			"TIViddec2 codecName=h264dec engineName=codecServer"
#define VIDEO_SOURCE			"v4l2src input-src=COMPOSITE"
#define PID_LOCATION			"/var/run/gst-launch.pid"
 */



/* testing settings*/
#define RECORDING_LOC_NAME 		"/home/linuser/recording.mp4"
#define IMAGE_LOC_NAME 			"/home/linuser/capture.jpg"
#define VIDEO_ENC_CODEC			"x264enc"
#define VIDEO_DEC_CODEC			"ffdec_h264"
#define VIDEO_SOURCE			"videotestsrc"
#define PID_LOCATION			"/home/linuser/gst-launch.pid"


/* encrypt to file
 * gst-launch videotestsrc num-buffers=1000 ! video/x-raw-yuv, format=(fourcc)NV12 ! TIVidenc1 codecName=h264enc engineName=codecServer ! filesink location=output_gen_D1.264
 * gst-launch -v v4l2src always-copy=FALSE num-buffers=800 input-src=composite ! video/x-raw-yuv,format=(fourcc)NV12,width=640,height=480 ! TIVidenc1 codecName=h264enc engineName=codecServer ! filesink location=output_cap_D1.264
 * gst-launch-0.10 filesrc location=my.mp4 ! mpeg2dec ! ffmpegcolorspace ! autovideosink
 *
 */

/* common to all configurations */
#define GST_VIEW_COMMAND		"gst-launch-0.10 -e %s ! capsfilter caps=video/x-raw-yuv,format=\\(fourcc\\)NV12,width=640,height=480 ! ffmpegcolorspace ! %s ! rtph264pay pt=96 ! udpsink port=5000 host=%s > /dev/null & echo $! > %s"
#define GST_PLAY_COMMAND		"gst-launch-0.10 filesrc location= %s ! mpegtsdemux ! h264parse ! rtph264pay pt=96 ! udpsink port=5000 host=%s > /dev/null & echo $! > %s"
#define GST_REC_COMMAND			"gst-launch-0.10 -e %s ! capsfilter caps=video/x-raw-yuv,format=\\(fourcc\\)NV12,width=640,height=480 ! %s ! tee name=t t. ! queue ! rtph264pay pt=96 ! udpsink port=5000 host=%s t. ! queue ! mpegtsmux ! filesink location=%s sync=true > /dev/null & echo $! > %s"
#define GST_IMG_COMMAND			"gst-launch-0.10 -e %s num-buffers=1 ! capsfilter caps=video/x-raw-yuv,format=\\(fourcc\\)NV12,width=640,height=480 ! ffmpegcolorspace ! jpegenc !  filesink location=%s> /dev/null"


extern char* portname;
int fd = 0;
static int gst_pid = -1;

pthread_mutex_t lock;
static char gstreamCommand[440];
static char callerIP[16]={0};

//the thread function
void *connection_handler(void *);

static void stop_gst(){
	FILE *pidFile = NULL;

	pidFile = fopen(PID_LOCATION, "r");
	if(pidFile != NULL ){
	  printf("Stopping any current stream\n");
	  fscanf(pidFile, "%d",&gst_pid);
	  printf("Killing  %i\n", gst_pid);
	  kill(gst_pid, SIGKILL);

	  fclose(pidFile);
	  unlink(PID_LOCATION);
	}

	pidFile = NULL;

}

int start_tcp_listener(void){

    int socket_desc , client_sock , c;
    struct sockaddr_in server , client;

    // create mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
	{
		puts("\n mutex init failed\n");
		return 1;
	}

    // open serial port
    fd = open_port(portname);
	if(fd<0){
		puts("error opening port\n");
		//return 1;
	}else
	{
		puts("COM Port opened\n");
	}

    //Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	puts("Socket created");

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( 8024 );

    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("Bind Done");

    //Listen
    listen(socket_desc , 3);

    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
	pthread_t thread_id;


    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");

        if( pthread_create( &thread_id , NULL ,  connection_handler , (void*) &client_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }

        //Now join the thread , so that we dont terminate before the thread
        //pthread_join( thread_id , NULL);
        puts("Handler assigned");
    }

    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }

	close_port();
	pthread_mutex_destroy(&lock);
	puts("Port Closed");
    return 0;

}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int bin_cmd = 0;
    FILE *pidFile;
    int read_size;
    int status;
    uint8_t *pRecMessage , client_message[BUFFSIZE];
    uint8_t transmitBuffer[BUFFSIZE];
    int rec_size;
    struct command_packet pCommandReceived;
    struct response_packet ResponseToSend;
	pRecMessage = 0;
	rec_size = 0;
    int IP_LEN = 16;

    // clear the response buffer
	memset(transmitBuffer, 0, BUFFSIZE);
/*
    //Send some messages to the client
    message = "Greetings! I am your connection handler\n";
    write(sock , message , strlen(message));

    message = "Now type something and i shall repeat what you type \n";
    write(sock , message , strlen(message));
*/
    //Receive a message from client
    while( (read_size = recv(sock , client_message , BUFFSIZE , 0)) > 0 )
    {
    	int length;

        //end of string marker
		client_message[read_size] = '\0';
		// send the command to the UART
		if(read_size>1){					// min packet size is 6 bytes
			length = read_size;
			pidFile = NULL;
			// decode packet
			printf("Read PacketSize: %u, ReadSize: %u\n", read_size, length);
			if(pRecMessage)
				free(pRecMessage);
			pRecMessage = 0;
			rec_size = 0;
			uint8_t *pIPString = 0;
			uint8_t *pOffString = 0;
			if(strstr(client_message, "IP=") != NULL){
				// IP address received
				printf("IP Command\n");
				printf("Client MSG: %s\n", client_message);

				IP_LEN = strlen(client_message) - 2; // really -3 for IP= but accounting for null char
				printf("Size: %u\n",IP_LEN);

				// mem copy IP into ip string for use
				memset(callerIP, 0, IP_LEN);
				pIPString = strstr(client_message, "IP=");
				pIPString+=3;

				// kill any already streaming task
				stop_gst();

				// copy ip to storage
				memcpy(callerIP, pIPString, IP_LEN-1);
				pIPString = 0;

				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer
				if(callerIP != NULL){

					ResponseToSend.status = 0;   // set to pass

				}



			}else if(strstr(client_message, "OFF") != NULL){
				printf("OFF Command\n");
				printf("Client MSG: %s\n", client_message);

				pOffString = strstr(client_message, "OFF");
				// kill streaming task
				stop_gst();
				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 0;   // set to pass
				ResponseToSend.checksum = 0;   // will get this from the receive buffer



			}else if(strstr(client_message, "VIEW") != NULL){
				// kill any already streaming task
				stop_gst();

				printf("VIEW Command\n");
				printf("Client MSG: %s\n", client_message);
				memset(gstreamCommand, 0, 440);

				sprintf(gstreamCommand, GST_VIEW_COMMAND, VIDEO_SOURCE, VIDEO_ENC_CODEC, callerIP, PID_LOCATION);

				printf("gstreamer command: %s\n", gstreamCommand);
				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer

				// launch Gstreamer process
				// create new process

				if(callerIP != NULL){
					status = system(gstreamCommand);

					pidFile = fopen(PID_LOCATION, "r");
					if(pidFile != NULL ){

						  gst_pid=-1;
						  fscanf(pidFile, "%d",&gst_pid);
						  fclose(pidFile);
						  if(gst_pid>=0)
								ResponseToSend.status = 0;  // set to pass
					}
					pidFile = NULL;
				}else{
					printf("NO IPAddress set\n");
					ResponseToSend.command = 98;
				}

			}else if(strstr(client_message, "REC") != NULL){
				// kill any already streaming task
				stop_gst();

				printf("REC Command\n");
				printf("Client MSG: %s\n", client_message);
				memset(gstreamCommand, 0, 440);

				sprintf(gstreamCommand, GST_REC_COMMAND, VIDEO_SOURCE, VIDEO_ENC_CODEC, callerIP, RECORDING_LOC_NAME, PID_LOCATION);

				printf("gstreamer command: %s\n", gstreamCommand);
				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer

				// launch Gstreamer process
				// create new process

				if(callerIP != NULL){
					status = system(gstreamCommand);

					pidFile = fopen(PID_LOCATION, "r");
					if(pidFile != NULL ){
						  gst_pid=-1;
						  fscanf(pidFile, "%d",&gst_pid);
						  fclose(pidFile);
						  if(gst_pid>=0)
								ResponseToSend.status = 0;  // set to pass
					}
					pidFile = NULL;
				}else{
					printf("NO IPAddress set\n");
					ResponseToSend.command = 98;
				}

				pidFile = NULL;
			}else if(strstr(client_message, "PLAY") != NULL){
				// kill any already streaming task
				stop_gst();

				printf("PLAY Command\n");
				printf("Client MSG: %s\n", client_message);

				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer

				// check for file
				pidFile = fopen(RECORDING_LOC_NAME, "r");
				if(pidFile == NULL ){
					printf("File Not Found\n");
					ResponseToSend.command = 97;  // file not found
				}else{
					// recording exists
					pidFile = NULL;
					memset(gstreamCommand, 0, 440);

					//sprintf(gstreamCommand, GST_PLAY_COMMAND, RECORDING_LOC_NAME, VIDEO_DEC_CODEC, callerIP, PID_LOCATION);
					sprintf(gstreamCommand, GST_PLAY_COMMAND, RECORDING_LOC_NAME, callerIP, PID_LOCATION);
					printf("gstreamer command: %s\n", gstreamCommand);

					// launch Gstreamer process
					// create new process

					if(callerIP != NULL){
						status = system(gstreamCommand);

						pidFile = fopen(PID_LOCATION, "r");
						if(pidFile != NULL ){
							  gst_pid=-1;
							  fscanf(pidFile, "%d",&gst_pid);
							  fclose(pidFile);
							  if(gst_pid>=0)
									ResponseToSend.status = 0;  // set to pass
						}
						pidFile = NULL;
					}else{
						printf("NO IPAddress set\n");
						ResponseToSend.command = 98;
					}
				}
			}else if(strstr(client_message, "IMG") != NULL){
				// kill any already streaming task
				stop_gst();
				FILE *fCapture = NULL;
				printf("IMG Command\n");
				printf("Client MSG: %s\n", client_message);
				memset(gstreamCommand, 0, 440);

				sprintf(gstreamCommand, GST_IMG_COMMAND, VIDEO_SOURCE, IMAGE_LOC_NAME );

				// check if existing capture
				fCapture = fopen(PID_LOCATION, "r");
				if(fCapture != NULL ){
				  // delete old capture file
				  fclose(fCapture);
				  unlink(IMAGE_LOC_NAME);

				  fCapture = NULL;
				}


				printf("gstreamer command: %s\n", gstreamCommand);
				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer

				// launch Gstreamer process
				// create new process

			    status = system(gstreamCommand);
			    // check for jpeg
				fCapture = fopen(PID_LOCATION, "r");
				if(fCapture != NULL ){
				  // success
				  fclose(fCapture);
				  ResponseToSend.status = 0;  // set to pass
				  fCapture = NULL;
				}

			}else if(strstr(client_message, "GET") != NULL){
				// kill any already streaming task
				stop_gst();
				FILE* pImageFile = NULL;
				printf("GET Command\n");
				printf("Client MSG: %s\n", client_message);

				ResponseToSend.length = 8;   // initially 8 bytes
				ResponseToSend.command = 99;
				ResponseToSend.pData = 0;
				ResponseToSend.status = 1;   // set to fail
				ResponseToSend.checksum = 0;   // will get this from the receive buffer

				// check for file
				pImageFile = fopen(IMAGE_LOC_NAME, "r");
				if(pImageFile == NULL ){
					ResponseToSend.command = 97;  // file not found
				}else{
					// read file and add send to client
					struct stat st;
					fclose(pImageFile);
					pImageFile = NULL;
					stat(IMAGE_LOC_NAME, &st);

					ResponseToSend.length = 8 + st.st_size; // set length
					ResponseToSend.command = 96;  // image file
				    ResponseToSend.status = 0;  // set to pass
					ResponseToSend.pData = 0;  // set data
					ResponseToSend.checksum = 0;   // set checksum

				}

			}else{

				printf("Bin Command\n");
				bin_cmd = 1;
				// get lock for uart
				printf("Get UART Lock\n");
				pthread_mutex_lock(&lock);
				// send and receive to uart
				if(!send_receive_buffer(client_message, length, pRecMessage, &rec_size)){
					// create a standard error packet
					puts("Error sending and receiving from uart\n");
					ResponseToSend.length = 8;   // initially 8 bytes
					ResponseToSend.command = (uint16_t)((client_message[3]<<8)|client_message[2]);
					ResponseToSend.pData = 0;
					ResponseToSend.status = 1;   // set to fail
					ResponseToSend.checksum = 0;   // will get this from the receive buffer
				}

				// unlock uart
				pthread_mutex_unlock(&lock);

				printf("Release UART Lock\n");
			}

			printf("Send Response Back\n");
			//Send the response back to client

			if(bin_cmd > 0){
				// pass data back from UART
				write(sock , pRecMessage , rec_size);
			}else
			{
				// set response packet
				rec_size = ResponseToSend.length;
				printf("Send Response Back Size: %u\n",rec_size);
				pRecMessage = malloc(BUFFSIZE);

				write(sock , (uint8_t*)(&ResponseToSend) , 6);
				memset(pRecMessage, 0, BUFFSIZE);

				if(ResponseToSend.pData !=0 || ResponseToSend.command == 96){
					int datasize = rec_size - 8;
					uint16_t* pData = 0;
					if(ResponseToSend.command == 96){
						// copy file to socket
						FILE* pImageFile = NULL;
						pImageFile = fopen(IMAGE_LOC_NAME, "r");
						if(pImageFile != NULL ){
							size_t bytes_read = 0;
							while (!feof(pImageFile)) {
								bytes_read = fread(pRecMessage,sizeof(unsigned char), BUFFSIZE-1,pImageFile);
								pRecMessage[bytes_read] = 0;
								write(sock , pRecMessage , bytes_read);
							}

							fclose(pImageFile);
							pImageFile = NULL;
							ResponseToSend.pData = 0;
							//unlink(IMAGE_LOC_NAME);
						}
					}else{
						// set pData to the data pointer
						pData = ResponseToSend.pData;

						while(datasize >0){
							if(datasize>BUFFSIZE){
								memcpy(pRecMessage, (uint8_t*)pData, BUFFSIZE);
								write(sock , pRecMessage , BUFFSIZE);
								pData += BUFFSIZE;
								datasize -= BUFFSIZE;
							}
							else{
								memcpy(pRecMessage, (uint8_t*)pData, datasize);
								write(sock , pRecMessage , datasize);
								datasize = 0;
							}

							memset(pRecMessage, 0, BUFFSIZE);

						}
					}


					// we sent the data now free it
					if(ResponseToSend.pData != NULL){
						free(ResponseToSend.pData);
						ResponseToSend.pData = 0;
					}
					pData = 0;
				}

				write(sock , &ResponseToSend.checksum , 2);

				memset(pRecMessage, 0, BUFFSIZE);

			}

			// free data
			if(pRecMessage != 0)
				free(pRecMessage);
			pRecMessage = 0;
			rec_size = 0;
		}


		//clear the message buffer
		memset(client_message, 0, BUFFSIZE);
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
		memset(callerIP, 0, IP_LEN);
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    fflush(stdout);
    return 0;
}

