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

#include <stdio.h>
#include <string.h> // strlen()
#include "command_handler.hpp"



// Define the strings returned for OK, ERROR, Invalid, and other cases:
static const char* const HELP_STR               = "help";
static const char* const NO_HELP_STR            = "Help not specified for this command";
static const char* const CMD_INVALID_STR        = "Command Invalid.  Try 'help' command";
static const char* const SUPPORTED_COMMANDS_STR = "Supported Commands:";
static const char* const COMMAND_FAILURE_HELP   = "Command failed!  Command's help is: ";
static const char* const NO_HELP_STR_PTR        = "";


void CommandProcessor::addHandler(CmdHandlerFuncPtr pFunc, const char* pPersistantCmdStr,
                                  const char* pPersistentCmdHelpStr,  void* pDataParam)
{
    CmdProcessorType handler;
    handler.pCommandStr  = pPersistantCmdStr;
    handler.pCmdHelpText = pPersistentCmdHelpStr;
    handler.pFunc = pFunc;
    handler.pDataParam = pDataParam;

    if (0 == handler.pCmdHelpText) {
        handler.pCmdHelpText = NO_HELP_STR_PTR;
    }
    if (0 != handler.pCommandStr && 0 != handler.pFunc) {
        mCmdHandlerVector += handler;
    }
}

bool CommandProcessor::handleCommand(str& cmd, CharDev& output)
{
    bool found = false;
    cmd.trimEnd("\r\n");

    // Note: HELP command cannot simply have a handler because this static handler
    //       will not be able to access the vector of commands
    if(cmd.beginsWithWholeWordIgnoreCase(HELP_STR))
    {
        prepareCmdParam(cmd, HELP_STR);
        getHelpText(cmd, output);
        found = true;
    }
    else
    {
        unsigned int i=0;
        for(i=0; i < mCmdHandlerVector.size(); i++)
        {
            CmdProcessorType &cp = mCmdHandlerVector[i];

            // If a command matches, return the response from the attached function pointer
            if(cmd.beginsWithWholeWordIgnoreCase(cp.pCommandStr))
            {
                prepareCmdParam(cmd, cp.pCommandStr);
                if (!cp.pFunc(cmd, output, cp.pDataParam)) {
                    output.putline(COMMAND_FAILURE_HELP);
                    output.putline(cp.pCmdHelpText);
                }
                found = true;
                break;
            }
        }

        /**
         * If command not matched, try to partially match a command.
         * ie: If command is "magic", match "m" as command
         */
        if(!found)
        {
            for(i=0; i < mCmdHandlerVector.size(); i++)
            {
                CmdProcessorType &cp = mCmdHandlerVector[i];
                STR_ON_STACK(regCmd, 32);
                regCmd = cp.pCommandStr;  /* Set to the command text */

                /**
                 * Check here if pCommandStr contains partial command :
                 *      - regCmd may be "thermostat", when input is "th" or "th on"
                 *      - So accept this command as shorthand command
                 */
                char shortCmd[8] = { 0 };
                cmd.scanf("%7s ", shortCmd);
                if(strlen(shortCmd) >= 2 && regCmd.beginsWithIgnoreCase(shortCmd))
                {
                    prepareCmdParam(cmd, cp.pCommandStr);
                    if (!cp.pFunc(cmd, output, cp.pDataParam)) {
                        output.putline(COMMAND_FAILURE_HELP);
                        output.putline(cp.pCmdHelpText);
                    }

                    found = true;
                    break;
                }
            }
        }

        if(!found)
        {
            output.putline(CMD_INVALID_STR);
        }
    }
    return found;
}

void CommandProcessor::getRegisteredCommandList(CharDev& output)
{
    char buffer[64];
    output.put(SUPPORTED_COMMANDS_STR);
    char *ptr = NULL;

    for(unsigned int i=0; i<mCmdHandlerVector.size(); i++)
    {
        CmdProcessorType& c = mCmdHandlerVector[i];
        if (strlen(c.pCmdHelpText) > 32) {
            sprintf(buffer, "\n %10s : %.32s ...", c.pCommandStr, c.pCmdHelpText);

            /* If a command's help has a newline, truncate it there .. */
            if ((ptr = strrchr(buffer, '\n')) > buffer) {
                strcpy(ptr, "...");
            }
            output.printf(buffer);
        } else {
            output.printf("\n %10s : %s", c.pCommandStr, c.pCmdHelpText);
        }
    }

    output.putline("\n 'help <command>' to get help of a command");
}

void CommandProcessor::getHelpText(str& helpForCmd, CharDev& output)
{
    // If there is a parameter, get help for this specific command
    // where this parameter itself is a command name
    if(helpForCmd.getLen() > 0)
    {
        for(unsigned int i=0; i < mCmdHandlerVector.size(); i++)
        {
            CmdProcessorType &cp = mCmdHandlerVector[i];
            if(helpForCmd.compareToIgnoreCase(cp.pCommandStr))
            {
                const char* out = (0 == cp.pCmdHelpText || '\0' == cp.pCmdHelpText[0]) ?
                                    NO_HELP_STR : cp.pCmdHelpText;
                output.putline(out);
                return;
            }
        }
        output.putline(CMD_INVALID_STR);
    }
    else {
        getRegisteredCommandList(output);
    }
}

void CommandProcessor::prepareCmdParam(str& input, const char* pCmdToRemove)
{
    int i = 0;
    for (i=0; i<input.getLen(); i++) {
        if (input[i] == ' ') {
            break;
        }
    }
    input.eraseFirst(i);
    input.trimStart(" ");
}
