package TinyTroopers.Screens;

import javax.swing.*;

public class WaitingForPlayersScreen extends Screen {

    public WaitingForPlayersScreen(JFrame window) {
        super(window);
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
