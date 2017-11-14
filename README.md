# Cachenet (CNET) 

CNET is an open-source implementation of the cache-based data transfer protocol based on [CJAG (cache-based jamming agreement)](https://github.com/IAIK/CJAG).

CJAG is an open-source implementation of the cache-based jamming agreement which establishes a cross-VM cache covert channels. CNET uses CJAG code to establish covert channels and transfer data over them. It is also possible to use CNET locally for test purposes.

A thorough description of the CJAG can be found in the whitepaper below:
 
 * [CJAG: Cache-based Jamming Agreement](https://www.blackhat.com/docs/asia-17/materials/asia-17-Schwarz-Hello-From-The-Other-Side-SSH-Over-Robust-Cache-Covert-Channels-In-The-Cloud-wp.pdf) (Michael Schwarz, Manuel Weber)

A cache-based, robust covert channel based on CJAG can be found in our NDSS'17 paper

 * [Hello from the Other Side: SSH over Robust Cache Covert Channels in the Cloud](https://cmaurice.fr/pdf/ndss17_maurice.pdf) (Clémentine Maurice, Manuel Weber, Michael Schwarz, Lukas Giner, Daniel Gruss, Carlo Alberto Boano, Stefan Mangard, Kay Römer)

## Table of contents
 
* [Prerequisites](#prerequisites)
* [Building CNET from source](#building-cnet-from-source)
* [Using CNET](#using-cnet)
* [FAQ](#faq)

## Prerequisites

CNET consists of multiple C files. There is no dependency on any external library, thus the only required packages are 

* gcc
* make

On Ubuntu, they can be installed using the package manager:

    sudo apt-get install gcc make
    
 As the program explicitly requests huge pages from the operating system, it requires support of the `mmap` flag `MAP_HUGETLB`. This is the case for any Linux kernel >= 2.6.32.

 Furthermore, if huge pages are not configured, they have to be enabled.
 This can either be done temporarily by running
 
     sudo sysctl -w vm.nr_hugepages=32
 
 or permanently by running
 
     echo "vm.nr_hugepages = 32" | sudo tee >> /etc/sysctl.conf
     
and rebooting afterwards.

## Building CNET from source

If all prerequisites are fulfilled, CNET can be simply built by executing

    make
    
This results in a `cnet` binary.

## Using CNET

The `cnet` binary includes both the sender and the receiver side. 
The sender side runs `./cnet -m <message>` whereas the receiver side runs `./cnet -r`. 

If you test CNET locally, the parameter auto detection should be able to figure out all parameters and CNET will just work. If, however, it does not work, you have to manually tweak the parameters. Run `./cnet --help` to get a list and explanation of all parameters. The most important ones are:

 * **--cache-size**: The size of the last-level cache (also called LLC or L3) in bytes. 
 * **--ways**: The number of cache ways. Will usually be something like 12 or 16. 
 * **--slices**: Usually the number of CPU cores (real cores, not hyperthreads). On modern CPUs it might sometimes also be the number of hyperthreads. 
 * **--threshold**: The minimum number of cycles it takes to access data which is not cached. You can find this number by running the tool `cachespeed` from the subfolder `cachespeed`. Take the value in row "L3 miss" and column "+ mfence".
* **--delay**: If your computer (or VM) is slow, try to increase this value. This gives CJAG more time to react on. Important: this value has to be the same for the sender and the receiver.

The whitepaper contains a table for these parameters for all environments we used to test CJAG (including Amazon EC2). 
If CNET was successful, the sender will display `Done. 100.00% of the channels are established, your system [ V U L N E R A B L E ].` and receiver will print message passed to the sender.
For a thorough explanation of the program's output related to the cache jamming please refer to the CJAG whitepaper above. 

## FAQ

* **I really like the auto detection/eviction set generation/eviction strategy/< insert any part here >. Can I use it in my own project?**

   Yes, all parts of CNET are open source and you are free to use it in your projects. 

* **I get `*[ERROR] Could not retrieve cache sets, please try to restart*`**

   Most likely some cache parameters are wrong. Maybe the auto detection did not work (happens on virtual machines) or you messed up some numbers. Check the specifications of your **host** CPU and try again.
   
* **It does not work!**

   Did you check that all the parameters are correct? Try to play around with the threshold and delay parameter. You should also check the whitepaper, section 4.3 "Common Errors". 
   
* **My cloud provider only has CPUs where the number of slices is not a power of 2.**

    Currently, the cache slice functions for such CPUs are not known. As soon as someone reverse engineers the functions (or Intel releases them), we will update the program. 
    
* **This is nice, but can you release your full covert channel?**

   We will not release the full covert channel. However, using CJAG as a base, the remaining covert channel is just (a lot of) engineering work. 
