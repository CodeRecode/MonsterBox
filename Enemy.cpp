/* Written by David Jackson
Copyright (C) 20xx DigiPen Institute of Technology. */
#include "Enemy.h"

void Enemy::OnDeath() {
    core->BroadcastMessage(new MessagePlaySound("dead"));
}
void Enemy::OnHit() {
    //core->BroadcastMessage(new MessagePlaySound("hit"));
    mesh->SetRGB(1, 0, 0);
    hitTime = .1;
    transform->translation.y += .05;
}

void Enemy::Update(double frameTime) {
    if (hitTime > 0) {
        hitTime -= (float)frameTime;
        if (hitTime <= 0) {
            mesh->SetRGB(1, 1, 1);
            transform->translation.y -= .05;
        }
    }

    float distance = transform->translation.Distance(core->player->transform->translation);
    float stepSize = 1 / 60.0;

    if (idealDistance - distance < -distEpsilon) {
        collider->velocity = -(transform->translation - core->player->transform->translation) * (collider->maxSpeed / distance);
    }
    else if (idealDistance - distance > distEpsilon) {
        collider->velocity = (transform->translation - core->player->transform->translation) * (collider->maxSpeed / distance);
    }
    else {
        collider->velocity *= 0;
    }

    if (shoots && nextShootTime < core->GetGameTime()) {
        core->BroadcastMessage(new MessageSpawnProjectile(handle, (core->player->transform->translation - transform->translation) * (1.0 / distance), transform->translation));
        nextShootTime = core->GetGameTime() + combat->fireDelay;
        core->BroadcastMessage(new MessagePlaySound("enemy_fire"));
    }
}