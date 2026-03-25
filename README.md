This is a small prototype I built to explore how a real time audio pipeline could be structured in Zephyr.

The goal was not to build a full system, but to understand how data should move through a pipeline and what issues come up when dealing with buffering, timing, and threading.

The system is organized as a simple producer and consumer model.

The capture thread acts as the producer. It generates synthetic audio data, applies a small processing step, and pushes fixed size blocks into a FIFO.

The playback thread acts as the consumer. It pulls blocks from the FIFO and releases them back to a fixed memory pool.

Instead of using dynamic allocation, the pipeline uses a memory slab with a fixed number of blocks. This makes the behavior predictable and avoids runtime allocation overhead.

I also added basic statistics to observe what is happening in the system while it runs. These include how many blocks are produced and consumed, how full the queue gets, and whether any allocation failures or timeouts occur.

Right now the capture side runs slightly faster than playback. This is intentional. It causes the queue to fill up and lets me observe how the system behaves under pressure. In particular, it shows when blocks cannot be allocated anymore and how often that happens.

The audio source is synthetic for now. This is just to make it easy to test the pipeline without hardware. The structure is set up so that a real backend such as I2S or DMIC could be plugged in later.

To build and run this, first please make sure the Zephyr environment is set up. audio_pipeline_proto is the name of the directory that has this project.

Then run:

cd ~/zephyrproject  
source zephyr/zephyr-env.sh  

cd audio_pipeline_proto
west build -b qemu_x86 -p always  
west build -t run  

When running, the program prints stats.

p is the number of blocks produced  
c is the number of blocks consumed  
d is the current queue depth  
h is the maximum queue depth observed  
m is the number of times capture could not allocate a block  
t is the number of times playback timed out waiting for data  

The output should show the queue filling up and allocation misses increasing, since the producer is slightly faster than the consumer.
