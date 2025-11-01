package shino;

import javax.imageio.ImageIO;
import java.awt.Graphics2D;
import java.awt.geom.AffineTransform;
import java.awt.image.BufferedImage;
import java.io.InputStream;

public class Car {
    private final Wheel backWheel, frontWheel;
    private BufferedImage image;

    public static final double IMG_BACK_CX      = 36;   // px
    public static final double IMG_BACK_CY      = 112;  // px
    public static final double IMG_FRONT_CX     = 114;  // px
    public static final double IMG_FRONT_CY     = 112;  // px
    public static final double IMG_WHEEL_BASE   = IMG_FRONT_CX - IMG_BACK_CX; // px
    public static final double IMG_WHEEL_RADIUS = 12;   // px

    private final double TORQUE_FORCE = 0.05;

    private double angle;
    private double drawScale = 1.0;
    private double fineDX = 0, fineDY = 18;

    public void nudgeInCarFrame(double dx, double dy){
        fineDX += dx; fineDY += dy;
    }

    public Car(Wheel backWheel, Wheel frontWheel) {
        this.backWheel = backWheel;
        this.frontWheel = frontWheel;
        loadImage();
    }

    private void loadImage() {
        try (InputStream in = Car.class.getResourceAsStream("/shino/car.png")) {
            if (in == null) throw new IllegalStateException("car.png not found on classpath");
            image = ImageIO.read(in);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void update(boolean accelerating, boolean braking) {
        if (accelerating) {
            backWheel.vy -= TORQUE_FORCE;
            frontWheel.vy += TORQUE_FORCE;
        }
        if (braking) {
            backWheel.vy += TORQUE_FORCE;
            frontWheel.vy -= TORQUE_FORCE;
        }

        angle = Math.atan2(frontWheel.y - backWheel.y, frontWheel.x - backWheel.x);

        double rAvg = (backWheel.getRadius() + frontWheel.getRadius()) * 0.5;
        drawScale = rAvg / IMG_WHEEL_RADIUS;
    }

    public int getVisualWheelRadius() {
        return (int) Math.round(IMG_WHEEL_RADIUS * drawScale);
    }
    
    public void draw(Graphics2D g2d, int camX, int camY) {
        if (image == null) return;

        int screenBackX = (int)Math.round(backWheel.x - camX);
        int screenBackY = (int)Math.round(backWheel.y + camY);

        AffineTransform at = new AffineTransform();
        at.translate(screenBackX, screenBackY);
        at.rotate(angle);
        at.scale(drawScale, drawScale);
        at.translate(-IMG_BACK_CX + fineDX, -IMG_BACK_CY + fineDY);

        g2d.drawImage(image, at, null);
    }
}