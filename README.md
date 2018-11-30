Persistence of View Globe for EE445L final project.

A PCB with a TM4C mcu is mounted at the center of a large 3D printed ring. 
A strip of LEDs is mounted on one side of the ring. 
The ring spins very fast and the LEDs flash in such a way to create images using the persistance of view optical illusion.
A magnetic encoder is used to detect the speed of the ring and adjust image output accordingly. 

There are currently three modes available. Since the system is entirely contained within the ring, a user can control the mode using the Blynk app via Wifi.
The modes are color, globe, and music visualizer. 

Color mode simply creates a color changing sphere.
Music visualizer mode uses a microphone and performs an FFT to visualize the frequency spectrum.
Globe mode displays an image of the world map. 

Fast processing is accomplished by reserving the CPU for signal and image processing and using DMA to output the actual LED values via SPI. 
Precise timing is extremely important for the visual effects, so this system is real time. 
