#include "ECS.h"
#include  <algorithm>
#include "../Logger/Logger.h"

int IComponent::nextId = 0;

int Entity::GetId() const {
    return id;
}

void System::AddEntityToSystem(Entity entity) {
    entities.push_back(entity);
}

void System::RemoveEntityFromSystem(Entity entity) {
    entities.erase(std::remove_if(entities.begin(), entities.end(), [&entity](Entity other) {
        return entity == other;
    }), entities.end());
    
}

std::vector<Entity> System::GetSystemEntities() const {
    return entities;
}

const Signature& System::GetComponentSignature() const {
    return componentSignature;
}


Entity Registry::CreateEntity() {
    int entityId;
    entityId = numEntities++;
    
    Entity entity(entityId);
    entitiesToBeAdded.insert(entity);

    // Make sure the entityComponentSignatures vector can accomodate the new entity
    if (entityId >= entityComponentSignatures.size()) {
        entityComponentSignatures.reserve(entityId + 1);
    }
    
    Logger::Log("Entity Create with id = " + std::to_string(entityId));

    return entity;
}

void Registry::AddEntityToSystem(Entity entity) {
    const auto entityId = entity.GetId();

    const auto& entityComponentSignature = entityComponentSignatures[entityId];

    // loop all the systems
    for (auto& system : systems) {
        const auto& systemComponentSignature = system.second->GetComponentSignature();
        bool isInterested = (entityComponentSignature & systemComponentSignature) == systemComponentSignature;
        if (isInterested) {
            system.second->AddEntityToSystem(entity);
        }
        
    }
}
void Registry::Update() {
    // Add the entities that are waiting to be created to the action System
    for (auto entity : entitiesToBeAdded) {
        AddEntityToSystem(entity);
    }
    entitiesToBeAdded.clear();

}