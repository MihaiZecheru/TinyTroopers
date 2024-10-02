package TinyTroopers.Screens;

import javax.swing.*;

/**
 * Screen base class. Represents a window screen in the GUI app.
 */
public abstract class Screen extends JPanel {
    protected final JFrame w;

    /**
     * Default initialization is setting the main window reference
     *
     * @param window A reference to the main window for which this screen is to be displayed
     */
    public Screen(JFrame window) {
        this.w = window;
        InitializeScreen();
    }

    /**
     * Initialize the screen
     */
    protected abstract void InitializeScreen();

    /**
     * Display the screen to the main window
     */
    public void Display() {
        this.w.setContentPane(this);
        this.w.revalidate();
        this.w.repaint();
    }
}
