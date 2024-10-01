import { WebSocket } from "ws";
import SendMessage from "./SendMessage";

export default function KeepAlive(ws: WebSocket): NodeJS.Timeout {
  return setInterval(() => {
    SendMessage(ws, "PING");
  }, 15000);
}