import { WebSocket } from "ws";
import { UUID } from "crypto";
import PlayerClass from "./PlayerClass";
import PlayerColor from "./PlayerColor";

export default class Player {
  public PlayerID: UUID;
  private PlayerClass: PlayerClass;
  private PlayerColor: PlayerColor;
  private ws: WebSocket;
}