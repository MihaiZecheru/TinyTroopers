package TinyTroopers;

import TinyTroopers.Screens.StartScreen;
import TinyTroopers.Screens.WaitingForPlayersScreen;

import javax.swing.*;
import java.util.Objects;

/**
 * Tiny Troopers entry point
 *
 * @author Mihai Zecheru
 * @version 1.0
 */
class Program {
    private static final JFrame window = new JFrame("Tiny Troopers");

    /**
     * Configure the window and handle everything up to the start of the game
     */
    public static void main(String[] args) {
        // Set the main window up
        ConfigureWindow();

        // Show the first screen
        new StartScreen(window).Display();
    }

    private static void ConfigureWindow() {
        // Exit the application when the window is closed
        window.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        // Windowed fullscreen
        window.setExtendedState(JFrame.MAXIMIZED_BOTH);
        window.setResizable(false);

        // Icon
        ImageIcon icon = new ImageIcon(Objects.requireNonNull(Program.class.getResource("/static/icon.png")));
        window.setIconImage(icon.getImage());

        // Show window
        window.setVisible(true);
    }
}
