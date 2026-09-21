import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

project = Path(__file__).resolve().parent
data_file = project / "metrics_full_run.txt"

columns = ["Seconds","Nanoseconds","Commit_Count","Identity_Count","Account_Count","Info_Count","Buffer_Occupancy_Pct","CPU_Pct"]

data = pd.read_csv(data_file, names=columns)

# Actual time in seconds
data["time"] = data["Seconds"] + data["Nanoseconds"] / 1e9

# Time from beginning of experiment, in hours
data["time_h"] = (data["time"] - data["time"].iloc[0]) / 3600

# Total incoming messages per second
data["msg_hz"] = (data["Commit_Count"]+ data["Identity_Count"]+data["Account_Count"]+data["Info_Count"])

# CPU idle percentage
data["cpu_idle"] = 100 - data["CPU_Pct"]

# Jitter
# Difference of each period from the ideal 1 second
data["jitter_ms"] = data["time"].diff() * 1000 - 1000

plt.figure(figsize=(10, 5))
plt.plot(data["time_h"], data["jitter_ms"], linewidth=0.7)
plt.xlabel("Time hours")
plt.ylabel("Jitter ms")
plt.title("Periodic Thread Jitter")
plt.grid(True)
plt.tight_layout()
plt.savefig("jitter.png", dpi=200)
plt.show()

# 2. Network load buffer
fig, ax1 = plt.subplots(figsize=(10, 5))
line1 = ax1.plot(data["time_h"],data["msg_hz"],linewidth=0.6,label="Message rate")

ax1.set_xlabel("Time hours")
ax1.set_ylabel("Messages per second Hz")
ax1.grid(True)
ax2 = ax1.twinx()
line2 = ax2.plot(data["time_h"],data["Buffer_Occupancy_Pct"],linestyle="--",linewidth=0.8,label="Buffer occupancy")

ax2.set_ylabel("Buffer occupancy %")
ax2.set_ylim(0, 100)

lines = line1 + line2
labels = [line.get_label() for line in lines]
ax1.legend(lines, labels, loc="upper right")

plt.title("Network Load and Circular Buffer")
fig.tight_layout()
plt.savefig("load_buffer.png", dpi=200)
plt.show()


# 3. CPU 
plt.figure(figsize=(8, 5))

plt.scatter(data["msg_hz"],data["CPU_Pct"],s=8,alpha=0.4,label="CPU Busy")

plt.scatter(data["msg_hz"],data["cpu_idle"],s=8,alpha=0.4,label="CPU Idle")

plt.xlabel("Messages per sec Hz")
plt.ylabel("CPU %")
plt.title("Message rate / CPU Usage")
plt.legend()
plt.grid(True)
plt.tight_layout()
plt.savefig("cpu.png", dpi=200)
plt.show()
