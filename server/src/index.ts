import express, { Request, Response } from 'express';
import expressWs from 'express-ws';

import Server from './Server';
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
 * Home route. Used to verify that app is online
 */
app.get('/', (req: Request, res: Response) => {
console.log(__dirname);
  res.sendFile(__dirname + '/index.html');
});

/**
 * Create a new room.
 *
 * Note that this does not connect the player to the room. To connect to the room, the player must use the WebSocket endpoint.
 */
app.post('/api/create-room', (req: Request, res: Response) => {
  const room: Room = Server.CreateRoom();
  res.json({ room_id: room.RoomID });
});

/**
 * Get the ID of every active room
 */
app.get('/api/rooms', (req: Request, res: Response) => {
  const rooms: string[] = Server.GetRooms();
  res.send(rooms);
});

/**
 * Get all players in the room with the given ID.
 * Will only get the ID of each player
 */
app.get('/api/:room_id/players', (req: Request, res: Response) => {
  const room: Room | null = Server.GetRoom(req.params.room_id as UUID);

  // Room ID was invalid
  if (!room) {
    res.send("Invalid room ID - room does not exist");
    return;
  }

  res.send(room.GetPlayers());
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
