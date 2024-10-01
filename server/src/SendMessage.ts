import { WebSocket } from "ws";

export default function SendMessage(ws: WebSocket, event: string, data?: string): void {
  ws.send(JSON.stringify({ event, data }));
}