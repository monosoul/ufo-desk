This is an ESPHome component for controlling a lifting desk of an unknown model.
======
Upd: It was noticed that a very similar model has been marketed as KLIFT SBASE by a French company KQueo (Only in France?)

[KLIFT SBASE Manual](pic/klift_sbase.pdf)

Thanks [@ryan-feeley](https://github.com/ryan-feeley) 

Story
======
Some time ago I picked up a used frame of a lifting desk with broken electronics. Quick research revealed that it didn't seem to be any of the known models that you can read about here for example https://community.home-assistant.io/t/desky-standing-desk-esphome-works-with-desky-uplift-jiecang-assmann-others/383790 

The problem was the the control panel didn't turn on every time. In fact, you had to plug and unplug it a hundred times
for it to finally come alive. Physically it seemed ok and I didn't have enough expertise in electronics to investigate
it further, so I connected a logic analyzer, managed to turn it on and recorded some bytes that I later used to put
this component together. I've also sketched a case that can be 3D printed.

[Some pictures of it](pic/pics.md)

Final product
======
![Looks like this](pic/panel.jpg)

Hardware
======
The component assumes that you use it with a popular ESP8266 NodeMCU module with a 0.96 inch display that looks like this:

![NodeMCU](pic/esp8266_display.jpeg)

Case is 3D printed.

The board: https://amzn.eu/d/02hhJ2TA

The buttons: https://amzn.eu/d/06ONH4V7

### Control box hardware mods
In my case the Control Box was making high-pitched whine with the board attached (see example here: https://youtube.com/shorts/FGPJeh3KBNg ).

To alleviate that I applied a silicon sealant (this one: https://amzn.eu/d/05OSW01C ) to the capacitor at E2 (the noisiest one), then C14 and MOV2 (those were the next noisiest ones), and then applied it to other coiled elements, just in case.
![CB1-01-sealant-1](pic/CB1-01_sealant_1.webp)

![CB1-01-sealant-2](pic/CB1-01_sealant_2.webp)

### RJ-45 wiring is as follows
![Wiring](pic/RJ-45_wiring.webp)

Case
======
[STL file](cad/ufo_desk_case.stl)

Model is here: 
https://cad.onshape.com/documents/45d78cc525e092aee55fb095/w/e9f9bd97a05b0db866f06647/e/74652b7a18d69035dbb2d360

Home Assistant
======
Since Home Assistant support is basically free in ESPHome I've exposed a couple of things just to 
give it a try. There is more that can be done, but I don't actually use it, so maybe later.

![HA](pic/ha.png)
