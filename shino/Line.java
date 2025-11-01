package shino;

public class Line{
    private final int x1, y1, x2, y2;
    private final double slope, intercept;
    
    public Line(int x1, int y1, int x2, int y2){
        this.x1 = x1;
        this.y1 = y1;
        this.x2 = x2;
        this.y2 = y2;
        slope = (double)(y2 - y1) / (double)(x2 - x1);
        intercept = y1 - slope * x1;
    }

    public int[] get(int x1bound, int y1bound, int x2bound, int y2bound, int dx, int dy){
        if(x1+dx < x1bound && x2+dx < x1bound) return null;
        if(x1+dx > x2bound && x2+dx > x2bound) return null;
        if(y1+dy < y1bound && y2+dy < y1bound) return null;
        if(y1+dy > y2bound && y2+dy > y2bound) return null;

        int[] coords = {x1+dx, y1+dy, x2+dx, y2+dy};
        if(x1+dx < x1bound){
            coords[0] = x1bound;
            coords[1] = y1+dy + (y2 - y1) * (x1bound - (x1+dx)) / (x2 - x1);
        }
        if(x2+dx < x1bound){
            coords[2] = x1bound;
            coords[3] = y1+dy + (y2 - y1) * (x1bound - (x1+dx)) / (x2 - x1);
        }
        if(x1+dx > x2bound){
            coords[0] = x2bound;
            coords[1] = y1+dy + (y2 - y1) * (x2bound - (x1+dx)) / (x2 - x1);
        }
        if(x2+dx > x2bound){
            coords[2] = x2bound;
            coords[3] = y1+dy + (y2 - y1) * (x2bound - (x1+dx)) / (x2 - x1);
        }
        if(y1+dy < y1bound){
            coords[1] = y1bound;
            coords[0] = x1+dx + (x2 - x1) * (y1bound - (y1+dy)) / (y2 - y1);
        }
        if(y2+dy < y1bound){
            coords[3] = y1bound;
            coords[2] = x1+dx + (x2 - x1) * (y1bound - (y1+dy)) / (y2 - y1);
        }
        if(y1+dy > y2bound){
            coords[1] = y2bound;
            coords[0] = x1+dx + (x2 - x1) * (y2bound - (y1+dy)) / (y2 - y1);
        }
        if(y2+dy > y2bound){
            coords[3] = y2bound;
            coords[2] = x1+dx + (x2 - x1) * (y2bound - (y1+dy)) / (y2 - y1);
        }
        return coords;
    }

    public int getX1(){
        return x1;
    }

    public int getX2(){
        return x2;
    }

    public double getSlope(){
        return slope;
    }

    public double getIntercept(){
        return intercept;
    }
}