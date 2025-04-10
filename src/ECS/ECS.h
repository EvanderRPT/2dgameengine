#ifndef ECS_H
#define ECS_H

#include <bitset>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <set>

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
    inline bool isEmpty const {
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
    std::vector<IPool*> componentPools;

    // Vector of component signatures per entity
    std::vector<Signature> entityComponentSignatures;

    std::unordered_map<std::type_index, System*> systems;

    // Set of entities that are flagged to be removed in the next registry Update()
    std::set<Entity> entitiesToBeAdded;
    std::set<Entity> entitiesToBeKilled;
public:
    Registry() = default;

    Entity CreateEntity();

    // Component management
    template<class TComponent, class ...TArgs> 
    void AddComponent(Entity entity, TArgs&& ...args);
    template <class TComponent> void RemoveComponent(Entity entity);
    template <class TComponent> bool HasComponent(Entity entity);

    void Update();
    void AddEntityToSystem(Entity entity);
    



};

template <class TComponent> 
void System::RequireComponent() {
    const auto componentId = Component<TComponent>::GetId();
    componentSignature.set(componentId);
}

template<class TComponent, class ...TArgs> 
void Registry::AddComponent(Entity entity, TArgs&& ...args) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();

    if (componentId >= componentPools.size()) {
        componentPools.resize(componentId + 1, nullptr);
    }

    if (!componentPools[componentId]) {
        Pool<TComponent>* newComponentPool = new Pool<TComponent>();
        componentPools[componentId] = newComponentPool;
    }

    Pool<TComponent>* componentPool = componentPools[componentId];
    
    if (entityId >= componentPool0->GetSize()) {
        componentPool->Resize(numEntities);
    }

    TComponent newComponent(std::forward<TArgs>(args)...);

    componentPool->Set(entityId, newComponent);
    entityComponentSignatures[entityId].set(componentId);
}

template<class TComponent>
void Registry::RemoveComponent(Entity entity) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    entityComponentSignatures[entityId].set(componentId, false);
}

template<class TComponent>
bool Registry::HasComponent(Entity entity) {
    const auto componentId = Component<TComponent>::GetId();
    const auto entityId = entity.GetId();
    return entityComponentSignatures[entityId].test(componentId);
}

#endif