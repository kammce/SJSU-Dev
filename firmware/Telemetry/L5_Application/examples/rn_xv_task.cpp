#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "rn_xv_task.hpp"
#include "file_logger.h"
#include "tlm/c_tlm_comp.h"
#include "tlm/c_tlm_var.h"
#include "utilities.h"
#include "str.hpp"



void wifiTask::wifiFlush(void)
{
    char c = 0;
    while(mWifi.getChar(&c, OS_MS(500))) {
        if(mWifiEcho) {
            putchar(c);
        }
    }
}

void wifiTask::wifiSendCmd(const char* pCmd, const char* pParam)
{
    mWifi.put(pCmd);
    if(0 != pParam) {
        mWifi.put(pParam);
    }
    mWifi.put("\r\n");

    wifiFlush();
}

void wifiTask::wifiEnterCmdMode(void)
{
    /* Exit out of command mode just in case */
    mWifi.putline("exit");

    vTaskDelayMs(260);
    mWifi.put("$$$");
    vTaskDelayMs(260);

    wifiFlush();
}

bool wifiTask::wifiHandleHttpReq(web_req_type* request)
{
    bool success = false;

    if(NULL == request->http_ip_host ||
       NULL == request->http_get_request ||
       NULL == request->http_response ||
       '\0' == request->http_get_request[0] ||
       '\0' == request->http_ip_host[0] ||
       request->http_response_size <= 0)
    {
        success = false;
        return success;
    }

    wifiEnterCmdMode();
    wifiSendCmd("set comm open @");

    /* ********************************************************
     * Open connection
     * RN-XV will exit command mode after connection is open,
     * so 'save' and 'exit' commands are not required
     */
    mWifi.put("open ");
    mWifi.put(request->http_ip_host);
    mWifi.putline(" 80");

    /* Need to wait for connection */
    // puts("Wait for connection char ...");
    char c = 0;
    while (c != '@' && mWifi.getChar(&c, OS_MS(30 * 1000))) {
        // putchar(c);
    }
    wifiFlush();

    success = ('@' == c);
    if (!success) {
        LOG_WARN("No connection char while servicing HTTP request");
    }

    /* Send our get request */
    mWifi.put("GET ");
    mWifi.put(request->http_get_request);
    mWifi.putline("\n\n\r\n");

    /* Wait higher timeout to get first server response */
    success = mWifi.getChar(&c, OS_MS(10 * 1000));
    if (!success) {
        LOG_WARN("No response data from HTTP server for 10 seconds");
    }

    if (success && request->http_discard_until) {
        /* Discard input until we see the discard_until char */
        while(request->http_discard_until != c) {
            if(!mWifi.getChar(&c, OS_MS(500))) {
                break;
            }
        }
    }

    /* Get the rest of the data with lower timeout and not that we
     * already received 1 byte above
     */
    request->http_response[0] = c;
    int len = request->http_response_size - 1;
    char *store = request->http_response + 1;
    while(len--) {
        if (! mWifi.getChar(&c, OS_MS(500))) {
            break;
        }
        *store++ = c;
    }
    *store = '\0';
    request->http_response_size = (store - request->http_response);

    wifiFlush();
    wifiEnterCmdMode();
    wifiSendCmd("close");
    wifiSendCmd("set comm open 0");
    wifiSendCmd("exit");

    return success;
}

