import pandas as pd

columns = [
    "Seconds",
    "Nanoseconds",
    "Commit_Count",
    "Identity_Count",
    "Account_Count",
    "Info_Count",
    "Buffer_Occupancy_Pct",
    "CPU_Pct"
]

data = pd.read_csv("metrics_log.txt", names=columns)

data["msg_hz"] = (
    data["Commit_Count"]
    + data["Identity_Count"]
    + data["Account_Count"]
    + data["Info_Count"]
)

dt = (
    data["Seconds"].diff()
    + data["Nanoseconds"].diff() / 1e9
)

data["jitter_ms"] = (dt - 1) * 1000

print("Samples:", len(data))
print("Average Hz:", data["msg_hz"].mean())
print("Min Hz:", data["msg_hz"].min())
print("Max Hz:", data["msg_hz"].max())
print("Max Buffer %:", data["Buffer_Occupancy_Pct"].max())
print("Average CPU %:", data["CPU_Pct"].mean())
print("Max CPU %:", data["CPU_Pct"].max())

print("Total commits:", data["Commit_Count"].sum())
print("Total identities:", data["Identity_Count"].sum())
print("Total accounts:", data["Account_Count"].sum())
print("Total infos:", data["Info_Count"].sum())

print()
print("Mean jitter ms:", data["jitter_ms"].mean())
print("Std jitter ms:", data["jitter_ms"].std())
print("Min jitter ms:", data["jitter_ms"].min())
print("Max jitter ms:", data["jitter_ms"].max())

print()
print("Timestamp gaps:", (data["Seconds"].diff().dropna() != 1).sum())