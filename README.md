# Reliable Multicast File Transfer Client (RMFTC)

### UDP Multicast Receiver with Custom Reliability and GTK3 Interface

![C](https://img.shields.io/badge/C-ANSI%20C-blue)
![GTK3](https://img.shields.io/badge/GUI-GTK3-green)
![UDP](https://img.shields.io/badge/Protocol-UDP%20Multicast-orange)
![Threads](https://img.shields.io/badge/Concurrency-Pthreads-red)
![Build](https://img.shields.io/badge/Build-Makefile-lightgrey)

------------------------------------------------------------------------

## Overview

**RMFTC** is a reliable file reception client implemented in C that
reconstructs files transmitted over **UDP multicast**.\
Because UDP does not provide delivery guarantees, the application
implements a lightweight reliability layer using block tracking and loss
detection.

The program joins a multicast group, receives file fragments, detects
missing packets, and reconstructs the original file while keeping the
interface responsive via a GTK3 graphical interface.

This project demonstrates practical implementation of:

-   Network programming in C
-   UDP multicast communication
-   Multithreading using POSIX threads
-   Custom reliability mechanisms over unreliable transport
-   GUI integration using GTK3

------------------------------------------------------------------------

## Architecture

    .
    ├── main.c                # Program entry point
    ├── sock.c / sock.h       # Multicast socket management
    ├── receiver_th.c         # Packet reception thread
    ├── bitmask.c             # Packet tracking and loss detection
    ├── file.c                # File reconstruction logic
    ├── callbacks.c           # GTK signal handlers
    ├── gui_g3.c              # GUI implementation
    ├── fmulticast_client.glade
    ├── Makefile

### Core Modules

**Socket Layer** - Creates UDP socket - Joins multicast group - Receives
datagrams

**Receiver Thread** - Continuously listens for packets - Processes
incoming fragments - Updates reception state

**Reliability Layer (Bitmask)** - Tracks received packet blocks -
Detects missing fragments - Determines completion

**File Assembly** - Writes received data to correct positions - Produces
final reconstructed file

**Graphical Interface** - GTK3 interface built using Glade - Allows
configuration and monitoring - Displays transfer progress

------------------------------------------------------------------------

## Reliability Strategy

Since UDP multicast offers no delivery guarantees, the client implements
a custom reliability approach:

1.  Each packet corresponds to a numbered file block
2.  A bitmask records received blocks
3.  Missing blocks are detected
4.  Transfer completion occurs only when all blocks are present

This preserves multicast efficiency while ensuring file integrity.

------------------------------------------------------------------------

## Build Instructions

### Requirements

-   GCC
-   GTK3 development libraries
-   Make

Install on Ubuntu/Debian:

``` bash
sudo apt install build-essential libgtk-3-dev
```

### Compile

``` bash
make
```

### Run

``` bash
./fmulticast_client
```

------------------------------------------------------------------------

## Networking Requirements

-   Multicast enabled network
-   UDP traffic allowed by firewall
-   Sender transmitting to the same multicast group and port

------------------------------------------------------------------------

## Concurrency Model

-   Main thread: GTK event loop
-   Receiver thread: network packet processing

This design keeps the interface responsive during transfers.

------------------------------------------------------------------------

## Educational Scope

The project covers topics commonly taught in:

-   Computer Networks
-   Operating Systems
-   Distributed Systems

Key concepts: - Multicast networking - Unreliable transport handling -
Thread synchronization - File reassembly

------------------------------------------------------------------------

## Possible Future Improvements

-   Negative acknowledgements (NACK)
-   Sender implementation
-   Transfer statistics
-   Multiple simultaneous transfers
-   Congestion control

------------------------------------------------------------------------

## License

MIT License
