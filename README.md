# Real-Time Bluesky Jetstream Monitor

Final project for the Real-Time Embedded Systems course.

The program runs on a Raspberry Pi Zero W and connects to the Bluesky Jetstream WebSocket. It uses three POSIX threads:

- **Producer:** receives JSON messages and stores them in a circular buffer.
- **Consumer:** reads the messages, parses the `kind` field with cJSON and updates the counters.
- **Monitor:** runs once every second and records message counters, buffer occupancy and CPU usage in `metrics_log.txt`.

## Requirements

```bash
sudo apt update
sudo apt install build-essential libwebsockets-dev libcjson-dev
```

## Build

From the project folder:

```bash
make
```

This creates the executable:

```text
espx_monitor
```

## Run

```bash
./espx_monitor
```

Stop the program with:

```text
Ctrl+C
```

## Output

The program writes one line per second to:

```text
metrics_log.txt
```

Each row has the format:

```text
Seconds,Nanoseconds,Commit_Count,Identity_Count,Account_Count,Info_Count,Buffer_Occupancy_Pct,CPU_Pct
```

## 24-hour run

The final dataset contains 86,400 samples collected continuously for 24 hours on the Raspberry Pi Zero W.
