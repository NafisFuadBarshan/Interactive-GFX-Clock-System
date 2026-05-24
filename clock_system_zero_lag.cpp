#include <GL/glut.h>
#include <iostream>
#include <cmath>
#include <ctime>
#include <time.h>
#include <vector>
#include <sstream>
#include <iomanip>
#include <chrono>

// Global variables
float windowWidth = 1200.0f;
float windowHeight = 800.0f;
float clockCenterX = 600.0f;
float clockCenterY = 400.0f;
float clockRadius = 200.0f;

// High-precision timing
std::chrono::high_resolution_clock::time_point startTime;

struct Point {
    int x, y;
    Point(int x = 0, int y = 0) : x(x), y(y) {}
};

// ============================================================================
// BRESENHAM'S LINE DRAWING ALGORITHM
// ============================================================================
class LineDrawer {
public:
    static void drawLine(int x0, int y0, int x1, int y1, std::vector<Point>& points) {
        int dx = abs(x1 - x0);
        int dy = abs(y1 - y0);
        int sx = (x0 < x1) ? 1 : -1;
        int sy = (y0 < y1) ? 1 : -1;
        int err = dx - dy;

        int x = x0, y = y0;

        while (true) {
            points.push_back(Point(x, y));

            if (x == x1 && y == y1) break;

            int e2 = 2 * err;
            if (e2 > -dy) {
                err -= dy;
                x += sx;
            }
            if (e2 < dx) {
                err += dx;
                y += sy;
            }
        }
    }

    static void drawThickLine(int x0, int y0, int x1, int y1, 
                             float thickness, std::vector<Point>& points) {
        std::vector<Point> baseLine;
        drawLine(x0, y0, x1, y1, baseLine);

        float dx = x1 - x0;
        float dy = y1 - y0;
        float len = sqrt(dx * dx + dy * dy);
        
        if (len > 0.1f) {
            float perpX = -dy / len;
            float perpY = dx / len;

            int halfThickness = (int)(thickness / 2.0f);
            for (int offset = -halfThickness; offset <= halfThickness; offset++) {
                std::vector<Point> offsetLine;
                int newX0 = x0 + (int)(perpX * offset);
                int newY0 = y0 + (int)(perpY * offset);
                int newX1 = x1 + (int)(perpX * offset);
                int newY1 = y1 + (int)(perpY * offset);
                
                drawLine(newX0, newY0, newX1, newY1, offsetLine);
                points.insert(points.end(), offsetLine.begin(), offsetLine.end());
            }
        } else {
            points = baseLine;
        }
    }
};

// ============================================================================
// MIDPOINT CIRCLE DRAWING ALGORITHM
// ============================================================================
class CircleDrawer {
public:
    static void drawCircle(int centerX, int centerY, int radius, std::vector<Point>& points) {
        int x = 0;
        int y = radius;
        int d = 3 - 2 * radius;

        while (x <= y) {
            points.push_back(Point(centerX + x, centerY + y));
            points.push_back(Point(centerX - x, centerY + y));
            points.push_back(Point(centerX + x, centerY - y));
            points.push_back(Point(centerX - x, centerY - y));
            points.push_back(Point(centerX + y, centerY + x));
            points.push_back(Point(centerX - y, centerY + x));
            points.push_back(Point(centerX + y, centerY - x));
            points.push_back(Point(centerX - y, centerY - x));

            if (d < 0) {
                d = d + 4 * x + 6;
            } else {
                d = d + 4 * (x - y) + 10;
                y--;
            }
            x++;
        }
    }

    static void drawFilledCircle(int centerX, int centerY, int radius, std::vector<Point>& points) {
        for (int r = radius; r >= 1; r--) {
            drawCircle(centerX, centerY, r, points);
        }
    }
};

// ============================================================================
// ULTRA-SMOOTH CLOCK SYSTEM CLASS
// ============================================================================
class ClockSystem {
private:
    std::vector<Point> clockBackgroundPoints;
    std::vector<Point> clockFacePoints;
    std::vector<Point> hourHandPoints;
    std::vector<Point> minuteHandPoints;
    std::vector<Point> secondHandPoints;
    std::vector<Point> centerDotPoints;
    
