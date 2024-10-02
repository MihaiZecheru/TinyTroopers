import { UUID, randomUUID } from "crypto";
import WaitingPlayer from "./WaitingPlayer";
import Player from "./Player";
import SendMessage from "./SendMessage";

/**
 * Represents a TinyTroopers game room.
 */
export default class Room {
  /**
   * The ID of the room
   */
  public RoomID: UUID;

  /**
   * True if the game has begun, false if the room is still waiting for players to join.
   */
  public game_started: boolean = false;

  /**
   * Initially, all players are PreGamePlayers. Once the game starts, they will be converted to Players.
   */
  private PreGamePlayers: Array<WaitingPlayer> = [];

  /**
   * Once all players are in the room and the game starts, all PreGamePlayers will be converted to Players.
   */
  private Players: Array<Player> = [];

  /**
   * Get the amount of players in the room
   */
  public get PlayerCount(): number {
    return this.Players.length || this.PreGamePlayers.length;
  }

  constructor() {
    this.RoomID = randomUUID();
  }

  /**
   * Add a player to the room. This is a 'WaitingPlayer' object, as players cannot join mid-game.
   * This player will automatically be sent a message when a new player joins the room or when a player leaves the room.
   *
   * @param player The player to add to the room. This is a 'WaitingPlayer' object, as players cannot join mid-game.
   */
  public AddPlayer(player: WaitingPlayer) {
    // Add player
    this.PreGamePlayers.push(player);

    // Notify all other players of the added player
    this.PreGamePlayers.forEach((p: WaitingPlayer) => {
      SendMessage(p.ws, "PLAYER-JOINED", p.PlayerID);
    });
  }

  /**
   * Remove the player with the given ID from the room.
   *
   * @param id The ID of the player to remove.
   */
  public RemovePlayer(id: string) {
    // Remove player
    this.Players = this.Players.filter((p: WaitingPlayer | Player) => p.PlayerID !== id);

    // Notify all other players of the removed player
    this.PreGamePlayers.forEach((p: WaitingPlayer) => {
      SendMessage(p.ws, "PLAYER-LEFT", id);
    });
  }

  /**
   * Get the players as a list of player IDs
   */
  public GetPlayers(): string[] {
    if (this.Players.length)
      return this.Players.map((p: Player) => p.PlayerID);
    else if (this.PreGamePlayers.length)
      return this.PreGamePlayers.map((p: WaitingPlayer) => p.PlayerID);
    else return [];
  }

  /**
   * Handle a message from a player.
   *
   * @param id The ID of the player who sent the message.
   * @param event The event that the player sent.
   * @param data The data that the player sent.
   */
  public HandleMessage(id: string, event: string, data?: string) {
    // Handle message
    console.log(`Received message from ${id}: ${event} ${data}`);
  }
}
