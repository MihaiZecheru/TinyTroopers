import { UUID } from "crypto";
import Room from "./Room";

export default class Server {
  private static Rooms: Room[] = [];

  public static CreateRoom(): Room {
    const room = new Room();
    this.Rooms.push(room);
    return room;
  }

  public static GetRoom(room_id: UUID): Room | null {
    return this.Rooms.find(r => r.RoomID === room_id) || null;
  }
}