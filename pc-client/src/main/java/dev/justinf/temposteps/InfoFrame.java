package dev.justinf.temposteps;

import javax.swing.*;
import javax.swing.text.DefaultCaret;
import java.awt.*;
import java.util.concurrent.atomic.AtomicBoolean;

public class InfoFrame {

    private static final Color _GREEN = new Color(98, 222, 131);
    private static final Color _RED = new Color(247, 144, 121);
    private static final Color _YELLOW = new Color(255, 212, 94);

    private final TempoStepsApp app;
    private final AtomicBoolean connectionResult;

    /* Components */
    private JPanel mainPanel;
    private JPanel connectionInfoPanel;
    private JLabel connectionInfoPanelTitleLabel;
    private JPanel sinceLastHandshakePanel;
    private JLabel sinceLastHandshakeLabel;
    private JTextField sinceLastHandshakeTextField;
    private JTextField serialPortTextField;
    private JButton connectButton;
    private JPanel connectPanel;
    private JPanel currentSerialPortPanel;
    private JLabel currentSerialPortLabel;
    private JTextField currentSerialPortTextField;
    private JLabel consolePanelTitleLabel;
    private JPanel consolePanel;
    private JTextArea consoleTextArea;
    private JScrollPane consoleScrollPane;
    private JButton doSomethingButton;

    public InfoFrame(TempoStepsApp app) {
        this.app = app;
        connectionResult = new AtomicBoolean(false);
        connectButton.addActionListener(e -> {
            app.getArduino().setPort(serialPortTextField.getText());
            app.testConnection();
        });
        doSomethingButton.addActionListener(e -> {
            byte random = (byte) ((int) (Math.random() * 3));
            // app.getSerialTerminal().write(random);
            app.log("Sent " + random + " to " + app.getArduino().getPort() + "!");
        });

        // Auto-scroll console text area
        ((DefaultCaret) consoleTextArea.getCaret()).setUpdatePolicy(DefaultCaret.ALWAYS_UPDATE);
    }

    public void update() {
        currentSerialPortTextField.setText(app.getArduino().getPort());
        if (app.getArduino().isConnecting()) {
            currentSerialPortTextField.setBackground(_YELLOW);
        } else {
            if (connectionResult.get()) {
                currentSerialPortTextField.setBackground(_GREEN);
            } else {
                currentSerialPortTextField.setBackground(_RED);
            }
        }

        if (app.getArduino().getLastHandshakeTimestamp() == -1) {
            sinceLastHandshakeTextField.setText("never");
        } else {
            sinceLastHandshakeTextField.setText("" + (System.currentTimeMillis() - app.getArduino().getLastHandshakeTimestamp()));
        }
    }

    public JTextArea getConsoleTextArea() {
        return consoleTextArea;
    }

    /* getset */
    public JPanel getMainPanel() {
        return mainPanel;
    }

    public void atomicallySetConnectionResult(boolean result) {
        connectionResult.set(result);
    }
}
