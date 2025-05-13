#ifndef PARTICLE_H
#define PARTICLE_H

#include <QPointF>
#include <QColor>

class Particle {
public:
    enum Shape { Line, Circle };

    Particle(QPointF pos, QPointF vel, QColor color, int lifespan, int size = 3)
        : m_position(pos), m_velocity(vel), m_color(color),
         m_lifespan(lifespan), m_age(0), m_size(size), m_shape(Line) {}
    
    void update() {
        m_position += m_velocity;
        m_velocity *= 0.98;  // 空气阻力
        m_age++;
    }
    bool isDead() const { return m_age >= m_lifespan; }
    QPointF position() const { return m_position; }
    QPointF velocity() const { return m_velocity; }
    QColor color() const { return m_color; }
    int size() const { return m_size; }
    void setShape(Shape shape) { m_shape = shape; }

private:
    QPointF m_position;
    QPointF m_velocity;
    QColor m_color;
    int m_lifespan;
    int m_age;
    int m_size;
    float m_gravity = 0.2f;
    float m_decay = 0.99f;
    Shape m_shape = Line;
};

#endif // PARTICLE_H 