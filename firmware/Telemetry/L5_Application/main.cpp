/*
 *     SocialLedge.com - Copyright (C) 2013
 *
 *     This file is part of free software framework for embedded processors.
 *     You can use it and/or distribute it as long as this copyright header
 *     remains unmodified.  The code is free for personal use and requires
 *     permission to use in a commercial product.
 *
 *      THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 *      OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 *      MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 *      I SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 *      CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *     You can reach the author of this software at :
 *          p r e e t . w i k i @ g m a i l . c o m
 */

/**
 * @file
 * @brief This is the application entry point.
 * 			FreeRTOS and stdio printf is pre-configured to use uart0_min.h before main() enters.
 * 			@see L0_LowLevel/lpc_sys.h if you wish to override printf/scanf functions.
 *
 */
#include <stdint.h>
#include <math.h>
#define PI 3.14159265

#include "tasks.hpp"
#include "examples/examples.hpp"
#include "periodic_scheduler/periodic_callback.h"
#include "uart2.hpp"
#include "uart3.hpp"
#include "utilities.h"
#include "tlm/c_tlm_var.h"
#include "io.hpp"

class vChangerTask : public scheduler_task
{
    public:
        vChangerTask(uint8_t priority) : scheduler_task("vChangerTask", 2048, priority) {}
        uint32_t x;
        float y;
        float cosine;
        int32_t z;
        uint32_t inc;

        int32_t acc_x;
        int32_t acc_y;
        int32_t acc_z;

        uint32_t light;
        uint32_t temp;

        bool init(void)
        {
            // Clear variables
            x = 0;
            y = 0;
            z = 1;
            cosine = 0;
            inc = 1;
            acc_x = 0;
            acc_y = 0;
            acc_z = 0;
            light = 0;
            temp = 0;
            // Get pointer to telemetry bucket
            tlm_component * app_tlm = tlm_component_add("App");
            tlm_component * sensors_tlm = tlm_component_add("Sensors");
            // Add variables to app_tlm
            TLM_REG_VAR(app_tlm, x, tlm_uint);
            TLM_REG_VAR(app_tlm, y, tlm_float);
            TLM_REG_VAR(app_tlm, z, tlm_int);
            TLM_REG_VAR(app_tlm, cosine, tlm_float);

            TLM_REG_VAR(sensors_tlm, acc_x, tlm_int);
            TLM_REG_VAR(sensors_tlm, acc_y, tlm_int);
            TLM_REG_VAR(sensors_tlm, acc_z, tlm_int);
            TLM_REG_VAR(sensors_tlm, light, tlm_int);
            TLM_REG_VAR(sensors_tlm, temp, tlm_int);

            return true;
        }
        bool run(void *p)
        {
            x++;
            y += 0.1;

            inc = (1 <= z && z <= 98) ? inc : -inc;
            z += inc;

            cosine = cos((x * PI)/180.0);

            LD.setNumber(z);

            light = LS.getRawValue();

            acc_x = AS.getX();
            acc_y = AS.getY();
            acc_z = AS.getZ();

            temp = TS.getFarenheit();

            vTaskDelay(100);
            return true;
        }
};

int main(void)
{
    /**
     * A few basic tasks for this bare-bone system :
     *      1.  Terminal task provides gateway to interact with the board through UART terminal.
     *      2.  Remote task allows you to use remote control to interact with the board.
     *      3.  Wireless task responsible to receive, retry, and handle mesh network.
     *
     * Disable remote task if you are not using it.  Also, it needs SYS_CFG_ENABLE_TLM
     * such that it can save remote control codes to non-volatile memory.  IR remote
     * control codes can be learned by typing the "learn" terminal command.
     */
    scheduler_add_task(new terminalTask(PRIORITY_HIGH));
    /* Consumes very little CPU, but need highest priority to handle mesh network ACKs */
    scheduler_add_task(new wirelessTask(PRIORITY_CRITICAL));
    scheduler_add_task(new vChangerTask(PRIORITY_LOW));
    scheduler_start(); ///< This shouldn't return
    return -1;
}
