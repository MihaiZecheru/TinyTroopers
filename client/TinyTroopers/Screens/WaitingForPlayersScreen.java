package TinyTroopers.Screens;

import TinyTroopers.API;

import javax.swing.*;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

public class WaitingForPlayersScreen extends Screen {

    public WaitingForPlayersScreen(JFrame window, String room_id) throws Exception {
        super(window);

        // Get existing players and populate the screen
        List<String> players = API.GetPlayersInRoom(room_id);
        // TODO: Populate the screen with the players

        // Start websocket connection
        // TODO: Start websocket connection
    }

    @Override
    protected void InitializeScreen() {
        // Set the layout manager
        setLayout(null);

        // Add the waiting for players label
        JLabel waitingForPlayersLabel = new JLabel("Waiting for players...");
        waitingForPlayersLabel.setBounds(100, 100, 200, 50);
        add(waitingForPlayersLabel);
    }
}