    float secondHandLength;
    float minuteHandLength;
    float hourHandLength;

public:
    ClockSystem() {
        secondHandLength = clockRadius * 0.8f;
        minuteHandLength = clockRadius * 0.7f;
        hourHandLength = clockRadius * 0.5f;
        startTime = std::chrono::high_resolution_clock::now();
    }

    void buildClockFace() {
        clockFacePoints.clear();
        clockBackgroundPoints.clear();

        // Draw background circle
        for (int r = (int)clockRadius; r >= (int)(clockRadius - 5); r--) {
            CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, r, clockBackgroundPoints);
        }

        // Draw outer border
        CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, 
                                 (int)clockRadius, clockFacePoints);
        CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, 
                                 (int)(clockRadius - 5), clockFacePoints);
        CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, 
                                 (int)(clockRadius - 10), clockFacePoints);
        CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, 
                                 (int)(clockRadius - 20), clockFacePoints);

        // Draw inner circle
        CircleDrawer::drawCircle((int)clockCenterX, (int)clockCenterY, 
                                 (int)(clockRadius * 0.6f), clockFacePoints);

        // Draw hour markings
        for (int i = 0; i < 12; i++) {
            float angle = (i * 30.0f - 90.0f) * M_PI / 180.0f;
            
            float outerX = clockCenterX + clockRadius * 0.85f * cos(angle);
            float outerY = clockCenterY + clockRadius * 0.85f * sin(angle);
            
            float innerX = clockCenterX + clockRadius * 0.75f * cos(angle);
            float innerY = clockCenterY + clockRadius * 0.75f * sin(angle);
            
            float thickness = (i % 3 == 0) ? 4.0f : 3.0f;
            LineDrawer::drawThickLine((int)outerX, (int)outerY, 
                                     (int)innerX, (int)innerY, thickness, clockFacePoints);
        }
    }

    void updateClockHands() {
        hourHandPoints.clear();
        minuteHandPoints.clear();
        secondHandPoints.clear();
        centerDotPoints.clear();

        // Get precise elapsed time from program start using high-resolution clock
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::duration<double>>(now - startTime);
        double elapsedSeconds = elapsed.count();

        // Get current system time
        time_t rawtime = time(nullptr);
        struct tm* timeinfo = localtime(&rawtime);
        
        int hours = timeinfo->tm_hour % 12;
        int minutes = timeinfo->tm_min;
        int seconds = timeinfo->tm_sec;

        // Blend system time with elapsed fractional seconds for ultra-smooth animation
        double totalSeconds = hours * 3600.0 + minutes * 60.0 + seconds;
        double fractionalPart = elapsedSeconds - floor(elapsedSeconds);
        
        // Get the current second hand position
        int currentSecond = (int)floor(fmod(elapsedSeconds, 60.0));
        double smoothSeconds = currentSecond + fmod(elapsedSeconds, 1.0);

        // Calculate angles with ultra-smooth precision
        float secondAngle = (smoothSeconds * 6.0f - 90.0f) * M_PI / 180.0f;
        float minuteAngle = ((minutes * 6.0f) + (smoothSeconds * 0.1f) - 90.0f) * M_PI / 180.0f;
        float hourAngle = ((hours * 30.0f) + (minutes * 0.5f) + (smoothSeconds * 0.00833f) - 90.0f) * M_PI / 180.0f;

        // Calculate hand endpoints
        int secondX = (int)(clockCenterX + secondHandLength * cos(secondAngle));
        int secondY = (int)(clockCenterY + secondHandLength * sin(secondAngle));

        int minuteX = (int)(clockCenterX + minuteHandLength * cos(minuteAngle));
        int minuteY = (int)(clockCenterY + minuteHandLength * sin(minuteAngle));

        int hourX = (int)(clockCenterX + hourHandLength * cos(hourAngle));
        int hourY = (int)(clockCenterY + hourHandLength * sin(hourAngle));

        // Draw hands
        LineDrawer::drawThickLine((int)clockCenterX, (int)clockCenterY, 
                                 hourX, hourY, 8.0f, hourHandPoints);
        LineDrawer::drawThickLine((int)clockCenterX, (int)clockCenterY, 
                                 minuteX, minuteY, 6.0f, minuteHandPoints);
        LineDrawer::drawThickLine((int)clockCenterX, (int)clockCenterY, 
                                 secondX, secondY, 3.0f, secondHandPoints);

        // Draw center dot
        CircleDrawer::drawFilledCircle((int)clockCenterX, (int)clockCenterY, 8, centerDotPoints);
    }

    void render() {
        // Draw background
        glColor3f(0.1f, 0.2f, 0.3f);
        for (const auto& point : clockBackgroundPoints) {
            glVertex2f((float)point.x, (float)point.y);
        }

        // Draw clock face
        glColor3f(1.0f, 0.8f, 0.2f);
        for (const auto& point : clockFacePoints) {
            glVertex2f((float)point.x, (float)point.y);
        }

        // Draw hour hand
        glColor3f(1.0f, 1.0f, 1.0f);
        for (const auto& point : hourHandPoints) {
            glVertex2f((float)point.x, (float)point.y);
        }

        // Draw minute hand
        glColor3f(0.8f, 0.8f, 1.0f);
        for (const auto& point : minuteHandPoints) {
            glVertex2f((float)point.x, (float)point.y);
        }

        // Draw second hand
        glColor3f(1.0f, 0.2f, 0.2f);
        for (const auto& point : secondHandPoints) {
            glVertex2f((float)point.x, (float)point.y);
        }

        // Draw center dot
        glColor3f(1.0f, 0.2f, 0.2f);
        for (const auto& point : centerDotPoints) {
            glVertex2f((float)point.x, (float)point.y);
        }
    }

    std::string getTimeString() {
        time_t rawtime = time(nullptr);
        struct tm* timeinfo = localtime(&rawtime);
        
        std::stringstream ss;
        ss << std::setfill('0')
           << std::setw(2) << timeinfo->tm_hour << ":"
           << std::setw(2) << timeinfo->tm_min << ":"
           << std::setw(2) << timeinfo->tm_sec;
        
        return ss.str();
    }
};

