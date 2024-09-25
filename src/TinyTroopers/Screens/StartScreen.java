package TinyTroopers.Screens;

import javax.swing.*;
import java.awt.*;

public class StartScreen extends Screen {
    private JButton createRoomButton = null;
    private JButton joinRoomButton = null;
    private JButton exitButton = null;

    public StartScreen(JFrame window) {
        super(window);
    }

    @Override
    protected void InitializeScreen() {
        // Set the layout for the main panel
        setLayout(new BorderLayout());

        // Create a centered panel
        JPanel centerPanel = new JPanel();
        centerPanel.setLayout(new GridLayout(3, 1, 10, 10)); // 3 rows, 1 column, with gaps

        // Create buttons
        createRoomButton = new JButton("Create Room");
        joinRoomButton = new JButton("Join Room");
        exitButton = new JButton("Exit");

        // Add buttons to the center panel
        centerPanel.add(this.createRoomButton);
        centerPanel.add(this.joinRoomButton);
        centerPanel.add(this.exitButton);

        // Set background color
        setBackground(Color.LIGHT_GRAY);

        // Set center panel background color
        centerPanel.setBackground(Color.DARK_GRAY);

        // Center the panel in the main panel
        add(centerPanel, BorderLayout.CENTER);

        // Initialize event listeners
        InitializeEventListeners();
    }

    /**
     * Initialize event listeners for the three buttons: join room, create room, and exit.
     */
    private void InitializeEventListeners() {
        this.createRoomButton.addActionListener(_ -> OnCreateRoomButtonPress());

        this.joinRoomButton.addActionListener(_ -> OnJoinRoomButtonPress());

        this.exitButton.addActionListener(_ -> System.exit(0));
    }

    /**
     * Logic for when the create room button is pressed.
     */
    private void OnCreateRoomButtonPress() {
        System.out.println("CREATE ROOM BUTTON PRESSED");
        // TODO: create the room here
        new WaitingForPlayersScreen(this.w).Display();
    }

    /**
     * Logic for when the join room button is pressed.
     */
    private void OnJoinRoomButtonPress() {
        System.out.println("JOIN ROOM BUTTON PRESSED");
        // TODO: join the room here
        new WaitingForPlayersScreen(this.w).Display();
    }
}
