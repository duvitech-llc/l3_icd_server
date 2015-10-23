  /*****************************************************************************
  * @file    d3ctlrmod.c
  * @author  George Vigelette
  * @version V1.0.0
  * @date    Sep 24, 2015
  * @brief   Kernel Module for controlling camera.
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
#include <linux/module.h>	/* Needed by all modules */
#include <linux/kernel.h>	/* Needed for KERN_INFO */
#include <linux/init.h>		/* Needed for the macros */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define DRIVER_AUTHOR "George Vigelette <gvigelet@duvitech.com>"
#define DRIVER_DESC   "Nanocore Control Driver"

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/

int nanocore_ctrl_init(void)
{
	printk(KERN_INFO "Loaded Nanocore Controler Module\n");

	/*
	 * A non 0 return means init_module failed; module can't be loaded.
	 */
	return 0;
}

void nanocore_ctrl_exit(void)
{
	printk(KERN_INFO "Unloaded Nanocore Controler Module\n");
}

module_init(nanocore_ctrl_init);
module_exit(nanocore_ctrl_exit);

/*
 * Get rid of taint message by declaring code as GPL.
 */
MODULE_LICENSE("GPL");

/*
 * Or with defines, like this:
 */
MODULE_AUTHOR(DRIVER_AUTHOR);	/* Who wrote this module? */
MODULE_DESCRIPTION(DRIVER_DESC);	/* What does this module do */
