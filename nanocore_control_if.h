  /*****************************************************************************
  * @file    nanocore_control_if.h
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 23, 2015
  * @brief   Contains the interface and structures for the nanocore ICD.
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
#ifndef D3_SERIAL_NANOCORE_CONTROL_IF_H_
#define D3_SERIAL_NANOCORE_CONTROL_IF_H_

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>

/* Exported types ------------------------------------------------------------*/
enum enumReticle_Ids{
	RETICLE_A = 0x0000,
	RETICLE_B = 0x0001,
	RETICLE_C = 0x0002,
	RETICLE_D = 0x0003,
	RETICLE_E = 0x0003,
};

enum enumBattery_Type{
	ALKALINE = 0x0000,
	LITHIUM = 0x0001,
	RESERVED1 = 0x0002,
	RESERVED2 = 0x0003,
};

enum enumColor_Mode{
	WHITE_HOT_POL = 0x0000,
	BLACK_HOT_POL = 0x0001,
	HOT_TGT_LVL1 = 0x0002,
	HOT_TGT_LVL2 = 0x0003,
	CUST_COLOR = 0x0004,
};


enum enumCommands{
	CMD_SET_RETICLE = 0x0001,			// reticle command
	CMD_SET_COLOR_MODE = 0x0002,		// set color mode
	CMD_GET_COLOR_MODE = 0x0003,		// get color mode
	CMD_SET_EZOOM = 0x0004,				// set ezoom mode
	CMD_GET_EZOOM = 0x0005,				// get ezoom mode
	CMD_SET_BAT_TYPE = 0x0006,			// set battery type
	CMD_GET_BAT_TYPE = 0x0007,			// get battery type
	CMD_UPDATE_FW = 0x0008,				// update wifi sw from file
};

struct typeEzoom_Params{
	uint16_t H_OFFSET;					// Horizontal offset in display pixels
	uint16_t V_OFFSET;					// Vertical offset in display pixels
	uint16_t H_ZOOM;					// Horizontal zoom value multiplied by 100
	uint16_t V_ZOOM;					// Vertical zoom value multiplied by 100
};

/* Exported constants --------------------------------------------------------*/
#define ICD_CMD_PASS	0
#define ICD_CMD_FAIL 	1

/* Exported macro ------------------------------------------------------------*/
/* Exported functions --------------------------------------------------------*/
bool set_reticle(enum enumReticle_Ids reticleId);
bool set_color_mode(enum enumColor_Mode colorMode);
bool get_color_mode(enum enumColor_Mode* pColorMode);
bool set_ezoom(struct typeEzoom_Params params);
bool get_ezoom(struct typeEzoom_Params* pParams);
bool set_battery_type(enum enumBattery_Type batteryType);
bool get_battery_type(enum enumBattery_Type* pBatteryType);
bool update_firmware();

#endif /* D3_SERIAL_NANOCORE_CONTROL_IF_H_ */
