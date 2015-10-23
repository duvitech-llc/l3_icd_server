  /*****************************************************************************
  * @file    nanocore_control_if.c
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 23, 2015
  * @brief   This file contains the headers for the nanocore ICD.
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
#include "nanocore_control_if.h"
#include "d3_command_proc.h"
#include <stdio.h>
#include <malloc.h>

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

bool set_reticle(enum enumReticle_Ids reticleId){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	uint16_t rId = reticleId;
	pSendPacket.length = 0x0008;
	pSendPacket.command = CMD_SET_RETICLE;
	pSendPacket.pData = (uint16_t*)&rId;
	pSendPacket.checksum = 0x0000;

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);

	return bSuccess;
}

bool set_color_mode(enum enumColor_Mode colorMode){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	uint16_t mColor = colorMode;
	pSendPacket.length = 0x0008;
	pSendPacket.command = CMD_SET_COLOR_MODE;
	pSendPacket.pData = (uint16_t*)&mColor;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool get_color_mode(enum enumColor_Mode* pColorMode){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	pSendPacket.length = 0x0006;
	pSendPacket.command = CMD_GET_COLOR_MODE;
	pSendPacket.pData = 0;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	if(bSuccess){
		// get the data
		if(pRespPacket.pData){
			*pColorMode = *pRespPacket.pData;
		}
		else {
			// error no data
			printf("Error no data returned\n");
			bSuccess = false;
		}

	}

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool set_ezoom(struct typeEzoom_Params params){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	if(params.H_OFFSET <0 || params.H_OFFSET > 319 ||
			params.H_ZOOM < 1 || params.H_ZOOM > 1000 ||
			params.V_OFFSET <0 || params.V_OFFSET > 319 ||
			params.V_ZOOM < 1 || params.V_ZOOM > 1000){
		printf("Invalid parameter values\n");
		return bSuccess;
	}

	pSendPacket.length = 0x000E;
	pSendPacket.command = CMD_SET_EZOOM;
	pSendPacket.pData = (uint16_t*)&params;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool get_ezoom(struct typeEzoom_Params* pParams){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	pSendPacket.length = 0x0006;
	pSendPacket.command = CMD_GET_EZOOM;
	pSendPacket.pData = 0;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	if(bSuccess){
		// get the data
		if(pRespPacket.pData){
			// populate structure
			uint16_t* pTemp = (uint16_t*)pRespPacket.pData;
			(*pParams).H_OFFSET = (uint16_t)*pTemp;
			pTemp++;
			(*pParams).V_OFFSET = (uint16_t)*pTemp;
			pTemp++;
			(*pParams).H_ZOOM = (uint16_t)*pTemp;
			pTemp++;
			(*pParams).V_ZOOM = (uint16_t)*pTemp;
		}
		else {
			// error no data
			printf("Error no data returned\n");
			bSuccess = false;
		}

	}

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool set_battery_type(enum enumBattery_Type batteryType){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	uint16_t mBattery = batteryType;
	pSendPacket.length = 0x0008;
	pSendPacket.command = CMD_SET_BAT_TYPE;
	pSendPacket.pData = (uint16_t*)&mBattery;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool get_battery_type(enum enumBattery_Type* pBatteryType){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	pSendPacket.length = 0x0006;
	pSendPacket.command = CMD_GET_BAT_TYPE;
	pSendPacket.pData = 0;
	pSendPacket.checksum = 0x0000; // gets calculated

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	if(bSuccess){
		// get the data
		if(pRespPacket.pData){
			*pBatteryType = *pRespPacket.pData;
		}
		else {
			// error no data
			printf("Error no data returned\n");
			bSuccess = false;
		}

	}

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);
	return bSuccess;
}

bool update_firmware(){
	bool bSuccess = false;
	struct command_packet pSendPacket;
	struct response_packet pRespPacket;

	pSendPacket.length = 0x0006;
	pSendPacket.command = CMD_UPDATE_FW;
	pSendPacket.pData = 0;
	pSendPacket.checksum = 0x0000;

	send_receive_packet(pSendPacket, &pRespPacket);

	if(pRespPacket.status == 0)
		bSuccess = true;

	// always check pData because it will be a memory leak
	if(pRespPacket.pData)
		free(pRespPacket.pData);

	return bSuccess;
}

