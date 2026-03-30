This is a small prototype I built to explore how a real time audio pipeline could be structured in Zephyr.

The goal was not to build a full system, but to understand how data should move through a pipeline and what issues come up when dealing with buffering, timing, and threading.

The system is now organized as a small three stage pipeline.

The capture thread acts as the source stage. It generates synthetic audio data and pushes fixed size blocks into the first queue.

The process thread acts as the middle stage. It pulls blocks from the first queue, applies a small processing step, and passes them into the second queue.

The playback thread acts as the sink stage. It pulls blocks from the second queue, sends them to a simulated sink backend, and then releases them back to the fixed memory pool.

Instead of using dynamic allocation, the pipeline uses a memory slab with a fixed number of blocks. This makes the behavior predictable and avoids runtime allocation overhead.

I also added basic statistics to observe what is happening in the system while it runs. These include how many blocks are produced and consumed, how full the queue gets, and whether any allocation failures or timeouts occur.

Right now the capture side runs slightly faster than playback. This is intentional. It causes the queue to fill up and lets me observe how the system behaves under pressure. In particular, it shows when blocks cannot be allocated anymore and how often that happens.

The audio source is synthetic for now, and the sink is simulated. This is just to make it easy to test the pipeline without hardware. The structure is set up so that a real backend such as DMIC could be attached on the source side later, and a real backend such as I2S or a codec path could be attached on the sink side.

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
d1 is the current depth of the source to process queue
d2 is the current depth of the process to sink queue
h1 is the maximum depth observed in the source to process queue
h2 is the maximum depth observed in the process to sink queue
m is the number of times capture could not allocate a block
t is the number of times playback timed out waiting for data 

The output should show pressure building in the pipeline and allocation misses increasing, since the producer side is still slightly faster than the sink side.
