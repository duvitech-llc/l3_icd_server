  /*****************************************************************************
  * @file    d3_command_proc.c
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 22, 2015
  * @brief   Contains the implementation of the serial command processor.
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

/* Includes ------------------------------------------------------------------*/
#include "d3_command_proc.h"
#include <errno.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <string.h>
#include <assert.h>
#include <malloc.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

#define BUFFSIZE 255

/* Private variables ---------------------------------------------------------*/
volatile int STOP=FALSE;

int wait_flag=TRUE;                     //TRUE while no signal received
char devicename[80];
long Baud_Rate = 115200;         // default Baud Rate
long BAUD;                      // derived baud rate from command line
long DATABITS;
long STOPBITS;
long PARITYON;
long PARITY;
int Data_Bits = 8;              // Number of data bits
int Stop_Bits = 1;              // Number of stop bits
int Parity = 0;                 // Parity as follows:
                  // 00 = NONE, 01 = Odd, 02 = Even, 03 = Mark, 04 = Space
int Format = 4;
FILE *input;
FILE *output;
int status;
int mPort;

/* Private function prototypes -----------------------------------------------*/

void signal_handler_IO (int status);    //definition of signal handler
uint16_t calculate_sum16 (const uint16_t *pBuffer, uint16_t length);
uint16_t calculate_checksum16 (const uint16_t *pBuffer, uint16_t length);

//void error_message(const char *format, ...);

/* Private functions ---------------------------------------------------------*/

uint16_t calculate_sum16 (const uint16_t *pBuffer, uint16_t length)
{
	uint16_t	sum;
	uint16_t    count;

	assert (pBuffer != NULL);
	assert (length != 0);

	for (sum = 0, count = 0; count < length; count++) {
		sum = (uint16_t) (sum + (uint16_t)(pBuffer[count]));
	}

	return sum;
}

uint16_t calculate_checksum16 (const uint16_t *pBuffer, uint16_t length)
{
	uint16_t	checkSum;
	checkSum = calculate_sum16(pBuffer, length);

	return (uint16_t)(0x10000 - checkSum);
}

int open_port (char* portname){
	if(mPort != 0)
		close(mPort);

	if(portname == NULL){
		// open default port
		mPort = open (SERIAL_DEVICE, O_RDWR | O_NOCTTY | O_SYNC);
	}
	else
	{
		// open provided port
		mPort = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	}

	if(mPort < 0)
		return mPort;

	set_interface_attribs (mPort, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (mPort, 0);                // set no blocking

	return mPort;
}


int close_port (){
	int iRet = 0;
	if(mPort){
		iRet = close(mPort);
		mPort = 0;
	}

	return iRet;
}

int set_interface_attribs (int fd, int speed, int parity)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf ("error %d from tcgetattr %s\n", errno, strerror(errno) );
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
									// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
									// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		printf ("error %d from tcsetattr %s\n", errno, strerror(errno));
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		printf ("error %d from tggetattr %s\n", errno , strerror(errno));
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	printf ("error %d setting term attributes %s\n", errno , strerror(errno));
}

bool send_receive_buffer(uint8_t* pSendBuffer, int sendSize, uint8_t* pReceiveBuffer, int* pRecSize){
	int nSent = 0;
	int respLen = 0;
	int nRec = 0;
	int x = 0;

	uint8_t rxBuffer[2];

	// clear buffer
	memset(rxBuffer, 0, 2);

	// if we have a send buffer and a size > 0 do it
	if(sendSize>0 && pSendBuffer){
		// send buffer provided
		nSent = write(mPort, pSendBuffer, sendSize);
		if(nSent != sendSize){
			// failed to send
			printf("Error send bytes do not match length\n");
			return false;
		}
		else
		{
			// receive response
			respLen = 2;
			for (x = 0; x < respLen; x++){
				if(x<2){
					nRec = read(mPort, &(rxBuffer[x]), 1);
				}else{
					if(x < respLen)
						nRec = read(mPort, &(pReceiveBuffer[x]), 1);
					else{
						// error to many bytes comming in
						if(pReceiveBuffer)
							free(pReceiveBuffer);
						pReceiveBuffer = 0;
						*pRecSize = 0;
						return false;
					}
				}

				if(nRec != 1){
					printf("Failed Read Bytes: %d\n", nRec);
					respLen = 0;
				}
				else{
					printf("0x%X ", rxBuffer[x]);
				}

				if(x == 1){
					respLen = (uint16_t)((rxBuffer[1]<<8)|rxBuffer[0]);
					printf("Receive Buffer Sizce: %d\n", respLen);
					pReceiveBuffer = malloc(respLen);
					memcpy(pReceiveBuffer, rxBuffer, 2);
				}
			}

			*pRecSize = respLen;
			return true;

		}

	}

	return false;
}

