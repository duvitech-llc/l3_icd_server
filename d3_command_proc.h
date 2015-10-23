  /******************************************************************************
  * @file    d3_command_proc.h
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 22, 2015
  * @brief   This file contains the headers for the serial command processor.
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


/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef D3_SERIAL_D3_COMMAND_PROC_H_
#define D3_SERIAL_D3_COMMAND_PROC_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
struct command_packet{
	uint16_t 	length;
	uint16_t 	command;
	uint16_t* 	pData;
	uint16_t 	checksum;
};

struct response_packet{
	uint16_t 	length;
	uint16_t 	command;
	uint16_t 	status;
	uint16_t* 	pData;
	uint16_t 	checksum;
};

/* Exported constants --------------------------------------------------------*/
#define BAUD_RATE B115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY_BIT 0		// 00 = NONE, 01 = Odd, 02 = Even, 03 = Mark, 04 = Space
#define SERIAL_DEVICE "/dev/ttyS0"
#define _POSIX_SOURCE 1 //POSIX compliant source
#define FALSE 0
#define TRUE 1

# define CIBAUD	  002003600000		/* input baud rate (not used) */
# define CMSPAR   010000000000		/* mark or space (stick) parity */
# define CRTSCTS  020000000000		/* flow control */
/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
int set_interface_attribs (int fd, int speed, int parity);
void set_blocking (int fd, int should_block);
int open_port (char* portname);
int close_port (void);
bool send_receive_packet(struct command_packet sendPacket, struct response_packet* pResponse);

#endif /* D3_SERIAL_D3_COMMAND_PROC_H_ */
