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

#include<stdio.h>
#include<string.h>    //strlen
#include<stdlib.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h> //inet_addr
#include<unistd.h>    //write
#include<pthread.h> //for threading , link with lpthread

#define BUFFSIZE 255

extern char* portname;
int fd = 0;

pthread_mutex_t lock;


//the thread function
void *connection_handler(void *);

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
		return 1;
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

    return 0;

}

void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char *message , client_message[BUFFSIZE];
    uint8_t transmitBuffer[BUFFSIZE];

    struct command_packet pCommandReceived;
    struct response_packet ResponseToSend;

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
		if(read_size>2){
			pCommandReceived.length = read_size;
			length = pCommandReceived.length;

			// decode packet
			printf("Read PacketSize: %ul, ReadSize: %ul", read_size, length);

			/*
			// get lock
			pthread_mutex_lock(&lock);
			// send and receive to uart
			if(!send_receive_packet(*pCommandReceived, pResponseToSend)){
				// create a standard error packet
				puts("Error sending and receiving from uart\n");
				if(pResponseToSend->pData != 0){
					free(pResponseToSend->pData);
					pResponseToSend->pData = 0;
				}
				memset(pResponseToSend, 0, BUFFSIZE);

				pResponseToSend->length = 8;   // initially 8 bytes
				pResponseToSend->command = pCommandReceived->command;
				pResponseToSend->status = 1;   // set to fail
				pResponseToSend->checksum = 0;   // will get this from the receive buffer

			}

			// unlock
			pthread_mutex_unlock(&lock);

			//Send the response back to client
			write(sock , pResponseToSend , pResponseToSend->length);

			// free data
			if(pResponseToSend->pData != 0)
				free(pResponseToSend);

			// clear the response buffer
			memset(pResponseToSend, 0, BUFFSIZE);
	*/
		}
		write(sock , client_message , strlen(client_message));

		//clear the message buffer
		memset(client_message, 0, BUFFSIZE);
    }

    if(read_size == 0)
    {
        puts("Client disconnected");
        fflush(stdout);
    }
    else if(read_size == -1)
    {
        perror("recv failed");
    }

    return 0;
}

