#include <iostream>
#include <cstring>
using namespace std;

struct Point {
    int x, y;

    Point () : x(), y() {}
    Point (int _x, int _y) : x(_x), y(_y) {}
};

class Shape {
    int vertices;
    Point** points;
public:
    Shape (int _vertices) {
        vertices = _vertices;
        points = new Point*[vertices];
        for (int i = 0; i < vertices; i++)
            points[i] = new Point();
    }

    ~Shape () {
        for (int i = 0; i < vertices; i++)
            delete points[i];
        delete [] points;
    }

    void addPoints (Point* pts) {
        for (int i = 0; i < vertices; i++)
            memcpy(points[i], &pts[i], sizeof(Point));
    }

    double area () {
        int temp = 0;
        for (int i = 0; i < vertices; i++) {
            int lhs = points[i]->x * points[(i+1)%vertices]->y;
            int rhs = points[(i+1)%vertices]->x * points[i]->y;
            temp += (lhs - rhs);
        }
        double area = abs(temp)/2.0;
        return area;
    }
};

int main () {
    Point tri1 (0, 0);
    Point tri2 (1, 2);
    Point tri3 (2, 0);

    Point triPts[3] = {tri1, tri2, tri3};
    Shape tri(3);
    tri.addPoints(triPts);


    Point quad1 (0, 0);
    Point quad2 (0, 2);
    Point quad3 (2, 2);
    Point quad4 (2, 0);

    Point quadPts[4] = {quad1, quad2, quad3, quad4};
    Shape quad(4);
    quad.addPoints(quadPts);

    cout << tri.area() << endl;
    cout << quad.area() << endl;

    return 0;
}
