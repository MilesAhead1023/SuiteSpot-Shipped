# Implementation Spec: Send game end notification via WebSocket

**Capability ID**: 145 | **Slug**: websocket-game-end-notification

## Overview

This capability manages WebSocket communication for send game end notification via websocket. Messages are broadcast to OBS clients on port 8085 in JSON format.

## Control Surfaces

- **Protocol**: WebSocket JSON
- **Port**: 8085
- **Transport**: Async background thread
- **Evidence**: GameManagement.cpp (SocketSend 'GameEnd')

## Code Path Trace

1. **Event Triggered** -> Game event or stat change
2. **Message Build** -> Construct JSON payload
3. **Queue Message** -> SocketSend() adds to broadcast queue
4. **Thread Process** -> WebSocket thread handles queue
5. **Deliver** -> Send to all connected clients

```
Game Event
    |
    v
Handler creates JSON message
    |
    v
SocketSend(message_type, data)
    |
    v
Message queued for broadcast
    |
    v
WebSocket thread processes queue
    |
    v
JSON sent to all clients
```

## Data and State

**Message Format**:
```json
{
    "name": "MessageType",
    "type": "Subtype",
    "data": { /* stat or state data */ },
    "states": {
        "IsInGame": bool,
        "IsInMenu": bool,
        "IsInFreeplay": bool,
        "IsInScoreboard": bool
    }
}
```

**Server Configuration**:
- Port: 8085 (fixed)
- Protocol: WebSocket (ws://)
- Thread: Dedicated background thread
- Queue: Message broadcast queue

## Threading and Safety

**Thread Safety**:
- Server runs in separate background thread
- Queue protected internally
- Stats accessed via GetStats() (copy returned)
- No cross-thread mutations

**Concurrency**: Dedicated WebSocket thread + game thread

## SuiteSpot Implementation Guide

### Step 1: Build Payload
- Construct JSON object
- Include relevant data
- Add game state context

### Step 2: Send Message
- Call SocketSend(type, json_data)
- SocketManagement handles queue/delivery

### Step 3: Verify Format
- Check JSON structure
- Validate field types
- Test with OBS client

### Step 4: Test Delivery
- Connect OBS WebSocket client
- Trigger event
- Verify message received

### Step 5: Handle Failures
- Plan for disconnections
- Handle message queue overflow
- Plan for reconnections

### Step 6: Integration Test
- Connect OBS source with script
- Verify real-time updates
- Check performance impact

## Failure Modes

| Failure | Symptoms | Solution |
|---------|----------|----------|
| Won't connect | OBS can't reach server | Check port 8085 listening |
| No messages | Connected but no updates | Verify SocketSend() called |
| Malformed JSON | Parse errors | Check JSON structure |
| Performance lag | Game stutters | Reduce message frequency |
| Client disconnect | Lost connection | Implement reconnect |

## Testing Checklist

- [ ] Server starts on port 8085
- [ ] Client can connect
- [ ] Message sends on event
- [ ] Message is valid JSON
- [ ] All fields present
- [ ] Client receives message
- [ ] Multiple messages work
- [ ] OBS integration works
- [ ] No performance impact
- [ ] Reconnect works

## Related Capabilities

- Cap #138: WebSocket server init
- Cap #165: Update on change
