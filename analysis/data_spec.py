import websocket
import json

url = "wss://jetstream1.us-east.bsky.network/subscribe?wantedCollections=app.bsky.feed.post"
ws = websocket.create_connection(url)

for i in range(5):
    raw = ws.recv()
    data = json.loads(raw)

    print("\nMessage", i+1)
    print("Kind:", data.get("kind"))
    print("Keys:", data.keys())
    print(json.dumps(data, indent=2)[:1500])

ws.close()
