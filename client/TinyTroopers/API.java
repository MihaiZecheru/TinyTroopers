package TinyTroopers;

import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.util.Arrays;
import java.util.List;

/**
 * API class for interacting with the Tiny Troopers server
 */
public class API {
    /**
     * Base URL
     */
    private static final String _url = "https://tiny-troopers.mzecheru.com";

    /**
     * Reusable HTTP client
     */
    private static final HttpClient client = HttpClient.newHttpClient();

    /**
     * Create a new room
     *
     * @return The ID of the newly created room
     * @throws Exception If the room could not be created
     */
    public static String CreateRoom() throws Exception {
        HttpRequest request = HttpRequest.newBuilder()
            .uri(URI.create(_url + "/api/create-room"))
            .header("Content-Type", "application/json")
            .POST(HttpRequest.BodyPublishers.ofString("{}")) // Empty body
            .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() == 200) {
            // Return the room ID
            return response.body().substring(12, 18);
        } else {
            throw new Exception("Failed to create room. HTTP error code: " + response.statusCode());
        }
    }

    /**
     * Get a list of all the players in the given room
     *
     * @param room_id The ID of the room
     * @return A list containing the ID of every player in the room
     * @throws Exception If the players could not be retrieved
     */
    public static List<String> GetPlayersInRoom(String room_id) throws Exception {
        HttpRequest request = HttpRequest.newBuilder()
            .uri(URI.create(_url + "/api/" + room_id + "/players"))
            .header("Content-Type", "application/json")
            .GET()
            .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() == 200) {
            // Return the list of players
            // Assuming response.body() returns a JSON array string like ["item1", "item2", "item3"]
            String responseBody = response.body();
            // Remove brackets and quotes
            responseBody = responseBody.replaceAll("[\\[\\]\"]", "");
            return Arrays.asList(responseBody.split(","));
        } else {
            throw new Exception("Failed to get players in room. HTTP error code: " + response.statusCode());
        }
    }

    /**
     * Get a list of all existing rooms.
     * Will get only the ID (code) for each room
     *
     * @return A list containing the ID of every existing room
     * @throws Exception If the rooms could not be retrieved
     */
    public static List<String> GetRooms() throws Exception {
        HttpRequest request = HttpRequest.newBuilder()
            .uri(URI.create(_url + "/api/rooms"))
            .header("Content-Type", "application/json")
            .GET()
            .build();

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() == 200) {
            // Return the list of rooms
            // Assuming response.body() returns a JSON array string like ["item1", "item2", "item3"]
            String responseBody = response.body();
            // Remove brackets and quotes
            responseBody = responseBody.replaceAll("[\\[\\]\"]", "");
            return Arrays.asList(responseBody.split(","));
        } else {
            throw new Exception("Failed to get rooms. HTTP error code: " + response.statusCode());
        }
    }
}
