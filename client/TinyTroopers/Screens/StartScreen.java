package TinyTroopers.Screens;

import TinyTroopers.API;

import javax.swing.*;
import java.awt.*;
import java.util.List;

public class StartScreen extends Screen {

    public StartScreen(JFrame window) {
        super(window);
    }

    @Override
    protected void InitializeScreen() {
        // Set the layout for the main panel
        setLayout(new BorderLayout());

        // Create components
        var createRoomButton = new JButton("Create Room");
        var roomCodeInput = new JTextField();
        var exitButton = new JButton("Exit");

        // Configure components
        createRoomButton.setPreferredSize(new Dimension(200, 50));
        roomCodeInput.setPreferredSize(new Dimension(200, 50));
        exitButton.setPreferredSize(new Dimension(200, 50));

        // Add buttons
        add(createRoomButton, BorderLayout.NORTH);
        add(roomCodeInput, BorderLayout.CENTER);
        add(exitButton, BorderLayout.SOUTH);

        // Initialize event listeners
        InitializeEventListeners();
    }

    /**
     * Initialize event listeners for the three buttons: join room, create room, and exit.
     */
    private void InitializeEventListeners() {
        JButton createRoomButton = (JButton) this.getComponents()[0];
        createRoomButton.addActionListener(_ -> OnCreateRoomButtonPress());

        JTextField roomCodeInputComponent = (JTextField) this.getComponents()[1];
        roomCodeInputComponent.addActionListener(_ -> JoinRoom());

        JButton exitButton = (JButton) this.getComponents()[2];
        exitButton.addActionListener(_ -> System.exit(0));
    }

    /**
     * Logic for when the create room button is pressed.
     * Will create a room and take the user to the waiting for players screen.
     */
    private void OnCreateRoomButtonPress() {
        try {
            String room_id = API.CreateRoom();
            new WaitingForPlayersScreen(this.w, room_id).Display();
        } catch (Exception e) {
            JOptionPane.showMessageDialog(
                null,
                e.getMessage(),
                "Error creating room",
                JOptionPane.ERROR_MESSAGE
            );
        }
    }

    /**
     * Called when the user presses 'enter' in the room code input field.
     * Will attempt to join the room with the given code then redirect to the waiting for players screen if successful.
     */
    private void JoinRoom() {
        List<String> rooms = null;
        try {
            rooms = API.GetRooms();
        } catch (Exception e) {
            throw new RuntimeException(e);
        }

        JTextField roomCodeInputComponent = (JTextField) this.getComponents()[1];
        String room_id = roomCodeInputComponent.getText();
        if (room_id.isEmpty() || !rooms.contains(room_id)) {
            JOptionPane.showMessageDialog(
                null,
                "The code you have entered is invalid",
                "Error joining room",
                JOptionPane.ERROR_MESSAGE
            );
            roomCodeInputComponent.setText("");
            return;
        }

        try {
            new WaitingForPlayersScreen(this.w, room_id).Display();
        } catch (Exception e) {
            JOptionPane.showMessageDialog(
                null,
                e.getMessage(),
                "Error joining room",
                JOptionPane.ERROR_MESSAGE
            );
        }
    }
}