bool wifiTask::wifiConnect(void)
{
    char *wifi_ssid = mWifiSsid;
    char *wifi_key  = mWifiKey;

    if(wifi_ssid && wifi_key && *wifi_ssid && *wifi_key)
    {
        puts("Using SSID/KEY from disk: ");
        puts(wifi_ssid);
        puts(wifi_key);
        puts("");
    }
    else
    {
        puts("Please configure wifi settings.");
        return false;
    }
    wifiEnterCmdMode();

    // Disable extra printing
    puts("Disable extra printing");
    wifiSendCmd("set sys printlvl 0");
    wifiSendCmd("set uart mode 0");

    // Set connection parameters
    puts("Set connection parameters");
    wifiSendCmd("set ip dhcp 1");
    wifiSendCmd("set wlan ssid ", wifi_ssid);
    wifiSendCmd("set wlan auth 4");
    wifiSendCmd("set wlan phrase ", wifi_key);
    wifiSendCmd("set wlan channel 0");
    wifiSendCmd("set wlan mask 0x1FFF");

    // Set greeting parameters
    puts("Set greeting parameters");
    wifiSendCmd("set comm close 0");
    wifiSendCmd("set comm open 0");
    wifiSendCmd("set comm remote 0");

    // Set flush parameters
    puts("Set buffer parameters");
    wifiSendCmd("set comm match 0");
    wifiSendCmd("set comm timeout 10");
    wifiSendCmd("set comm size 1024");
    //wifi_SendCommand(w, "set comm idle 0");
    wifiSendCmd("set comm idle 10");

    // Set our TCP/IP server parameters
    puts("Setup TCP/IP");
    wifiSendCmd("set ip protocol 2"); /* TCP Server and Client */
    wifiSendCmd("set ip localport " WIFI_PORT);

    puts("Reboot");
    wifiSendCmd("save");
    wifiSendCmd("reboot");
    wifiFlush();
    return true;
}

bool wifiTask::wifiIsConnected(void)
{
    wifiEnterCmdMode();
    mWifi.putline("show connection");

    /**
     * Get response, which could either be : "show connection\n####" (uart echo)
     * or the response could be just "####" (status)
     */
    STR_ON_STACK(rsp, 128);
    mWifi.gets((char*)rsp(), rsp.getCapacity(), 1000);
    if(rsp.beginsWithIgnoreCase("show")) {
        mWifi.gets((char*)rsp(), rsp.getCapacity(), 1000);
    }

    mWifi.putline("exit");
    wifiFlush();

    // Response is in hex: 8ABC
    // Bits 4 and 5 indicate if connection is established
    char lsbHex = rsp[2];
    if(lsbHex >= '0' && lsbHex <= '9') {
        lsbHex -= '0';
    }
    else if(lsbHex >= 'A' && lsbHex <= 'F') {
        lsbHex -= 'A' + 10;
    }

    bool connected = (lsbHex & 0x3);
    return connected;
}

void wifiTask::wifiSendTestCmd(void)
{
    wifiFlush();
    wifiEnterCmdMode();
    wifiFlush();
    mWifi.putline("ver");
}

bool wifiTask::wifiInitBaudRate(void)
{
    STR_ON_STACK(s, 128);

    /* If we can communicate right now, we've got baud rate match so return out of here */
    printf("    Wifi attempt communication @ %i bps\n", (int)mWifiBaudRate);
    mWifi.setBaudRate(mWifiBaudRate);
    wifiSendTestCmd();

    /* Wifi should either echo back "ver" or send response with "wifly" in it */
    s.clear();
    mWifi.gets((char*)s(), s.getCapacity(), 1000);
    if (s.containsIgnoreCase("wifly") || s.containsIgnoreCase("ver")) {
        printf("    Wifi Baud Rate confirmed @ %i\n", (int)mWifiBaudRate);
        wifiSendCmd("exit");
        return true;
    }

    /* We cannot communicate, so try different baud rates */
    int baudRatesToTry[] = {9600, 38400, 115200, 230400, 460800};
    const int num_baud_rates = sizeof(baudRatesToTry) / sizeof(baudRatesToTry[0]);
    for (int i = 0; i < num_baud_rates; i++) {
        const int baudRateToTry = baudRatesToTry[i];

        printf("    Wifi attempt communication @ %i bps\n", baudRateToTry);
        wifiFlush();
        mWifi.setBaudRate(baudRateToTry);
        wifiSendTestCmd();

        /* If we can enter command mode now, then we've found the baud rate!
         * So set/change the desired baud rate on the wifi module
         */
        s.clear();
        if (mWifi.gets((char*)s(), s.getCapacity(), 1000)) {
            if(s.containsIgnoreCase("wifly") || s.containsIgnoreCase("ver")) {
                printf("    Wifi Baud Rate is: %i bps\n", baudRateToTry);

                printf("    Changing Wifi to %i bps\n", (int)mWifiBaudRate);
                s.printf("set uart baudrate %i", mWifiBaudRate);
                wifiSendCmd(s());
                wifiSendCmd("save");
                wifiSendCmd("reboot");
                delay_ms(2000);
                wifiFlush();
                return true;
            }
            else {
                printf("    Wifi bad response: %s\n", s());
            }
        }
    }

    mWifi.setBaudRate(mWifiBaudRate);
    printf("    Wifi Baud Rate is UNKNOWN.  Set baud rate back to %i\n", (int) mWifiBaudRate);
    return false;
}

