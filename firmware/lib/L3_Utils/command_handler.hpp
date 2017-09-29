/**
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

/*
 * @brief Provides command handling mapping with a function pointer as handler
 * @ingroup Utilities
 *
 * Version: 11102013    Removed 4th parameter (size) of command handler
 * Version: 05022013    Removed output string and replaced with output interface.
 * Version: 04192013    Removed restriction of command limit, just rely on source str as the command.
 * Version: 02162013    Removed input str (moved to stack).  Improved 'help' command output.
 * Version: 12082012    Added short-command handler capability
 * Version: 07262012    Removed "Version" Handler
 * Version: 06192012    Initial
 */
#ifndef COMMANDHANDLER_HPP_
#define COMMANDHANDLER_HPP_

#include "vector.hpp"
#include "str.hpp"
#include "char_dev.hpp"



/**
 * This is a function pointer to handle a command.  If the command handler was added
 * for "CPU" and a command is input to command handler as "CPU UTIL 5", then cmdParams
 * will contain "UTIL 5" which are additional parameters for this command.
 * @param cmdParams     The parameter string to your handler
 * @param output        The output interface you can use to output data.
 * @param pDataParam    This is the same parameter passed when addHandler() is called
 * @param dataParamLen  The length of data associated with pDataParam
 * @returns One of the responses from CmdRspType
 */
typedef bool (*CmdHandlerFuncPtr)(str& cmdParams, CharDev& output, void* pDataParam);

/**
 * This macro can be used to declare and/or define a handler function.
 * @see Parameters of CmdHandlerFuncPtr()
 * Example:
 * @code
 * CMD_HANDLER_FUNC(myHandler);
 *
 * CMD_HANDLER_FUNC(myHandler)
 * {
 *    ...
 * }
 * @endcode
 */
#define CMD_HANDLER_FUNC(name) bool name(str& cmdParams, CharDev& output, void* pDataParam)




/**
 * Command Processor Class
 * @ingroup Utilities
 *
 * This class allows users to add a command string and associated handler for commands.
 * When user inputs a command, it will call the mapped handler.
 * Note that command input is capitalized to make this class case insensitive.
 *
 * One handler is already part of this class:
 *   - "help"   : Get list of supported commands
 *
 * The "help" handler lists out supported commands,
 * "HELP <command>" will show the help-text associated with the given command.
 *
 * Example Usage:
 * @code
 *      CMD_HANDLER_FUNC(cmdHandler)
 *      {
 *          // Process the command ...
 *          output.puts("OK");
 *      }
 *
 *      CommandProcessor cp;
 *      cp.addHandler(cmdHandler, "cmd", "My Cmd Help");
 * @endcode
 */
class CommandProcessor
{
    public:
        /**
         * Constructor
         * @param numCmds Optional: Initial list of vector size to avoid memory re-allocation
         * @note addHandler() will grow the vector of command handlers if more commands are added later
         */
        CommandProcessor(int numCmds=8) :
            mCmdHandlerVector(numCmds), mEnShortCmds(true)
        {
        }

        /**
         * Adds a command to the command handler list
         * @param pFunc                 The function pointer of the handler of this command
         * @param pPersistantCmdStr     The persistent data pointer of a command's text
         * @param pPersistentCmdHelpStr The persistent data pointer of this command's help text
         * @param pDataParam            Optional Param: The data parameter pointer to pass to your handler when it gets called
         * @warning pPersistentCmdStr and pPersistentCmdHelp must always exist in memory without going out of scope because
         *          these strings are not copied internally but their pointer is referenced during comparison
         * @note command is matched while ignoring case.
         */
        void addHandler(CmdHandlerFuncPtr pFunc, const char* pPersistantCmdStr,
                        const char* pPersistentCmdHelpStr=0, void* pDataParam=0);

        /**
         * @{ Command handling functions
         * Handles an incoming command and @returns null terminated string of the command output
         * @param cmd  Input command either as char* pointer or str object
         * @returns    char* pointer containing the command's response
         * @note "HELP" command will output the list of supported commands.
         */
        bool handleCommand(str& cmd, CharDev& out);
        /** @} */

        /**
         * Enables short-hand commands.  If a registered command is "information", and
         * a command comes in as "info", then it will be handled by "information" handler.
         * First command that matches first two characters will take precedence.
         * @note This option is enabled by default.
         */
        inline void enableShortCmds(bool en) { mEnShortCmds = en;}

    private:
        /// Structure of a Handler
        typedef struct
        {
            const char* pCommandStr;  ///< Pointer to the command text
            const char* pCmdHelpText; ///< Pointer to the command's help
            CmdHandlerFuncPtr pFunc;  ///< Pointer to the function pointer handler
            void* pDataParam;         ///< Pointer to the data that should be passed as void pointer to pFunc
        } CmdProcessorType;

        VECTOR<CmdProcessorType> mCmdHandlerVector; ///< Vector of the command handlers
        bool mEnShortCmds; ///< Enables partial matching of command names

        /// Handles a command stored at input and stores output in output object
        void handleCmd(str& input, CharDev& output);

        /// Gets a list of all registered commands which is used by the "HELP" command
        void getRegisteredCommandList(CharDev& output);

        /**
         * Gets output of a "HELP" command.  If helpForCmd is an empty string, then
         * the list of commands is returned, if it is a command name itself, then
         * the help of THAT command's text is returned.
         * @param helpForCmd The name of the command, or null/empty for top level "HELP"
         * @param output     The output text of the command's help is stored here
         */
        void getHelpText(str& helpForCmd, CharDev& output);

        /// Removes null terminated pCmdToRemove from the "input" str and clears output str
        void prepareCmdParam(str& input, const char* pCmdToRemove);
};

#endif /* COMMANDHANDLER_HPP_ */