bool send_receive_packet(struct command_packet sendPacket, struct response_packet* pResponse){
	uint8_t* txBuffer = 0;
	bool bRet = true;

	memset(pResponse, 0, BUFFSIZE);

	pResponse->length = 8;   // initially 8 bytes
	pResponse->command = sendPacket.command;
	pResponse->status = 1;   // set to fail
	pResponse->checksum = 0;   // will get this from the receive buffer

	txBuffer = (uint8_t*)malloc(sendPacket.length);
	if(txBuffer != 0){
		uint8_t* rxBuffer = 0;
		int nSent = 0;
		int nRec = 0;
		int x = 0;
		uint16_t respLen = 0;
		uint16_t cksum = 0;

		uint8_t* pBuffer = txBuffer;
		memset(txBuffer, 0, sendPacket.length);
		// fill tx buffer

		memcpy(pBuffer, (uint8_t*)&sendPacket.length, 2);	// copy length word
		pBuffer+=2;
		memcpy(pBuffer, (uint8_t*)&sendPacket.command, 2);	// copy command word
		pBuffer+=2;
		if(sendPacket.pData){
			memcpy(pBuffer, (uint8_t*)sendPacket.pData, sendPacket.length - 6);	// copy data words
		}
		// set pointer checksum
		pBuffer+=(sendPacket.length -2);
		// calculate checksum and add
		cksum = calculate_checksum16((uint16_t*)txBuffer, sendPacket.length/2 - 1);
		memcpy(pBuffer, (uint8_t*)&cksum, 2);	// copy checksum word
		pBuffer = 0;

		// get byte pointer for send
		nSent = write(mPort, txBuffer, sendPacket.length);

		// check bytes sent
		if(nSent != sendPacket.length){
			// issue
			printf("Error send bytes do not match length\n");
			bRet = false;
		}else{
			printf("Packet Sent\n");
		}

		usleep ((nSent + 25) * 100);             // sleep enough to transmit the 7 plus
			                                     // receive 25:  approx 100 uS per char transmit
		// free allocated memory
		free(txBuffer);
		txBuffer = 0;
		if(!bRet){
			return bRet;
		}

		printf("Awaiting Response\n");
		// Receive Response packet
		// setup receive buffer
		rxBuffer = (uint8_t*)malloc(200);
		respLen = 2;
		for (x = 0; x < respLen; x++){
			nRec = read(mPort, &(rxBuffer[x]), 1);
			if(nRec != 1){
				printf("Failed Read Bytes: %d\n", nRec);
				respLen = 0;
			}
			else{
				printf("0x%X ", rxBuffer[x]);
			}

			if(x == 1){
				respLen = (uint16_t)((rxBuffer[1]<<8)|rxBuffer[0]);
			}
		}

		if(respLen >0){
			printf("Response Creating Packet\n");
			// set response data and calculate checksum and verify data
			cksum = calculate_checksum16((uint16_t*)rxBuffer, respLen/2 - 1);
			uint16_t* pTemp = (uint16_t*)rxBuffer;
			pResponse->length = respLen; // length (command is already set
			pTemp= (uint16_t*)(&rxBuffer[2]);
			pResponse->command = *pTemp;
			pTemp= (uint16_t*)(&rxBuffer[4]);
			pResponse->status = *pTemp;
			pResponse->pData = 0;

			if((respLen-2)-6>0){		// 6 to len-2
				int dLen = (respLen-2)-6;
				pResponse->pData = 0;
				pResponse->pData = malloc(dLen);
				int x = 0;
				int y = 0;
				for(y = 0; y < dLen; y+=2){
					pTemp = (uint16_t*)(&rxBuffer[6 + y]);
					((uint16_t*)pResponse->pData)[x] = *pTemp;
					x++;
				}

			}else{
				pResponse->pData = 0;
			}

			pTemp= (uint16_t*)(&rxBuffer[respLen-2]);
			pResponse->checksum = *pTemp;
			if(pResponse->checksum == cksum && pResponse->status == 0)
				pResponse->status = 0;
			else
				pResponse->status = 1;

		}else{
			printf("Response Failed\n");
			// make any changes to the fail response message
		}

		if(rxBuffer){
			free(rxBuffer);
			rxBuffer = 0;
		}
	}

	return bRet;
}

/***************************************************************************
* signal handler. sets wait_flag to FALSE, to indicate above loop that     *
* characters have been received.                                           *
***************************************************************************/

void signal_handler_IO (int status)
{
   printf("received SIGIO signal.\n");
   wait_flag = FALSE;
}

