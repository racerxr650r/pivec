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
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <bcm_host.h>

// Macros *********************************************************************
// I've never liked how the static keyword is overloaded in C
// These macros make it more intuitive
#define local        static
#define persistent   static

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#endif
#ifndef EXIT_ERROR
#define EXIT_ERROR   -1
#endif

#define LOG(fmt_str,...)	do{\
   if(appConfig.verbose) \
	   fprintf(stdout,fmt_str, ##__VA_ARGS__); \
}while(0)

// Constants ******************************************************************
#define VEC_REG_OFFSET  0x00c13000
#define VEC_REG_LENGTH  0x1000
#define VEC_REVID       0x40
#define VEC_CONFIG0     0x41
#define CHRDIS          0x00000080
#define CHRDIS_BIT      7
#define BURDIS          0x00000100
#define BURDIS_BIT      8

// Data Types *****************************************************************
// Configuration
typedef struct CONFIG
{
   bool        color_set, color, verbose;
}config_t;

// Globals ********************************************************
// Configuration w/default values
config_t appConfig = {  .color_set = false,
                        .color = false,
                        .verbose = false};

char pi_type[][64] = {  "PI 1 Model A",
                        "PI 1 Model B",
                        "PI 1 Model A+",
                        "PI 1 Model B+",
                        "PI 2 Model B",
                        "PI Alpha",
                        "PI CM 1",
                        "PI CM 2",
                        "PI 3 Model B",
                        "PI Zero",
                        "PI CM 3",
                        "PI CUSTOM",
                        "PI Zero 2w",
                        "PI 3 Model B+",
                        "PI 3 Model A+",
                        "PI FPGA",
                        "PI CM 3+",
                        "PI Model 4 B",
                        "PI 400",
                        "PI CM 4"};
char pi_processor[][32] = {   "BCM2835",
                              "BCM2836",
                              "BCM2837",
                              "BCM2838",
                              "BCM2711"};

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
   bcm_host_init();

   // Parse the command line and setup the config
   parseCommandLine(argc, argv);

   // Open the /dev/mem device we are about do things...
   int mem_fd = open("/dev/mem", O_RDWR | O_SYNC);
   if(mem_fd<0)
      exitApp("Unable to open /dev/mem. Check that you have permission to read/write this device.",false,-1);
   LOG("Opened /dev/mem device\n\r");

   // TODO: use https://github.com/jviki/dtree to get the vec address from the device tree
   void *regs_ptr = mmap(NULL, VEC_REG_LENGTH, PROT_READ | PROT_WRITE, MAP_SHARED, mem_fd, bcm_host_get_peripheral_address() + VEC_REG_OFFSET);
   if(regs_ptr == MAP_FAILED)
      exitApp("Unable to the mmap memory",false,EXIT_ERROR);
   volatile uint32_t *regs = (volatile uint32_t *)regs_ptr;
   LOG("Mapped the VEC peripheral registers.\n\r");

   // If set color...
   if(appConfig.color_set)
   {
      LOG("Setting color state to ");
      // If color is to be turned on...
      if(appConfig.color)
      {
         regs[VEC_CONFIG0] = regs[VEC_CONFIG0] & (~CHRDIS & ~BURDIS);
         LOG("ON\n\r");
      }
      // Else color is to be turned off...
      else
      {
         regs[VEC_CONFIG0] = regs[VEC_CONFIG0] | CHRDIS | BURDIS;
         LOG("OFF\n\r");
      }
   }

   // If verbose, display system information
   LOG("\n\r%s\n\rCPU:     %s\n\r",pi_type[bcm_host_get_model_type()],pi_processor[bcm_host_get_processor_id()]);
   uint32_t width, height;
   graphics_get_display_size(0, &width, &height);
   LOG("Display: %dx%d\n\r",width,height);
   LOG("Peripheral Address: 0x%08x\n\r",bcm_host_get_peripheral_address());
   LOG("Peripheral Size:    0x%08x\n\r",bcm_host_get_peripheral_size());
   LOG("VEC address:        0x%08x\n\r",bcm_host_get_peripheral_address()+VEC_REG_OFFSET);
   LOG("VEC config0:        0x%08x\n\r",regs[VEC_CONFIG0]);
   LOG("VEC Chroma:         %s\n\r",(regs[VEC_CONFIG0] & CHRDIS)>>CHRDIS_BIT?"off":"on");
   LOG("VEC Color Burst:    %s\n\r",(regs[VEC_CONFIG0] & BURDIS)>>BURDIS_BIT?"off":"on");

   if(munmap(regs_ptr, VEC_REG_LENGTH)<0)
   {
      close(mem_fd);
      exitApp("Unable to unmap memory",false,EXIT_ERROR);
   }

   close(mem_fd);
   bcm_host_deinit();
   return EXIT_SUCCESS;
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
         // Decode the command line switch and apply...
         switch(argv[i][1])
         {
            case 'c':
               appConfig.color_set = true;
               ++i;
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
               exitApp(NULL, true, EXIT_SUCCESS);
               break;
            default:
               exitApp("Unknown switch", true, EXIT_ERROR);
         }
      }
   }
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
