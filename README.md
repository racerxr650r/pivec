# pivec
Simple user mode app to configure the Raspberry PI composite video controller. Use this application to disable the color burst and modulated chroma signal. It solves the problem described in this [thread on the raspberry pi forums](https://forums.raspberrypi.com/viewtopic.php?t=248217). Use this if you are using an old monochrome CRT with composite input and you want higher resolution without the annoying moving dithering. This app has been tested with the Raspberry PI 4 and PI zero 2w. It's expected to work with the PI 1, 2, 3, zero. I expect it will not work with the PI 5 without some modification since this feature has been moved to the new PI southbridge.

> [!TIP]
> To reduce screen burn in, add consoleblank=60 to the cmdline.txt file. This will blank the console video after 1 minute of inactivity.