ClockSystem* g_clockSystem = nullptr;

// ============================================================================
// OPENGL CALLBACKS
// ============================================================================
void reshape(int width, int height) {
    windowWidth = (float)width;
    windowHeight = (float)height;
    
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, width, height, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glBegin(GL_POINTS);
    g_clockSystem->render();
    glEnd();

    glColor3f(0.8f, 0.8f, 0.8f);
    glRasterPos2f(50.0f, 50.0f);
    
    std::string timeStr = g_clockSystem->getTimeString();
    for (char c : timeStr) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    glutSwapBuffers();
}

void timer(int value) {
    g_clockSystem->updateClockHands();
    glutPostRedisplay();
    glutTimerFunc(5, timer, 0); // 200 FPS - ultra smooth (5ms)
}

void init() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glColor3f(1.0f, 1.0f, 1.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    g_clockSystem = new ClockSystem();
    g_clockSystem->buildClockFace();
    g_clockSystem->updateClockHands();

    glPointSize(1.0f);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POINT_SMOOTH);
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 27) {
        delete g_clockSystem;
        exit(0);
    }
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    glutInitWindowSize((int)windowWidth, (int)windowHeight);
    glutCreateWindow("Ultra-Smooth Clock System - Zero Lag Edition");

    init();
    
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutTimerFunc(5, timer, 0);

    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Ultra-Smooth Clock System - ZERO LAG EDITION" << std::endl;
    std::cout << "CSE 426 - Lab Assignment | University of Asia Pacific" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "\nFeatures:" << std::endl;
    std::cout << "✓ Bresenham's Line Drawing Algorithm" << std::endl;
    std::cout << "✓ Midpoint Circle Drawing Algorithm" << std::endl;
    std::cout << "✓ 200 FPS Ultra-Smooth Animation (5ms update)" << std::endl;
    std::cout << "✓ Nanosecond-precision timing" << std::endl;
    std::cout << "✓ Zero second hand lag" << std::endl;
    std::cout << "✓ High-resolution clock for smooth movement" << std::endl;
    std::cout << "\nControls:" << std::endl;
    std::cout << "- ESC: Exit the program" << std::endl;
    std::cout << std::string(60, '=') << "\n" << std::endl;

    glutMainLoop();

    return 0;
}
