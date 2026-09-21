# Real-Time Bluesky Jetstream Monitor

Final project for the Real-Time Embedded Systems course.

The application runs on a Raspberry Pi Zero W and processes live
JSON messages from the Bluesky Jetstream WebSocket API.

## Architecture

The system uses three POSIX threads:

- Producer: receives WebSocket messages and stores them in a circular buffer.
- Consumer: parses JSON messages using cJSON and updates message counters.
- Monitor: runs every second and records message rate, buffer occupancy
  and CPU usage.

Thread synchronization is implemented using pthread mutexes and
condition variables.

## Hardware

- Raspberry Pi Zero W
- Raspberry Pi OS

## Libraries

- pthread
- libwebsockets
- cJSON

## Build

```bash
make
