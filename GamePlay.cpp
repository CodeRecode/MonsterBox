/* Written by David Jackson
Copyright (C) 20xx DigiPen Institute of Technology. */
#include "GamePlay.h"
#include "GameObject.h"
#include "Core.h"
#include "PowerUp.h"

void CollisionProjectileTerrain(GameCore *core, GameObject *projectile, GameObject *terrain) {
    core->BroadcastMessage(new MessageDeleteObject(projectile->handle));
    projectile->OnDeath();

    core->BroadcastMessage(new MessageDeleteObject(terrain->handle));
    terrain->OnDeath();
}

void CollisionProjectilePlayer(GameCore *core, GameObject *projectile, GameObject *player) {
    if (projectile->source != player->handle) {
        core->BroadcastMessage(new MessageDeleteObject(projectile->handle));
        projectile->OnDeath();

        player->combat->health -= projectile->combat->damage;
        player->OnHit();        
        core->BroadcastMessage(new MessagePlaySound("player_hit"));
    }
}

void CollisionProjectileEnemy(GameCore *core, GameObject *projectile, GameObject *enemy) {
    if (projectile->source != enemy->handle) {
        core->BroadcastMessage(new MessageDeleteObject(projectile->handle));
        projectile->OnDeath();

        enemy->combat->health -= projectile->combat->damage;
        enemy->OnHit();
    }
}

void CollisionProjectileProjectile(GameCore *core, GameObject *projectile1, GameObject *projectile2) {
    GameObject *source1 = core->GetGameObjectById(projectile1->source), *source2 = core->GetGameObjectById(projectile2->source);
    if (!source1 || !source2 || source1->type != source2->type)
    {
        core->BroadcastMessage(new MessageDeleteObject(projectile1->handle));
        projectile1->OnDeath();

        core->BroadcastMessage(new MessageDeleteObject(projectile2->handle));
        projectile2->OnDeath();
    }
}

void CollisionExplosionTerrain(GameCore *core, GameObject *explosion, GameObject *terrain) {    
    core->BroadcastMessage(new MessageDeleteObject(terrain->handle));
    terrain->OnDeath();
}

void CollisionExplosionPlayer(GameCore *core, GameObject *explosion, GameObject *player) {
    if (explosion->source == player->handle) {
        player->combat->health -= explosion->combat->damage / 4; // Reduced self damage
        player->OnHit();
    }
}

void CollisionExplosionEnemy(GameCore *core, GameObject *explosion, GameObject *enemy) {
    enemy->combat->health -= explosion->combat->damage;
    enemy->OnHit();
}

void CollisionPlayerEnemy(GameCore *core, GameObject *player, GameObject *enemy) {
    enemy->combat->health -= player->combat->damage;
    enemy->OnHit();

    player->combat->health -= enemy->combat->damage;
    player->OnHit();
}

void CollisionPlayerPowerUp(GameCore *core, GameObject *player, GameObject *powerUp) {
    PowerUp *pu = dynamic_cast<PowerUp *>(powerUp);

    core->BroadcastMessage(new MessageDeleteObject(powerUp->handle));
    powerUp->OnDeath();

    if (pu->health > 0)
        player->combat->health = pu->health;
    core->player->rockets += pu->rockets;

    if (pu->berserk) {
        core->player->berserk = true;
        core->player->berserkDuration = pu->duration;
        core->BroadcastMessage(new MessageSetSceneColor(1, .3, .3));
    }
}

