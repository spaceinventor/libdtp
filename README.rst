DTP
=======

CSP FTP is the implementation of an interruptible, resumable, evenutally reliable data transfer protocol over CSP designed to utilize the available bandwith optimally.

It uses the concept of `sessions` which basically encapsulate the state data associated with a data transfer operation.

It operates using CSP datagram sockets to communicate between 2 parties, a `client` which initiates a transfer and a `server` which responds to client requests.

Features
--------

* resumable sessions: a transfer can be interrupted (voluntarily or not) and resumed at a later point
* serializable sessions: state data related to a transfer can be stored somewhere (disk, memory, etc) and loaded back into a session object that can subsequently be resumed
* immediate data access: data retrieved from the network is immediately available for consumption by client application, even though the entire data is not yet available or there are holes (missing chunks of data).

Architecture
------------

3-tier approach:

=============  ====
Module         Role
=============  ====
common         Defines the external interfaces (client, server and configuration)
server         Module serving data upon request
client         Module requesting data and handling status
=============  ====

Client-server protocol
----------------------

.. aafigure::  :name: Protocol Overview
  :scale: 2

 +---------+     +---------+
 |Client   |     |Server   |
 +----+----+     +----+----+
      |               |
      |               |
      |  transfer req |
      |-------------->|
      |      resp     |
      |<--------------|
      |               |
      .               .
      .               .
      .               .
      |  datagram 1   |
      |<--------------|
      |  datagram 2   |
      |<--------------|
      |               |
      .               .
      .               .
      .               .
      | datagram "n-1"|
      |<--------------|
      |  datagram n   |
      |X--------------|
      |datagram "n+1" |
      |X--------------|
      |datagram "n+2" |
      |X--------------|
      .               .
      .               .
      .               .
      |               |
      |<--------------|
      |               |---+
      |               |   |
      |               |<--+



Using CSP FTP
=============

Development
===========

Tests
-----

