                                MotNet
                                ======

Author: Andrea Crotti, Marius Grysla, Oscar Dustmann <andrea.crotti.0@gmail.com>
Date: 2010-07-26 16:24:31 CEST


Table of Contents
=================
1 Goal 
2 Architecture 
        2.1 dsfkslj 
3 Files 


1 Goal 
~~~~~~~
  The oal of this project is to create a create a network between motes.
  Supposing we have mote A attached to a computer with an internet connection and mote b is attached to a computer without internet connection.

  Then traversing a theoretically arbitrary network of motes we can let B connect to the internet through the network.

  

2 Architecture 
~~~~~~~~~~~~~~~

2.1 dsfkslj 
------------

  + 
    
          

3 Files 
~~~~~~~~
  This is the tree of files in our application
  - driver
    In this directory we have client and gateway program, written in C for linux systems with some bash scripts.
    + reconstruct.c
      this module in charge of reconstructing the chunks we get from from the network

    + chunker.c
      
    + client.c

    + gateway.c

    + tunnel.c

    + structs.c
      contain some useful functions to manage our own data structures

  - shared
    In this directory we keep the data structures definition that we use both from the client/gateway program and the program installed on the motes

  - motes

  - simple-motes




