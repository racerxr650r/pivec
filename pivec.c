/*
 * pivec.c
 *
 * User mode app to configure the Raspberry PI composite video controller
 *
 * Created: 12/11/2024
 * Author : john anderson
 *
 * Copyright (C) 2024 by John Anderson <racerxr650r@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any 
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */ 
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <signal.h>

// Macros *********************************************************************
// I've never liked how the static keyword is overloaded in C
// These macros make it more intuitive
#define local        static
#define persistent   static

#define LOG(fmt_str,...)	do{\
   if(appConfig.verbose) \
	   fprintf(stdout,fmt_str, ##__VA_ARGS__); \
}while(0)

// Constants ******************************************************************

// Data Types *****************************************************************
// Configuration
typedef struct CONFIG
{
   bool        color, verbose;
}config_t;

// Globals ********************************************************
// Configuration w/default values
config_t appConfig = {  .color = false,
                        .verbose = false};

// Local function prototypes **************************************************
local void parseCommandLine(  int argc,      // Total count of arguments
                              char *argv[]); // Array of pointers to the argument strings

local void displayUsage(FILE *ouput);        // File pointer to output the text to

local void exitApp(  char* error_str,        // Descriptive char string
                     bool display_usage,     // Display usage?
                     int return_code);       // Return code to use for exit()

/*
 * Main Entry Point ***********************************************************
 */
int main(int argc, char *argv[])
{
   // Parse the command line and setup the config
   parseCommandLine(argc, argv);

   return 0;
}

// Program Runtime Functions **************************************************
/*
 * Parse the application command line and set up the configuration
 */
local void parseCommandLine(int argc, char *argv[])
{
   // For each command line argument...
   for(int i=1;i<=argc-1;++i)
   {
      // If command line switch "-" character...
      if(argv[i][0]=='-')
      {
         int baudrate, databits, stopbits;

         // Decode the command line switch and apply...
         switch(argv[i][1])
         {
            case 'c':
               // If valid color setting...
               if(!strcmp(argv[i],"on"))
                  appConfig.color = true;
               else if(!strcmp(argv[i],"off"))
                  appConfig.color = false;
               break;
            case 'v':
               appConfig.verbose = true;
               break;
            case 'h':
            case '?':
               exitApp(NULL, true, 0);
               break;
            default:
               exitApp("Unknown switch", true, -9);
         }
      }
      // Else update the device path/name 
      else
         appConfig.tty = argv[i];
   }

   if(appConfig.tty==NULL)
      exitApp("No serial device provided", true, -11);
}

/*
 * Display the application usage w/command line options and exit w/error
 */
local void displayUsage(FILE *output_stream)
{
   fprintf(output_stream, "Usage: pivec [OPTION]\n\n\r"
          "Simple user mode app to configure the Raspberry PI composite video controller.\n\r"
          "Use this application to disable the color burst and modulated chroma signal.\n\r"
          "It solves the problem described in this thread on the raspberry pi forums. Use\n\r"
          "this if you are using an old monochrome CRT with composite input and you want\n\r"
          "higher resolution without the annoying moving dithering. This app has been\n\r"
          "tested with the Raspberry PI 4 and PI zero 2w. It's expected to work with the\n\r"
          "PI 1, 2, 3, zero. I expect it will not work with the PI 5 without some\n\r"
          "modification since this feature has been moved to the new PI southbridge.\n\r"
          "OPTIONS:\n\r"
          "  -c   on|off\n\r"
          "       Turn the color burst and chrominance on or off (default:off)\n\r");
}

/*
 * Display a message and exit the application with a given return code
 */
local void exitApp(char* error_str, bool display_usage, int return_code)
{
   FILE *output;

   // Is the return code an error...
   if(return_code)
      output = stderr;
   else
      output = stdout;

   // If an error string was provided...
   if(error_str)
      if(strlen(error_str))
         fprintf(output, "%s %s\n\r%s %s\n\r",  return_code?"Error:":"OK:", 
                                       error_str, 
                                       return_code?"Error Code -":"", 
                                       return_code?strerror(errno):"");

   if(display_usage == true)
      displayUsage(output);

   fflush(output);
   exit(return_code);
}
