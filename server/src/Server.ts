import Room from "./Room";

/**
 * Handles the Tiny Troopers game rooms
 */
export default class Server {
  private static Rooms: Room[] = [];

  public static CreateRoom(): Room {
    const room = new Room();
    this.Rooms.push(room);
    return room;
  }

  /**
   * Get a room by its ID
   * 
   * @returns The room if it exists, null otherwise
   */
  public static GetRoom(room_id: string): Room | null {
    return this.Rooms.find(r => r.RoomID === room_id) || null;
  }

  /**
   * Get the ID of every room
   */
  public static GetRooms(): string[] {
    return this.Rooms.map((r: Room) => r.RoomID);
  }
}
