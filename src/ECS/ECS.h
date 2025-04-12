#ifndef ECS_H
#define ECS_H

#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>
#include <memory>
#include "../Logger/Logger.h"
const unsigned int MAX_COMPONENTS = 32;
/*
    We use a bitset (1s and 0s) to keep track of which component an entity has,
    and also helps keep track of which entities a system is interested in.
*/
typedef std::bitset<MAX_COMPONENTS> Signature;

struct IComponent {
protected:
    static int nextId;
};




// Used to assign a unique id to a component type
template <class T>
class Component: public IComponent {
public:

    static int GetId() {
        static auto id = nextId++;
        return id;
    }
};

class Entity {
private:
    int id;
public:
    Entity(int id): id(id) {};
    Entity(const Entity& entity) = default;
    int GetId() const;


    Entity& operator =(const Entity& other) = default;
    bool operator ==(const Entity& other) const { return id == other.id; }
    bool operator !=(const Entity& other) const { return id != other.id; }
    bool operator >(const Entity& other) const { return id > other.id; }
    bool operator <(const Entity& other) const { return id < other.id; }

    template<class TComponent, class ...TArgs> void AddComponent(TArgs&& ...args);
    template<class TComponent> void RemoveComponent();
    template<class TComponent> bool HasComponent() const;
    template<class TComponent> TComponent& GetComponent() const;
    // Hold a pointer to the entity's owner registry
    class Registry* registry;
};


/*
    The System Processes entities that contain a specific signature
*/
class System {
private:
    Signature componentSignature;
    std::vector<Entity> entities;

public:
    System() = default;
    ~System() = default;

    void AddEntityToSystem(Entity entity);
    void RemoveEntityFromSystem(Entity entity);
    std::vector<Entity> GetSystemEntities() const;
    const Signature& GetComponentSignature() const;

    //Define the component type that entities must have to be considered by the system    
    template <class TComponent> void RequireComponent();



};


/*
    A pool is just a vector (contiguous data) of objects of type T
*/
class IPool {
public:
    virtual ~IPool() {};
};

template<class T>
class Pool:public IPool {
private:
    std::vector<T> data;
public:
    Pool(int size = 100) {
        data.resize(size);
    }
    virtual ~Pool() = default;
    inline bool isEmpty() const {
        return data.empty();
    }
    inline int GetSize() const {
        return data.size();
    }
    inline void Resize(int n) {
        data.resize(n);
    }
    inline void Clear() {
        data.clear();
    }
    inline void Add(T object) {
        data.push_back(object);
    }
    inline void Set(int index, T object) {
        data[index] = object;
    }
    inline T& Get(int index) {
        return static_cast<T&>(data[index]);
    }
    inline T& operator [](unsigned int index) {
        return data[index];
    }


};


/*
    The registry manages the creation and destruction of entities, add systems, 
    and components.
*/
class Registry {
private:
    int numEntities = 0;

    // Vector of component pools, each pool contains all the data for a certain component type
    // Vector index = component type id
    // Pool index = entity id
    std::vector<std::shared_ptr<IPool>> componentPools;

    // Vector of component signatures per entity
    std::vector<Signature> entityComponentSignatures;

    std::unordered_map<std::type_index, std::shared_ptr<System>> systems;

    // Set of entities that are flagged to be removed in the next registry Update()
    std::set<Entity> entitiesToBeAdded;
    std::set<Entity> entitiesToBeKilled;
public:
    Registry() = default;
    ~Registry() = default;
    Entity CreateEntity();
    void Update();

    // Component management
    template<class TComponent, class ...TArgs> void AddComponent(Entity entity, TArgs&& ...args);
    template <class TComponent> void RemoveComponent(Entity entity);
    template <class TComponent> bool HasComponent(Entity entity) const;
    template <class TComponent> TComponent& GetComponent(Entity entity) const;
    // System management
    template <class TSystem, class ...TArgs> void AddSystem(TArgs&& ...args);
    template <class TSystem> void RemoveSystem();
    template <class TSystem> bool HasSystem() const;
    template <class TSystem> TSystem GetSystem() const;

    // Checks the component signature of an entity and add the entity to the systems
    // that are interested in it
    void AddEntityToSystem(Entity entity);
    



};



template <class TComponent> 
void System::RequireComponent() {
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}



template <class TSystem, class ...TArgs> 
void Registry::AddSystem(TArgs&& ...args) {
    std::shared_ptr<TSystem> newSystem = std::make_shared<TSystem>(std::forward<TArgs>(args)...);
    systems.insert(std::make_pair(std::type_index(typeid(TSystem)), newSystem));

}

template <class TSystem> 
void Registry::RemoveSystem() {
    auto system = systems.find(std::type_index(typeid(TSystem)));
    systems.erase(system);
}

template <class TSystem> bool Registry::HasSystem() const {
    return  systems.find(std::type_index(typeid(TSystem))) != systems.end();
}
template <class TSystem> TSystem Registry::GetSystem() const {
    auto system = systems.find(std::type_index(typeid(TSystem)));
    return *(std::static_pointer_cast<TSystem>(system->second));
}



// Component
template<class TComponent, class ...TArgs> 
void Registry::AddComponent(Entity entity, TArgs&& ...args) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= componentPools.size()) {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId]) {
        std::shared_ptr<Pool<TComponent>> newComponentPool = std::make_shared<Pool<TComponent>>();
        componentPools[componentId] = newComponentPool;
    }

    std::shared_ptr<Pool<TComponent>> componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    
    if (entityId >= componentPool->GetSize()) {
        componentPool->Resize(numEntities);
    }

    TComponent newComponent(std::forward<TArgs>(args)...);

    componentPool->Set(entityId, newComponent);
    entityComponentSignatures[entityId].set(componentId);

    Logger::Log("Component id = " + std::to_string(componentId) + " was added to entity id " + std::to_string(entityId));

}

template<class TComponent>
void Registry::RemoveComponent(Entity entity) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    entityComponentSignatures[entityId].set(componentId, false);
    Logger::Log("Component id = " + std::to_string(componentId) + " was removed from entity id " + std::to_string(entityId));

}

template<class TComponent>
bool Registry::HasComponent(Entity entity) const{
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return entityComponentSignatures[entityId].test(componentId);
}

template<class TComponent>
TComponent& Registry::GetComponent(Entity entity) const {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    auto componentPool = std::static_pointer_cast<Pool<TComponent>>(componentPools[componentId]);
    return componentPool->Get(entityId);
}


template <typename TComponent, typename ...TArgs>
void Entity::AddComponent(TArgs&& ...args) {
    registry->AddComponent<TComponent>(*this, std::forward<TArgs>(args)...);
}


template <typename TComponent>
void Entity::RemoveComponent() {
    registry->RemoveComponent<TComponent>(*this);
}

template <typename TComponent>
bool Entity::HasComponent() const  {
    return registry->HasComponent<TComponent>(*this);
}

template <typename TComponent>
TComponent& Entity::GetComponent() const {
    return registry->GetComponent<TComponent>(*this);
}

#endif