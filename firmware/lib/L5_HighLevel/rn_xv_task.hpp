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
 * @brief Contains RN-XV task to connect RN-XV Wifly module to WIFI
 */
#ifndef RNXV_TASK_HPP_
#define RNXV_TASK_HPP_


#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "scheduler_task.hpp"
#include "uart_dev.hpp"



/**
 * Web Request Type.
 * See example below on how to use it.
 */
typedef struct {
    char *http_ip_host;       ///< IP Address or hostname
    char *http_get_request;   ///< GET request
    char http_discard_until;  ///< HTTP response will be ignored until we find this string
    char *http_response;      ///< HTTP response will be stored to this buffer
    int http_response_size;   ///< Size of the http_response
    char success;             ///< Changed to true if HTTP request was successful

    SemaphoreHandle_t req_done_signal; ///< After web-request is made, this semaphore is given (if not zero)
} web_req_type;

#define WIFI_PORT       "5555"  ///< Port number to configure for TCP/IP Server
#define WIFI_BAUD_RATE  230400  ///< Baud rate you wish to use (it will auto-detect and change to this)
#define WIFI_SSID       "ssid"  ///< Your SSID
#define WIFI_KEY        "key"   ///< Your WPA2 pass-phrase
#define WIFI_RXQ_SIZE   512     ///< Size of UART's RXQ
#define WIFI_TXQ_SIZE   512     ///< Size of UART's TXQ
#define WIFI_SHR_OBJ    "webrq" ///< The shared object name of this task's Web reqeust queue




/**
 * This is a RN-XV "wifi" task responsible to connect to Wifi Network. It can also allow
 * other tasks to perform HTTP GET requests or POST data to a web-server.
 *
 * User can provide which UART shall be used for the wifiTask and RN-XV. User can use that
 * UART whenever Uart.isReady() is true. If Uart.isReady() returns false, then this task is
 * using the RN-XV and you shouldn't use it.
 *
 * The objective of this task is to connect your RN-XV to Wifi, and provide the method to make
 * a WEB request.  Other than that, you can use the same UART as given to this task for your
 * communication based on isReady() method.  If you only want to connect, you can borrow this
 * code, or run this task and then suspend it once connection is made.
 *
 * To make a web request, populate the web request structure and do this in another task :
 *
 * @code
 * char myBuff[128] = { 0 };
 * web_req_type webreq;
 * webreq.http_ip_host = "www.google.com";
 * webreq.http_get_request = "index.html";
 * webreq.http_discard_until = 0;
 * webreq.http_response = &myBuff[0];
 * webreq.http_response_size = sizeof(myBuff);
 * webreq.req_done_signal = 0; // Don't use signal
 *
 * void *q = getSharedObject(WIFI_SHR_OBJ);
 * xQueueSend(q, &webreq, portMAX_DELAY);
 *
 * // Wait 30 sec (or more) or if you don't want to poll, use a semaphore as signal
 * // webreq.req_done_signal semaphore is given after web request terminates
 * if (webreq.success) {
 *     // Your data will be at myBuff
 * }
 * @endcode
 *
 * If SYS_CFG_ENABLE_TLM is enabled, then the WIFI SSID and Passphrase is saved
 * to disk, which allows you to change the settings during run-time and these
 * settings are preserved across power cycle.  To change these keys, you can
 * use terminal command :
 *  "telemetry disk mWifiSsid mySsid"
 *  "telemetry disk mWifiKey mykey"
 */
class wifiTask : public scheduler_task
{
public:
        /**
         * Task constructor
         * @param uartForWifi  The UART for your RN-XV, such as: "Uart3::getInstance()"
         * @param priority     The task's priority
         */
        wifiTask(UartDev& uartForWifi, uint8_t priority);

        bool run(void *p);    ///< Services web requests
        bool init(void);      ///< Inits stuff (obviously)
        bool taskEntry(void); ///< Auto-detects baud-rate and sets it on RN-XV
        bool regTlm(void);    ///< Registers "disk" variables

private:
        bool wifiInitBaudRate(void);
        void wifiSendTestCmd(void);
        bool wifiHandleHttpReq(web_req_type* request);

        void wifiFlush(void);
        void wifiSendCmd(const char* pCmd, const char* pParam=0);
        void wifiEnterCmdMode(void);
        bool wifiConnect(void);
        bool wifiIsConnected(void);

        UartDev& mWifi;               ///< The uart to use for RN-XV
        uint32_t mWifiBaudRate;       ///< The baud rate of the Wifi
        QueueHandle_t mHttpReqQueue;  ///< Queue handle of web request

        /** @{ Disk telemetry variables */
        bool mWifiEcho;     ///< If true, wifi echo is printed using printf()
        char mWifiSsid[24]; ///< SSID is saved here
        char mWifiKey[24];  ///< Wifi phrase is saved here
        /** @} */
};


#endif /* RNXV_TASK_HPP_ */
