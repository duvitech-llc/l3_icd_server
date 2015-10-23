  /******************************************************************************
  * @file    main.c
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 22, 2015
  * @brief   Test Program .
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

#include "d3_command_proc.h"
#include "nanocore_control_if.h"
#include "d3_tcp_svr.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>

bool bTest = false;
bool bSilent = false;
char* portname = "/dev/ttyS1";

int main(int argc, char *argv[]){

	if(argc > 1){
		if(strcmp(argv[1], "-test")==0){
			bTest = true;
		}else if(strcmp(argv[1], "-quiet")==0){
			bSilent = true;
		}
	}

	printf("L3 ICD Server\n\n");
	if(!bTest){
	printf("TCP Server Program\n");
		start_tcp_listener();
	}else{
		printf("Serial Test Program\n");
		int fd = open_port(portname);
		if(fd<0){
			printf("error %d opening port\n", errno);
			return 1;
		}else
		{
			printf("COM Port opened\n", errno);
		}

		// reticle testing
		printf("Set Reticle Test\n");
		if(!set_reticle(RETICLE_E)){
			printf("\nReticle Test Failed\n");
		}else
		{
			printf("\nReticle Test Passed\n");
		}

		// color mode testing
		printf("Set Color Mode Test\n");
		if(!set_color_mode(HOT_TGT_LVL2)){
			printf("\nSet Color Mode Test Failed\n");
		}else
		{
			printf("\nSet Color Mode Test Passed\n");
		}

		printf("Get Color Mode Test\n");
		enum enumColor_Mode colorMode;
		if(!get_color_mode(&colorMode)){
			printf("\nGet Color Mode Test Failed\n");
		}else
		{
			printf("\nGet Color Mode Test Passed\n");
		}


		// EZoom testing
		printf("Set EZoom Test\n");
		struct typeEzoom_Params settings;
		settings.H_OFFSET = 220;
		settings.H_ZOOM = 53;
		settings.V_OFFSET = 110;
		settings.V_ZOOM = 15;

		if(!set_ezoom(settings)){
			printf("\nSet EZoom Test Failed\n");
		}else
		{
			printf("\nSet EZoom Test Passed\n");
		}


		printf("Get EZoom Test\n");
		settings.H_OFFSET = 0;
		settings.H_ZOOM = 0;
		settings.V_OFFSET = 0;
		settings.V_ZOOM = 0;

		if(!get_ezoom(&settings)){
			printf("\nGet EZoom Test Failed\n");
		}else
		{
			printf("\nGet EZoom Test Passed\n");
		}


		close_port();

		printf("\nSerial Test Ending\n");
	}
	return 0;
}

