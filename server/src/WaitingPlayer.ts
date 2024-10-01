import { WebSocket } from "ws";

/**
 * Represents a player waiting in a room for the game to start.
 */
export default interface WaitingPlayer {
  /**
   * The player's unique username, which serves as an ID
   */
  PlayerID: string;

  /**
   * The player's WebSocket connection to the room
   */
  ws: WebSocket;
}