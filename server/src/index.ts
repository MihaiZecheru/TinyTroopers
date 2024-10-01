import express, { Request, Response } from 'express';
import expressWs from 'express-ws';

import Server from './server';
import KeepAlive from './KeepAlive';
import WaitingPlayer from './WaitingPlayer';
import Room from './Room';
import { WebSocket } from 'ws';
import { UUID } from 'crypto';

import dotenv from 'dotenv';
dotenv.config();

const app = express();
app.use(express.json());
const app_ws = expressWs(app).app;

/**
 * Create a new room.
 * 
 * Note that this does not connect the player to the room. To connect to the room, the player must use the WebSocket endpoint.
 */
app.get('/api/create-room', (req: Request, res: Response) => {
  const room: Room = Server.CreateRoom();
  res.json({ room_id: room.RoomID });
});

/**
 * Connect to a room.
 * 
 * While waiting for the game to start, players will automatically be sent a message when a new player joins the room or when a player leaves the room.
 */
app_ws.ws('/ws/:room_id', (ws: WebSocket, req: Request) => {
  const room: Room | null = Server.GetRoom(req.params.room_id as UUID);
  
  // Room ID was invalid
  if (!room) {
    ws.close();
    return;
  }

  const default_player_name: string = `Player ${room.PlayerCount + 1}`;
  const player: WaitingPlayer = { PlayerID: default_player_name, ws };
  room.AddPlayer(player);

  ws.onmessage = (message: any) => {
    const msg: { event: string, data?: string } = JSON.parse(message.data);
    room.HandleMessage(player.PlayerID, msg.event, msg?.data);
  }
  
  const interval_id: NodeJS.Timeout = KeepAlive(ws);
  ws.onclose = () => {
    if (interval_id) clearInterval(interval_id);
    room.RemovePlayer(player.PlayerID);
  }
});

const PORT = process.env.PORT;

if (!PORT) {
  throw new Error('PORT environment variable is required but undefined');
}

app.listen(PORT, () => {
  console.log(`\nServer is running on http://localhost:${PORT}\n`);
});