wifiTask::wifiTask(UartDev& uartForWifi, uint8_t priority) :
        scheduler_task("rnxv", 512*8, priority),
        mWifi(uartForWifi),
        mWifiBaudRate(WIFI_BAUD_RATE),
        mWifiEcho(true)
{
    mHttpReqQueue = xQueueCreate(1, sizeof(web_req_type*));
    memset(mWifiSsid, 0, sizeof(mWifiSsid));
    memset(mWifiKey, 0, sizeof(mWifiKey));

    /* Default values, can be changed/saved during runtime */
    strncpy(mWifiSsid, WIFI_SSID, sizeof(mWifiSsid) - 1);
    strncpy(mWifiKey,  WIFI_KEY , sizeof(mWifiKey) - 1);
}

bool wifiTask::init()
{
    return addSharedObject(WIFI_SHR_OBJ, mHttpReqQueue);
}

bool wifiTask::regTlm()
{
#if SYS_CFG_ENABLE_TLM
    /* User can change this during run-time by setting telemetry variable */
    tlm_component *disk = tlm_component_get_by_name(SYS_CFG_DISK_TLM_NAME);
    TLM_REG_VAR(disk, mWifiSsid, tlm_string);
    TLM_REG_VAR(disk, mWifiKey,  tlm_string);
    TLM_REG_VAR(disk, mWifiEcho, tlm_bit_or_bool);
    TLM_REG_VAR(disk, mWifiBaudRate, tlm_uint);
#endif

    return true;
}

bool wifiTask::taskEntry()
{
    // Not ready until changed otherwise
    mWifi.setReady(false);

    /* If we cannot detect baud rate, error out from here, but return true
     * so that this task doesn't halt the whole system due to this error.
     */
    if(!wifiInitBaudRate()) {
        return true;
    }

    if(!wifiIsConnected()) {
        puts("    Wifi not connected");
        if(wifiConnect()) {
            puts("    Wifi is now connected!");
        }
        else {
            puts("    Wifi ERROR connecting");
        }
    }

    /* Display the IP address */
    do {
        wifiEnterCmdMode();
        mWifi.putline("get ip");
        char buffer[128] = { 0 };
        mWifi.gets(buffer, sizeof(buffer), 1000); /* echo */
        mWifi.gets(buffer, sizeof(buffer), 1000); /* IF=UP */
        mWifi.gets(buffer, sizeof(buffer), 1000); /* DHCP=ON */
        mWifi.gets(buffer, sizeof(buffer), 1000); /* IP : Port */
        mWifi.putline("exit");
        printf("    Wifi %s\n", buffer);
    } while (0) ;

    wifiFlush();
    mWifi.setReady(true);

    return true;
}

bool wifiTask::run(void* p)
{
    web_req_type *request = 0;
    if (xQueueReceive(mHttpReqQueue, &(request), portMAX_DELAY)) {
        mWifi.setReady(false);
        {
            request->success = wifiHandleHttpReq(request);
            if(request->req_done_signal) {
                xSemaphoreGive( (request->req_done_signal));
            }
        }
        mWifi.setReady(true);
    }

    return true;
}
