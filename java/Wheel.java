package shino;

import java.util.ArrayList;

public class Wheel {
    public double x, y; 
    private final int radius;
    private final ArrayList<Wheel> others = new ArrayList<>();
    private final ArrayList<Double> distances = new ArrayList<>();

    private double vx = 0, vy = 0;

    public Wheel(int x, int y, int radius){
        this.x = x;
        this.y = y;
        this.radius = radius;
    }

    public void update(double ax, double ay){
        vx += ax;
        vy -= ay;
    }

    public void attach(Wheel other){
        others.add(other);
        distances.add(Math.sqrt((other.x - x)*(other.x - x) + (other.y - y)*(other.y - y)));
    }

    public void simulate(ArrayList<Line> lines, boolean accelerating, boolean braking){
        x += vx;
        y -= vy;

        vy -= 0.02; // Gravity

        vx *= 0.9999; // Air resistance
        vy *= 0.9999; // Air resistance

        double maxVelocity = 10;

        for(Line line : lines){
            double m = line.getSlope();
            double b = line.getIntercept();

            double dist = Math.abs(m*x - y + b) / Math.sqrt(m*m + 1);

            double intersectionx = (m*(y - b) + x) / (m*m + 1);

            if(dist < radius && intersectionx >= line.getX1() && intersectionx <= line.getX2()){
                while(dist < radius){
                    y -= 1;
                    dist = Math.abs(m*x - y + b) / Math.sqrt(m*m + 1);
                }
                double theta = -Math.atan(m);

                double vAlongLine = vx * Math.cos(theta) + vy * Math.sin(theta);
                double vNormalToLine = vy * Math.cos(theta) - vx * Math.sin(theta);

                vNormalToLine = vNormalToLine * 1/(1+Math.pow(3, -vNormalToLine)); // Bounce
                vAlongLine *= 0.999; // Friction

                if(accelerating && vAlongLine < maxVelocity) vAlongLine += 0.2;
                if(braking && vAlongLine > -maxVelocity) vAlongLine -= 0.1;

                vx = vAlongLine * Math.cos(theta) - vNormalToLine * Math.sin(theta);
                vy = vAlongLine * Math.sin(theta) + vNormalToLine * Math.cos(theta);
            }
        }

        for(int i=0; i<others.size(); i++){
            final double springConstant = 0.01;
            final double dampingFactor = 0.05;

            Wheel other = others.get(i);
            double desiredDistance = distances.get(i);

            double deltaX = other.x - x;
            double deltaY = other.y - y;
            double actualDistance = Math.sqrt(deltaX * deltaX + deltaY * deltaY);

            if (actualDistance == 0) continue;

            double displacement = actualDistance - desiredDistance;
            double springForceMagnitude = displacement * springConstant;

            double unitX = -deltaX / actualDistance;
            double unitY = deltaY / actualDistance;

            double forceX = unitX * springForceMagnitude;
            double forceY = unitY * springForceMagnitude;

            double relativeVx = other.vx - vx;
            double relativeVy = other.vy - vy;
            double dampingForceX = relativeVx * dampingFactor;
            double dampingForceY = relativeVy * dampingFactor;

            this.vx -= (forceX - dampingForceX);
            this.vy -= (forceY - dampingForceY);
            other.vx += (forceX - dampingForceX);
            other.vy += (forceY - dampingForceY);
        }
    }

    public int[] get(int x1, int y1, int x2, int y2, int cx, int cy){
        if(x + radius + cx < x1 || x - radius + cx > x2 || y + radius + cy < y1 || y - radius + cy > y2) return null;
        return new int[]{(int)Math.round(x + cx), (int)Math.round(y + cy), radius};
    }
}