void GamePlayController::UpdateCollisions() {
    while (!collisions.empty()) {
        CollisionPair cp = collisions.front();
        collisions.pop();

        switch(cp.first->type) {
        case GameObjectType::Projectile:
            switch(cp.second->type) {
            case GameObjectType::Projectile:
                CollisionProjectileProjectile(core, cp.first, cp.second);
                break;
            case GameObjectType::Player:
                CollisionProjectilePlayer(core, cp.first, cp.second);
                break;
            case GameObjectType::Enemy:
                CollisionProjectileEnemy(core, cp.first, cp.second);
                break;
            case GameObjectType::Terrain:
                CollisionProjectileTerrain(core, cp.first, cp.second);
                break;
            }
            break;
        case GameObjectType::Explosion:
            switch(cp.second->type) {
            case GameObjectType::Player:
                CollisionExplosionPlayer(core, cp.first, cp.second);
                break;
            case GameObjectType::Enemy:
                CollisionExplosionEnemy(core, cp.first, cp.second);
                break;
            case GameObjectType::Terrain:
                CollisionExplosionTerrain(core, cp.first, cp.second);
                break;
            }
            break;
        case GameObjectType::Player:
            switch(cp.second->type) {
            case GameObjectType::Projectile:
                CollisionProjectilePlayer(core, cp.second, cp.first);
                break;
            case GameObjectType::Explosion:
                CollisionExplosionPlayer(core, cp.second, cp.first);
                break;
            case GameObjectType::Enemy:
                CollisionPlayerEnemy(core, cp.first, cp.second);
                break;
            case GameObjectType::PowerUp:
                CollisionPlayerPowerUp(core, cp.first, cp.second);
                break;
            }
            break;
        case GameObjectType::Enemy:
            switch(cp.second->type) {
            case GameObjectType::Projectile:
                CollisionProjectileEnemy(core, cp.second, cp.first);
                break;
            case GameObjectType::Explosion:
                CollisionExplosionEnemy(core, cp.second, cp.first);
                break;
            case GameObjectType::Player:
                CollisionPlayerEnemy(core, cp.second, cp.first);
                break;
            }
            break;
        case GameObjectType::PowerUp:
            switch(cp.second->type) {
            case GameObjectType::Player:
                CollisionPlayerPowerUp(core, cp.second, cp.first);
                break;
            }
            break;
        case GameObjectType::Terrain:
            switch(cp.second->type) {
            case GameObjectType::Projectile:
                CollisionProjectileTerrain(core, cp.second, cp.first);
                break;
            case GameObjectType::Explosion:
                CollisionExplosionTerrain(core, cp.second, cp.first);
                break;
            }
            break;
        }

    }

}

void GamePlayController::Update(double frameTime) {
    UpdateCollisions();

    if (core->warmupTime > 0) {
        core->warmupTime -= frameTime;
    }
    else {
        core->gameTimeLeft -= frameTime;
    }

    // Check for end conditions
    if (core->gameTimeLeft <= 0) {
        core->gameWon = true;
        core->isGameOver = true;
        return;
    }
    else if (core->player->combat->health <= 0 || core->player->transform->translation.y < -10)
    {
        core->gameWon = false;
        core->isGameOver = true;
        return;
    }

    // Iterate through objects and update
    GameObject* obj;
    for (int i = 0; i < core->GAME_OBJECT_MAX; i++) {
        obj = core->gameObjects[i];

        if (obj)
            obj->Update(frameTime);

        if (obj && obj->combat) {
            // Check for expiring objects
            if (obj->combat->expires) {
                obj->combat->lifeTime -= (float)frameTime;

                // Shrink before destruction
                if (obj->combat->lifeTime < 1) {
                    obj->transform->scale *= .97;
                }

                if (obj->combat->lifeTime <= 0) {
                    core->BroadcastMessage(new MessageDeleteObject(obj->handle));
                }
            }

            // Check for dead objects
            if (obj->combat->health <= 0 && obj->type != GameObjectType::Player) {
                core->BroadcastMessage(new MessageDeleteObject(obj->handle));
                obj->OnDeath();
            }
        }
    }

    if (core->warmupTime > 0)
        return;

    for (int i = 0; i < core->spawners.size(); i++) {
        Spawner &spawner = core->spawners[i];
        spawner.currentTime -= frameTime;

        // Time for new spawn
        if (spawner.currentTime <= 0) {
            spawner.currentTime = spawner.time;
            bool validLocationFound = false;
            int x, y, z;
            // ineffecient
            while (!validLocationFound)
            {
                validLocationFound = true;
                x =  std::rand() % (core->mapWidth - 2) + 1;
                y = std::rand() % (core->mapHeight - 1) + 1;
                z =  std::rand() % (core->mapLength - 2) + 1;

                if (core->player->transform->translation.DistanceSquared(Vector3D(x, y, z)) < 9) {
                    validLocationFound = false;
                    continue;
                }

                GameObject *obj;
                for (int j = 0; j < core->GAME_OBJECT_MAX; j++) {
                    obj = core->gameObjects[i];

                    if (obj && obj->transform)
                    {
                        if ((int)obj->transform->translation.x == x && (int)obj->transform->translation.y == y && (int)obj->transform->translation.z == z) {
                            validLocationFound = false;
                            break;
                        }
                    }
                }
            }
            core->BroadcastMessage(new MessageSpawnEnemy(spawner.file, Vector3D(x,y,z)));
        }
    }
}