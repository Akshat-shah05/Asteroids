#include <SFML/Graphics.hpp>
#include <ctime>

// Define a window size
const unsigned int WINDOW_WIDTH = 800;
const unsigned int WINDOW_HEIGHT = 600;
const float PI = 3.14159265f;

sf::Vector2f wrapAround(const sf::Vector2f &pos) {
    float x = pos.x;
    float y = pos.y;
    if (x < 0) x += WINDOW_WIDTH;
    if (x > WINDOW_WIDTH) x -= WINDOW_WIDTH;
    if (y < 0) y += WINDOW_HEIGHT;
    if (y > WINDOW_HEIGHT) y -= WINDOW_HEIGHT;
    return { x, y };
}

// SHIP CLASS
class Ship {
public:
    sf::ConvexShape shape;
    sf::Vector2f velocity;
    float rotationSpeed;
    float acceleration;

    // Constructor for the ship
    Ship() {
        // literally just making a triangle lol
        shape.setPointCount(3);
        shape.setPoint(0, sf::Vector2f(0, -15));
        shape.setPoint(1, sf::Vector2f(10, 10));
        shape.setPoint(2, sf::Vector2f(-10, 10));
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);
        // Position the ship at the center
        shape.setPosition(sf::Vector2f(WINDOW_WIDTH / 2.f, WINDOW_HEIGHT / 2.f));
        velocity = sf::Vector2f(0.f, 0.f);
        rotationSpeed = 100.f;
        acceleration = 200.f;
    }

    void update(float dt) {
        shape.move(velocity * dt);
        velocity *= 0.99f;
        // in case we need to wrap around
        shape.setPosition(wrapAround(shape.getPosition()));
    }

    void rotate(float angle) {
        // gotta convert to sf::radians, annoying asf
        shape.rotate(sf::Angle(sf::radians(angle)));
    }

    void accelerate(float dt) {
        float angle = shape.getRotation().asDegrees() - 90.f;
        float rad = angle * PI / 180.f;
        sf::Vector2f thrust(std::cos(rad), std::sin(rad));
        velocity += thrust * acceleration * dt;
    }

    void draw(sf::RenderWindow &window) {
        window.draw(shape);
    }
};

// ASTEROID CLASS
class Asteroid {
public:
    sf::ConvexShape shape;
    sf::Vector2f velocity;
    float rotationSpeed;
    int level; // if its a level two and collision --> split into two smaller ones. if its a level one then it gets destroyed on collision

    // Constructor
    Asteroid(sf::Vector2f position, int level): level{level} { // MIL
        // decide the size based on its radius
        float baseRadius = (level == 2) ? 30.f : 15.f;
        int points = 8; // we'll make this an 8 sided shape
        shape.setPointCount(points);
        for (int i = 0; i < points; ++i) {
            float angle = i * 2 * PI / points;
            // add randomness for irregularity
            float randomOffset = (std::rand() % 10 - 5) / 10.f * baseRadius;
            float r = baseRadius + randomOffset;
            shape.setPoint(i, sf::Vector2f(r * std::cos(angle), r * std::sin(angle)));
        }
        shape.setFillColor(sf::Color::Transparent);
        shape.setOutlineColor(sf::Color::White);
        shape.setOutlineThickness(2);
        shape.setPosition(position);
        float speed = 50.f + std::rand() % 50;
        float angleDeg = std::rand() % 360;
        float rad = angleDeg * PI / 180.f;
        velocity = sf::Vector2f(std::cos(rad), std::sin(rad)) * speed;
        rotationSpeed = (std::rand() % 100 - 50) * 0.1f;
    }   

    void update(float dt) {
        shape.move(velocity * dt);
        shape.rotate(sf::Angle(sf::radians(rotationSpeed * dt)));
        shape.setPosition(wrapAround(shape.getPosition()));
    }

    void draw(sf::RenderWindow &window) {
        window.draw(shape);
    }

    sf::Vector2f getPosition() {
        return shape.getPosition();
    }

    // Collision radius math --> Use the "furthest point out" to determine a radius
    float getRadius() {
        float maxRadius = 0.f;
        for (unsigned int i = 0; i < shape.getPointCount(); ++i) {
            float dist = std::sqrt(std::pow(shape.getPoint(i).x, 2) +
                                   std::pow(shape.getPoint(i).y, 2));
            if (dist > maxRadius)
                maxRadius = dist;
        }
        return maxRadius;
    }
};

class Bullet {
public:
    sf::CircleShape shape;
    sf::Vector2f velocity;
    float lifeTime;

