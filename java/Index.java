package shino;

import javax.swing.SwingUtilities;

public class Index {
    public static void main(String[] args) {
        SwingUtilities.invokeLater(() -> {
            MainWindow mainWindow = new MainWindow();
            mainWindow.show();
        });
    }
}