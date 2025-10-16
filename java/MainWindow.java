package shino;

import java.awt.BasicStroke;
import java.awt.Color;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GraphicsDevice;
import java.awt.GraphicsEnvironment;
import java.awt.event.ActionEvent;
import java.util.ArrayList;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.AbstractAction;
import javax.swing.ActionMap;
import javax.swing.InputMap;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.KeyStroke;

public class MainWindow {
    private final JFrame frame;

    private final JPanel panel;

    private int width, height;

    private ArrayList<Line> lines = new ArrayList<>();

    private ArrayList<Wheel> wheels = new ArrayList<>();
    
    private int lastx, lasty;
    private float slope = 0f;

    // terrain generation parameters
    private final int step = 20;
    private float difficulty = 0.01f;
    private final float difficultyincrement = 0.001f;
    private final float irregularity = 0.5f;

    private int camerax = 0;
    private int cameray = 200;
    private int cameraxfarthest = 0;

    //driving parameters
    private boolean accelerating = false;
    private boolean braking = false;

    public MainWindow() {

        frame = new JFrame("Driver");
        
        GraphicsDevice device = GraphicsEnvironment.getLocalGraphicsEnvironment().getDefaultScreenDevice();
        if (device.isFullScreenSupported()) {
            frame.setUndecorated(true);
            device.setFullScreenWindow(frame);
        } else {
            System.err.println("Fullscreen not supported");
            frame.setSize(800, 600);
            frame.setLocationRelativeTo(null);
            frame.setVisible(true);
        }

        panel = new JPanel(){
            @Override
            public void paint(Graphics g) {
                super.paint(g);
                Graphics2D g2d = (Graphics2D) g;
                g2d.setColor(new Color(0, 150,50));
                g2d.setStroke(new BasicStroke(5));
                
                for(Line line : lines){
                    int[] coords = line.get(0, 0, width, height, -camerax, cameray);
                    if(coords != null){
                        g2d.drawLine(coords[0], coords[1], coords[2], coords[3]);
                    }
                }

                for(Wheel wheel : wheels){
                    int[] coords = wheel.get(0, 0, width, height, -camerax, cameray);
                    if(coords != null){
                        g2d.setColor(Color.WHITE);
                        g2d.fillOval(coords[0]-coords[2], coords[1]-coords[2], coords[2]*2, coords[2]*2);
                    }
                }
            }
        };

        frame.add(panel);

        panel.setBackground(Color.BLACK);
        panel.setSize(frame.getWidth(), frame.getHeight());
        
        width = panel.getWidth();
        height = panel.getHeight();
        
        InputMap inputMap = panel.getInputMap(JComponent.WHEN_IN_FOCUSED_WINDOW);
        ActionMap actionMap = panel.getActionMap();

        inputMap.put(KeyStroke.getKeyStroke("ESCAPE"), "escPressed");
        actionMap.put("escPressed", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                System.exit(0);
            }
        });

        inputMap.put(KeyStroke.getKeyStroke("W"), "wPressed");
        actionMap.put("wPressed", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                accelerating = true;
            }
        });

        inputMap.put(KeyStroke.getKeyStroke("released W"), "wReleased");
        actionMap.put("wReleased", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                accelerating = false;
            }
        });

        inputMap.put(KeyStroke.getKeyStroke("S"), "sPressed");
        actionMap.put("sPressed", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                braking = true;
            }
        });

        inputMap.put(KeyStroke.getKeyStroke("released S"), "sReleased");
        actionMap.put("sReleased", new AbstractAction() {
            @Override
            public void actionPerformed(ActionEvent e) {
                braking = false;
            }
        });

        lasty = height/2;

        for(int i=step; i<=width+step; i+=step){
            slope += (float)(Math.random() - (float)lasty/height)*difficulty;
            if(slope > 1) slope = 1;
            if(slope < -1) slope = -1;
            int newy = lasty + Math.round(slope*(float)Math.pow(Math.abs(slope), irregularity)*step);
            lines.add(new Line(i-step, lasty, i, newy));
            lasty = newy;

            difficulty += difficultyincrement;
        }

        lastx = step * lines.size();

        wheels.add(new Wheel(100, 300, 20));
        wheels.add(new Wheel(200, 300, 20));
        wheels.get(0).attach(wheels.get(1));

        panel.repaint();
    }

    public void show(){
        Timer timer = new Timer();
        timer.schedule(new TimerTask(){
            @Override
            public void run(){
                int averagex = 0;
                int averagey = 0;
                for(Wheel wheel : wheels){
                    averagex += wheel.x;
                    averagey += wheel.y;
                }
                averagex /= wheels.size();
                averagey /= wheels.size();

                camerax = averagex - 200;
                cameray = -averagey + height/2;

                if(camerax > cameraxfarthest){
                    cameraxfarthest += step;
                    slope += (float)(Math.random() - (float)lasty/height) * difficulty;
                    if(slope > 1) slope = 1;
                    if(slope < -1) slope = -1;
                    int newy = lasty + Math.round(slope*(float)Math.pow(Math.abs(slope), irregularity)*step);
                    lines.add(new Line(lastx, lasty, lastx+step, newy));
                    lasty = newy;
                    lastx += step;

                    if(lines.size() > (width/step)*3) lines.remove(0);
    
                    difficulty += difficultyincrement;
                }

                for(Wheel wheel : wheels){
                    wheel.simulate(lines, accelerating, braking);
                }

                panel.repaint();
                show();
            }
        }, 5);
    }
}