    Bullet(sf::Vector2f position, float angle) {
        shape.setRadius(2);
        shape.setFillColor(sf::Color::White);
        shape.setPosition(position);
        float rad = (angle - 90.f) * PI / 180.f;
        float speed = 400.f;
        velocity = sf::Vector2f(std::cos(rad), std::sin(rad)) * speed;
        lifeTime = 2.f; // bullet only survives for 2 seconds
    }

    // simple movement --> no complex mechanics
    bool update(float dt) {
        shape.move(velocity * dt);
        shape.setPosition(wrapAround(shape.getPosition()));
        lifeTime -= dt;
        return (lifeTime <= 0);
    }

    void draw(sf::RenderWindow &window) {
        window.draw(shape);
    }

    sf::Vector2f getPosition() {
        return shape.getPosition();
    }
};

int main() {
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Asteroids");
    window.setFramerateLimit(60);

    // ship + vector of asteroids and vector of bullets
    Ship ship;
    std::vector<Asteroid> asteroids;
    std::vector<Bullet> bullets;

    // spawn in in itial asteroids (all of which are type 2 asteroids)
    for (int i = 0; i < 5; ++i) {
        float x = std::rand() % WINDOW_WIDTH;
        float y = std::rand() % WINDOW_HEIGHT;
        asteroids.push_back(Asteroid(sf::Vector2f(x, y), 2));
    }

    sf::Clock clock;
    sf::Clock asteroidSpawnClock;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        while (const std::optional<sf::Event> event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape)
                    window.close();
            }
        }

        // --- Handle Input for Moving the ship ---
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            ship.rotate(-ship.rotationSpeed * dt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            ship.rotate(ship.rotationSpeed * dt);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            ship.accelerate(dt);


        // --- Shooting with Spacebar Logic ---
        static bool canShoot = true;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Space)) {
            if (canShoot) {
                sf::Transform transform = ship.shape.getTransform();
                sf::Vector2f tip = transform.transformPoint(sf::Vector2f(0, -15));
                bullets.push_back(Bullet(tip, ship.shape.getRotation().asDegrees()));
                canShoot = false;
            } else {
                canShoot = true;
            }
        }
        
        // --- Spawn in Asteroids --- 
        if (asteroidSpawnClock.getElapsedTime().asSeconds() >= 5.0f) {
            asteroids.push_back(Asteroid(
                sf::Vector2f(std::rand() % WINDOW_WIDTH, std::rand() % WINDOW_HEIGHT), 
                2
            ));
            asteroidSpawnClock.restart();
        }

        // --- Update the Ship ---
        ship.update(dt);
        for (auto it = bullets.begin(); it != bullets.end(); )
        {
            if (it->update(dt))
                it = bullets.erase(it);
            else
                ++it;
        }
        for (auto &asteroid : asteroids)
            asteroid.update(dt);

        // --- Collisions between bullets and asteroids ---
        for (auto bIt = bullets.begin(); bIt != bullets.end(); ) {
            bool bulletRemoved = false;
            for (auto aIt = asteroids.begin(); aIt != asteroids.end(); ) {
                float dx = bIt->getPosition().x - aIt->getPosition().x;
                float dy = bIt->getPosition().y - aIt->getPosition().y;
                float distance = std::sqrt(dx * dx + dy * dy);
                if (distance < aIt->getRadius()) {
                    if (aIt->level > 1) {
                        int newLevel = aIt->level - 1;  // New level is 1.
                        asteroids.push_back(Asteroid(aIt->getPosition(), newLevel));
                        asteroids.push_back(Asteroid(aIt->getPosition(), newLevel));
                    }
                    aIt = asteroids.erase(aIt);
                    bIt = bullets.erase(bIt);
                    bulletRemoved = true;
                    break;
                } else {
                    ++aIt;
                }
            }
            if (!bulletRemoved) {
                ++bIt;
            }
        }

        // --- Collisions between ship and asteroids ---
        for (auto &asteroid : asteroids)
        {
            float dx = ship.shape.getPosition().x - asteroid.getPosition().x;
            float dy = ship.shape.getPosition().y - asteroid.getPosition().y;
            float distance = std::sqrt(dx * dx + dy * dy);
            // Approximating the ship's collision radius as 15 pixels.
            if (distance < 15.f + asteroid.getRadius())
            {
                window.close(); // End game if the ship is hit.
                break;
            }
        }

        // --- Drawing ---
        window.clear(sf::Color::Black);
        ship.draw(window);
        for (auto &asteroid : asteroids)
            asteroid.draw(window);
        for (auto &bullet : bullets)
            bullet.draw(window);
        window.display();
    }

    return 0;
    
}
