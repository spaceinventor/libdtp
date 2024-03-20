DTP
=======

DTP (aka the **D** ata **T** ransfer **P** rotocol) is the implementation of an interruptible, resumable, eventually reliable data transfer protocol over CSP designed to utilize the available bandwith optimally.

It uses the concept of `sessions` which basically encapsulate the state data associated with a data transfer operation.

It operates using CSP datagram sockets to communicate between 2 parties, a `client` which initiates a transfer and a `server` which responds to client requests.

Features
--------

* resumable sessions: a transfer can be interrupted (voluntarily or not) and resumed at a later point
* serializable sessions: state data related to a transfer can be stored somewhere (disk, memory, etc) and loaded back into a session object that can subsequently be resumed
* immediate data access: data retrieved from the network is immediately available for consumption by client application, even though the entire data is not yet available or there are holes (missing chunks of data).

Architecture
------------

=============  ====
Module         Role
=============  ====
common         Defines the external interfaces (client, server and configuration)
server         Module serving data upon request
client         Module requesting data and handling status
=============  ====

Client-server communication
---------------------------

.. aafigure::  :name: Communication Overview
  :scale: 2

 +---------+         +---------+
 |Client   |         |Server   |
 +----+----+         +----+----+
      |                   .   |
      |                   .   |
      | transfer req (1)  .   |
      |---------------------->|
      |      resp         .   |
      |<----------------------|                             
                          .   .
      .                   .   .
                              .
      |  datagram 1       |   .
      |<------------------|   .
      |  datagram 2       |   .   
      |<------------------|   .
      |                   |   .
                              .
      .                   .   .
                              .
      | datagram "n-1"    |   .
      |<------------------|   .
      |  datagram n       |   .
      |X------------------|   .
      |datagram "n+1"     |   .
      |X------------------|   .
      |datagram "n+2"     |   .
      |X------------------|   .
                              .
      .                   .   .
                              
      |                   |   |
      |                   |   |
      |                   |   |


* A client initiates a transfer by sending a request to the server using a CSP socket (referred to the `Control Socket` in the rest of this document).

* The request may contain information about which bytes in the payload are to be transferred or may indicate that the entire payload shall be transferred.

* The server responds with the payload's total size as well as the number of bytes expected to be transferred during the session (these 2 numbers
may be equal when a client requests the entire payload).

* At this point, the client creates a connection-less CSP socket (referred to the `Data Socket` in the rest of this document) and starts listening for data from the server.

* Similarily, the server begins sending payload data to the client, until the entire requested data is sent or a request to stop is received on the Control Socket.

* Each "datagram" sent by the server shall be contained in a single CSP packets and shall contains the current pyalod data offset as the initial 32-bits of the CSP packet data. It is assumed
  that the network link between the client and the server (Radio Link, ZMQ, etc) is capable of delivering at least one entire CSP packet, meaning that no partial data is ever received by the 
  DTP client, either a packet is received or it is considered lost.

Implementation
==============

The current DTP implementation consists of the following logical components:

* a **Client** embeddable library, 
* a **Server** embeddable library using no dynamic memory allocation required, only configurable static data structures
* a **Client** CSH APM, using the client library, see the ``apm/dtp_client`` folder in the source code tree
* a **Server** CSH APM, using the server library, see the ``apm/dtp_server`` folder in the source code tree


Using DTP
=========

Development
===========

Tests
-----

