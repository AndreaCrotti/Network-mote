                            TOSNET PROJECT
                            ==============

Author: Andrea Crotti, Marius Grysla, Oscar Dustmann
Date: 2010-08-18 10:28:11 CEST


Goal 
~~~~~
  The goal of this project is to create a network between motes.
  Supposing we have mote A attached to a computer with an internet connection and mote B is attached to a computer without internet connection.

  Then traversing a theoretically arbitrary network of motes we can let B connect to the internet through the network.

How to use it 
~~~~~~~~~~~~~~
  - Attach the motes on two computers /A/, /B/, where /A/ has internet access while /B/ doesn't
  - login as root (needed for tun device manipulation)
  - check that the motes are connected (motelist)
  - setup the TinyOS environment
    + setup TOSROOT and other useful variables, use [.bash\_tinyos] to do it automatically
    + follow the [blip tutorial] to compile the serialforwarder
  - install the program on both motes with:
    + *cd motes-simple && make telosb install.$1 /dev/ttyUSB0,1*
      where $1 is the preferred mote ID (254 for the gateway, 1 for the client)
  - compile the driver with
    + *make force*
  - on /A/ run the gateway with *./gateway /dev/ttyUSB0 wlan0* (for example)
  - on /B/ run the client with *make run*
  
  Now B should also have internet access, try to ping outside to check if it works.


  [.bash\_tinyos]: http://www.5secondfuse.com/tinyos/.bash_tinyos
  [blip tutorial]: http://docs.tinyos.net/index.php/BLIP_Tutorial

Further documentation 
~~~~~~~~~~~~~~~~~~~~~~
  In *driver* you can run *make doc* to generate the doxygen documentation of the code, which you will find in *doc\_doxy*

Files 
~~~~~~
  This is the tree of files in our application
  - *driver*
    In this directory we have client and gateway program, written in C for Linux systems.
    + *reconstruct.c*
      this module is in charge of reconstructing the chunks we get from from the network

    + *chunker.c*
      functions to split the message into many chunks

    + *client.c*
      start the client version of the program

    + *gateway.c*
      start the gateway

    + *tunnel.c*
      manage the tunnel (open close write read)

    + *setup.c*
      all the functions used both by the client and the gateway

    + *structs.c*
      contains some useful functions to manage our own data structures

    + *motecomm.c*
      low level communication between motes and the driver program

    + *glue.c*
      Wrapper for the select system call, glues several file descriptors together

    + *serialif.c*
      Serial implementation for the pc side using the serial interface of blip

    + *serialforwardif.c*
      Serial implementation using the serial forwarder for the pc side (not fully supported)

    + *util.c*
      constructor/destructor for class-like types

  - *motes-simple*
    Contains the mote program that was used in the presentation.
    + *SimpleMoteAppC.nc*
      The TinyOS configuration file.

    + *SimpleMoteAppP.nc*
      Here the implementation is done.

    + *SimpleMoteApp.h*
      Contains defines and data structures for the mote program

    + *SendQueueC.nc & SendQueueP.nc*
      Contain a generic queue module for outgoing outgoing messages.

    + *gen_network.py, listen.py, packet.py & simulation.py*
      Some Python scripts that can be used for testing with TOSSIM

  - *motes*
    The structure for a mote program with support for 6lowpan packets.
    Currently not in use, because of speed issues.
    + *MoteNetAppC.nc*
      The configuration

    + *MoteNetAppP.nc*
      Implementation of the mote program. Uses the handler stack.

    + *MoteNetApp.h*
      Structs and defines for the program.

    + *SendQueueC.nc & SendQueueP.nc*
      Contain a generic queue module for outgoing outgoing messages.

  - *python*
    Contains experimental python code creating structs, testing packets chunking and compression

  - *talks*
    - *slides.org*
      org-mode source file of the presentation
    - *slides.tex*
      tex beamer generated file from slides.org
    - *.svg*
      images used
    - *slides.pdf*
      resulting slides
