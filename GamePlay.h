/* Written by David Jackson
Copyright (C) 20xx DigiPen Institute of Technology. */
#pragma once
#include "Precompiled.h"
#include "Vector3D.h"
#include "IController.h"

class GamePlayController : public IController {
private:
    typedef std::pair<GameObject *, GameObject *> CollisionPair;
    std::queue<CollisionPair> collisions;
    double gameTime;
public:
    ~GamePlayController() {};
    void Init(GameCore *core) { this->core = core; };
    void GiveMessage(Message *message) {
        if (message->type == MessageType::Collision) {
            auto mc = dynamic_cast<MessageCollision *>(message);
            collisions.push(CollisionPair(mc->obj1, mc->obj2));
        }
    };
    void Update(double frameTime);
    void UpdateCollisions();
};